#include "mpirs-notify.h"
#include <string.h>

gchar *metadata_get_first_artist(const MprisMetadata *md) {
    if (!md || !md->artists || !md->artists[0]) return NULL;
    return g_strdup(md->artists[0]);
}

gchar *metadata_format_artist_string(const MprisMetadata *md) {
    if (!md || !md->artists) return g_strdup("");
    return util_join_strings(md->artists, ", ");
}

gboolean metadata_has_valid_track(const MprisMetadata *md) {
    return md && md->title && md->title[0] != '\0';
}