#include "mpirs-notify.h"
#include <glib.h>
#include <string.h>

static const gchar *default_cache_dir = "/tmp/mpirs-notify-art";

Config *config_load(void) {
    Config *cfg = g_new0(Config, 1);
    cfg->timeout_ms = 5000;
    cfg->show_year = TRUE;
    cfg->show_album = TRUE;
    cfg->truncate_chars = 128;
    cfg->cache_dir = g_strdup(default_cache_dir);
    cfg->cache_max_items = 20;
    cfg->fallback_icon = g_strdup("audio-x-generic");

    const gchar *config_dir = g_get_user_config_dir();
    gchar *config_file = g_build_filename(config_dir, "mpirs-notify", "config.ini", NULL);

    GKeyFile *kf = g_key_file_new();
    if (g_key_file_load_from_file(kf, config_file, G_KEY_FILE_NONE, NULL)) {
        gint timeout = g_key_file_get_integer(kf, "notification", "timeout_ms", NULL);
        if (timeout > 0) cfg->timeout_ms = timeout;

        cfg->show_year = g_key_file_get_boolean(kf, "notification", "show_year", NULL);
        cfg->show_album = g_key_file_get_boolean(kf, "notification", "show_album", NULL);

        gsize trunc = g_key_file_get_uint64(kf, "notification", "truncate_chars", NULL);
        if (trunc > 0) cfg->truncate_chars = trunc;

        gchar *cache_dir = g_key_file_get_string(kf, "art", "cache_dir", NULL);
        if (cache_dir) {
            g_free(cfg->cache_dir);
            cfg->cache_dir = cache_dir;
        }

        gsize max_items = g_key_file_get_uint64(kf, "art", "cache_max_items", NULL);
        if (max_items > 0) cfg->cache_max_items = max_items;

        gchar *fallback = g_key_file_get_string(kf, "art", "fallback_icon", NULL);
        if (fallback) {
            g_free(cfg->fallback_icon);
            cfg->fallback_icon = fallback;
        }

        gchar **ignore = g_key_file_get_string_list(kf, "player", "ignore_players", NULL, NULL);
        if (ignore) {
            for (gchar **p = ignore; *p; p++) {
            }
            cfg->ignore_players = ignore;
        }
    }
    g_key_file_free(kf);
    g_free(config_file);

    return cfg;
}

void config_free(Config *cfg) {
    if (!cfg) return;
    g_free(cfg->cache_dir);
    g_free(cfg->fallback_icon);
    g_strfreev(cfg->ignore_players);
    g_free(cfg);
}