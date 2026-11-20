#include "mpirs-notify.h"
#include <libnotify/notify.h>
#include <string.h>

static gboolean notify_inited = FALSE;

static void notify_ensure_init(void) {
    if (!notify_inited) {
        notify_init("mpirs-notify");
        notify_inited = TRUE;
    }
}

gboolean notify_show(const MprisMetadata *md, const ArtResult *art,
                     const PlayerIcon *player_icon, gint timeout_ms) {
    if (!md || !metadata_has_valid_track(md)) return FALSE;

    notify_ensure_init();

    gchar *artist_str = metadata_format_artist_string(md);
    gchar *summary = g_strdup_printf("%s - %s", artist_str, md->title);
    g_free(artist_str);

    gchar *body = NULL;
    if (md->year > 0 && md->album && md->album[0]) {
        body = g_strdup_printf("%u - %s", md->year, md->album);
    } else if (md->year > 0) {
        body = g_strdup_printf("%u", md->year);
    } else if (md->album && md->album[0]) {
        body = g_strdup(md->album);
    }

    NotifyNotification *n = notify_notification_new(summary, body ? body : "", player_icon ? player_icon->icon_name : "audio-x-generic");
    g_free(summary);
    g_free(body);

    if (md->player_identity) {
        notify_notification_set_app_name(n, md->player_identity);
    }

    if (player_icon && player_icon->pixbuf) {
        notify_notification_set_icon_from_pixbuf(n, player_icon->pixbuf);
    }

    notify_notification_set_urgency(n, NOTIFY_URGENCY_LOW);
    notify_notification_set_timeout(n, timeout_ms);
    notify_notification_set_category(n, "x-mpris.now-playing");

    if (art && art->pixbuf) {
        notify_notification_set_image_from_pixbuf(n, art->pixbuf);
    }

    GError *err = NULL;
    gboolean res = notify_notification_show(n, &err);
    if (!res && err) {
        g_warning("Failed to show notification: %s", err->message);
        g_error_free(err);
    }

    g_object_unref(n);
    return res;
}

void notify_cleanup(void) {
    if (notify_inited) {
        notify_uninit();
        notify_inited = FALSE;
    }
}