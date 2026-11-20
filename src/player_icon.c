#include "mpirs-notify.h"
#include <string.h>

static gboolean icon_theme_has_icon_simple(const gchar *icon_name) {
    GFile *file = g_file_new_for_path("/usr/share/icons");
    gboolean has = FALSE;
    if (g_file_query_exists(file, NULL)) {
        GFileEnumerator *enumr = g_file_enumerate_children(file,
            "standard::name,standard::type",
            G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);
        if (enumr) {
            GFileInfo *info;
            while ((info = g_file_enumerator_next_file(enumr, NULL, NULL)) != NULL) {
                const gchar *name = g_file_info_get_name(info);
                if (g_str_has_prefix(name, icon_name)) {
                    has = TRUE;
                    g_object_unref(info);
                    break;
                }
                g_object_unref(info);
            }
            g_object_unref(enumr);
        }
    }
    g_object_unref(file);
    return has;
}

static gchar *icon_try_desktop_entry(const gchar *desktop_entry) {
    if (!desktop_entry) return NULL;

    gchar *lower = g_ascii_strdown(desktop_entry, -1);
    if (icon_theme_has_icon_simple(lower)) {
        return lower;
    }

    const gchar *suffixes[] = { "-media-player", "-player", "", NULL };
    for (int i = 0; suffixes[i]; i++) {
        gchar *candidate = g_strdup_printf("%s%s", desktop_entry, suffixes[i]);
        gchar *cand_lower = g_ascii_strdown(candidate, -1);
        g_free(candidate);
        if (icon_theme_has_icon_simple(cand_lower)) {
            g_free(lower);
            return cand_lower;
        }
        g_free(cand_lower);
    }
    g_free(lower);
    return NULL;
}

static gchar *icon_try_identity(const gchar *identity) {
    if (!identity) return NULL;

    gchar *lower = g_ascii_strdown(identity, -1);
    g_strdelimit(lower, " ", '-');
    if (icon_theme_has_icon_simple(lower)) {
        return lower;
    }
    g_free(lower);
    return NULL;
}

gboolean player_icon_resolve(const MprisMetadata *md, PlayerIcon **out_icon) {
    if (!md) return FALSE;

    PlayerIcon *pi = g_new0(PlayerIcon, 1);

    gchar *icon_name = NULL;

    if (md->player_desktop_entry) {
        icon_name = icon_try_desktop_entry(md->player_desktop_entry);
    }
    if (!icon_name && md->player_identity) {
        icon_name = icon_try_identity(md->player_identity);
    }
    if (!icon_name) {
        icon_name = g_strdup("audio-x-generic");
    }

    pi->icon_name = icon_name;
    pi->pixbuf = NULL;

    *out_icon = pi;
    return TRUE;
}

void player_icon_free(PlayerIcon *pi) {
    if (!pi) return;
    g_free(pi->icon_name);
    if (pi->pixbuf) g_object_unref(pi->pixbuf);
    g_free(pi);
}