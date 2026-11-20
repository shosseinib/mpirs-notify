#ifndef MPIRS_NOTIFY_H
#define MPIRS_NOTIFY_H

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <dbus/dbus.h>

typedef struct {
    gchar *title;
    gchar **artists;
    gchar *album;
    gchar *album_artist;
    guint32 year;
    guint32 track_number;
    guint32 disc_number;
    gchar *art_url;
    gchar *track_id;
    gint64 length_us;
    gchar *url;
    gchar *player_name;
    gchar *player_identity;
    gchar *player_desktop_entry;
} MprisMetadata;

typedef struct {
    GdkPixbuf *pixbuf;
    gchar *cached_path;
    gboolean is_temp;
} ArtResult;

typedef struct {
    gchar *icon_name;
    GdkPixbuf *pixbuf;
} PlayerIcon;

typedef struct {
    gint timeout_ms;
    gboolean show_year;
    gboolean show_album;
    gsize truncate_chars;
    gchar *cache_dir;
    gsize cache_max_items;
    gchar *fallback_icon;
    gchar **ignore_players;
} Config;

gboolean mpris_discover_player(gchar **out_bus_name, gchar **out_identity, gchar **out_desktop_entry, const Config *cfg);
gboolean mpris_get_metadata(const gchar *bus_name, MprisMetadata **out_metadata);
void mpris_metadata_free(MprisMetadata *md);

gboolean art_fetch(const gchar *art_url, ArtResult **out_result, const gchar *cache_dir, gsize cache_max_items);
void art_result_free(ArtResult *ar);

gboolean player_icon_resolve(const MprisMetadata *md, PlayerIcon **out_icon);
void player_icon_free(PlayerIcon *pi);

gboolean notify_show(const MprisMetadata *md, const ArtResult *art,
                     const PlayerIcon *player_icon, gint timeout_ms);

Config *config_load(void);
void config_free(Config *cfg);

void util_truncate_string(gchar **str, gsize max_len);
gchar *util_join_strings(gchar **arr, const gchar *sep);
guint32 util_parse_year_from_iso8601(const gchar *iso8601);
gchar *util_strdup_variant_string(GVariant *variant);
gchar **util_strdup_variant_string_array(GVariant *variant);

gchar *metadata_get_first_artist(const MprisMetadata *md);
gchar *metadata_format_artist_string(const MprisMetadata *md);
gboolean metadata_has_valid_track(const MprisMetadata *md);

void notify_cleanup(void);

DBusConnection *dbus_get_connection(void);

#endif