#include "mpirs-notify.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [OPTIONS]\n", prog);
    fprintf(stderr, "Show now-playing notification from MPRIS players.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -t, --timeout MS      Notification timeout in milliseconds (default: 5000)\n");
    fprintf(stderr, "  -l, --list-players    List available MPRIS players and exit\n");
    fprintf(stderr, "  -p, --print-metadata  Print metadata of current track and exit\n");
    fprintf(stderr, "  -v, --version         Show version and exit\n");
    fprintf(stderr, "  -h, --help            Show this help and exit\n");
}

static void print_version(void) {
    printf("mpirs-notify %s\n", VERSION);
}

static gboolean list_players(void) {
    DBusConnection *conn = dbus_get_connection();
    if (!conn) return FALSE;

    DBusMessage *msg = dbus_message_new_method_call("org.freedesktop.DBus",
                                                     "/org/freedesktop/DBus",
                                                     "org.freedesktop.DBus",
                                                     "ListNames");
    if (!msg) return FALSE;

    DBusError err;
    dbus_error_init(&err);
    DBusMessage *reply = dbus_connection_send_with_reply_and_block(conn, msg, 2000, &err);
    dbus_message_unref(msg);

    if (dbus_error_is_set(&err)) {
        dbus_error_free(&err);
        return FALSE;
    }

    DBusMessageIter iter;
    dbus_message_iter_init(reply, &iter);
    DBusMessageIter array_iter;
    dbus_message_iter_recurse(&iter, &array_iter);

    gboolean found = FALSE;
    while (dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_INVALID) {
        const gchar *name;
        dbus_message_iter_get_basic(&array_iter, &name);
        if (g_str_has_prefix(name, "org.mpris.MediaPlayer2.")) {
            printf("%s\n", name);
            found = TRUE;
        }
        dbus_message_iter_next(&array_iter);
    }
    dbus_message_unref(reply);
    return found;
}

static gboolean print_metadata(void) {
    gchar *bus_name = NULL;
    gchar *identity = NULL;
    gchar *desktop_entry = NULL;
    Config *cfg = config_load();

    if (!mpris_discover_player(&bus_name, &identity, &desktop_entry, cfg)) {
        fprintf(stderr, "No active MPRIS player found\n");
        return FALSE;
    }

    MprisMetadata *md = NULL;
    if (!mpris_get_metadata(bus_name, &md)) {
        fprintf(stderr, "Failed to get metadata\n");
        g_free(bus_name);
        g_free(identity);
        g_free(desktop_entry);
        return FALSE;
    }

    printf("Player: %s (%s)\n", identity ? identity : "Unknown", bus_name);
    printf("DesktopEntry: %s\n", desktop_entry ? desktop_entry : "N/A");
    printf("Title: %s\n", md->title ? md->title : "N/A");
    printf("Artist(s): ");
    if (md->artists) {
        for (gchar **a = md->artists; *a; a++) {
            printf("%s%s", *a, (*(a+1) ? ", " : ""));
        }
    }
    printf("\n");
    printf("Album: %s\n", md->album ? md->album : "N/A");
    printf("Album Artist: %s\n", md->album_artist ? md->album_artist : "N/A");
    printf("Year: %u\n", md->year);
    printf("Track Number: %u\n", md->track_number);
    printf("Disc Number: %u\n", md->disc_number);
    printf("Art URL: %s\n", md->art_url ? md->art_url : "N/A");
    printf("Track ID: %s\n", md->track_id ? md->track_id : "N/A");
    printf("Length (us): %ld\n", md->length_us);
    printf("URL: %s\n", md->url ? md->url : "N/A");

    mpris_metadata_free(md);
    g_free(bus_name);
    g_free(identity);
    g_free(desktop_entry);
    if (cfg) config_free(cfg);
    return TRUE;
}

int main(int argc, char *argv[]) {
    static struct option long_opts[] = {
        {"timeout", required_argument, 0, 't'},
        {"list-players", no_argument, 0, 'l'},
        {"print-metadata", no_argument, 0, 'p'},
        {"version", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    gint timeout_ms = 5000;
    gboolean do_list = FALSE;
    gboolean do_print = FALSE;

    int opt;
    while ((opt = getopt_long(argc, argv, "t:lpvh", long_opts, NULL)) != -1) {
        switch (opt) {
            case 't':
                timeout_ms = atoi(optarg);
                if (timeout_ms <= 0) timeout_ms = 5000;
                break;
            case 'l':
                do_list = TRUE;
                break;
            case 'p':
                do_print = TRUE;
                break;
            case 'v':
                print_version();
                return 0;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (do_list) {
        if (!list_players()) {
            fprintf(stderr, "No MPRIS players found\n");
            return 1;
        }
        return 0;
    }

    if (do_print) {
        return print_metadata() ? 0 : 1;
    }

    Config *cfg = config_load();
    if (cfg && cfg->timeout_ms > 0) {
        timeout_ms = cfg->timeout_ms;
    }

    gchar *bus_name = NULL;
    gchar *identity = NULL;
    gchar *desktop_entry = NULL;

    if (!mpris_discover_player(&bus_name, &identity, &desktop_entry, cfg)) {
        fprintf(stderr, "No active MPRIS player found\n");
        if (cfg) config_free(cfg);
        return 1;
    }

    MprisMetadata *md = NULL;
    if (!mpris_get_metadata(bus_name, &md)) {
        fprintf(stderr, "Failed to get metadata\n");
        g_free(bus_name);
        g_free(identity);
        g_free(desktop_entry);
        if (cfg) config_free(cfg);
        return 1;
    }

    ArtResult *art = NULL;
    const gchar *cache_dir = cfg && cfg->cache_dir ? cfg->cache_dir : "/tmp/mpirs-notify-art";
    gsize cache_max = cfg && cfg->cache_max_items > 0 ? cfg->cache_max_items : 20;
    if (md->art_url) {
        art_fetch(md->art_url, &art, cache_dir, cache_max);
    }

    PlayerIcon *player_icon = NULL;
    player_icon_resolve(md, &player_icon);

    gboolean res = notify_show(md, art, player_icon, timeout_ms);

    notify_cleanup();

    if (art) art_result_free(art);
    if (player_icon) player_icon_free(player_icon);
    mpris_metadata_free(md);
    g_free(bus_name);
    g_free(identity);
    g_free(desktop_entry);
    if (cfg) config_free(cfg);

    return res ? 0 : 1;
}