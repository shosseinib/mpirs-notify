#include "mpirs-notify.h"
#include <gio/gio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static GHashTable *art_cache = NULL;

static void art_cache_init(const gchar *cache_dir, gsize max_items) {
    (void)max_items;
    if (art_cache) return;
    art_cache = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    g_mkdir_with_parents(cache_dir, 0700);
}

static gchar *art_cache_get(const gchar *art_url) {
    if (!art_cache) return NULL;
    return g_hash_table_lookup(art_cache, art_url);
}

static void art_cache_put(const gchar *art_url, const gchar *local_path) {
    if (!art_cache) return;
    g_hash_table_insert(art_cache, g_strdup(art_url), g_strdup(local_path));
}

static gchar *art_download_http(const gchar *url, const gchar *cache_dir) {
    GFile *src = g_file_new_for_uri(url);
    gchar *basename = g_file_get_basename(src);
    if (!basename) basename = g_strdup("art.jpg");

    gchar *dest_path = g_build_filename(cache_dir, basename, NULL);
    g_free(basename);

    GFile *dest = g_file_new_for_path(dest_path);
    GError *err = NULL;
    gboolean res = g_file_copy(src, dest, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &err);
    g_object_unref(src);
    g_object_unref(dest);

    if (!res) {
        g_warning("Failed to download art: %s", err ? err->message : "unknown error");
        g_clear_error(&err);
        g_free(dest_path);
        return NULL;
    }
    return dest_path;
}

static gchar *art_handle_data_uri(const gchar *uri, const gchar *cache_dir) {
    const gchar *prefix = "data:";
    if (!g_str_has_prefix(uri, prefix)) return NULL;
    const gchar *data = uri + strlen(prefix);

    const gchar *comma = strchr(data, ',');
    if (!comma) return NULL;
    comma++;

    gsize decoded_len = 0;
    guchar *decoded = g_base64_decode(comma, &decoded_len);
    if (!decoded) return NULL;

    gchar *template = g_build_filename(cache_dir, "mpirs-art-XXXXXX", NULL);
    gint fd = g_mkstemp(template);
    if (fd == -1) {
        g_free(decoded);
        g_free(template);
        return NULL;
    }
    gsize written = write(fd, decoded, decoded_len);
    close(fd);
    g_free(decoded);

    if (written != decoded_len) {
        unlink(template);
        g_free(template);
        return NULL;
    }
    return template;
}

static GdkPixbuf *art_load_pixbuf(const gchar *path) {
    GError *err = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(path, &err);
    if (!pixbuf) {
        g_warning("Failed to load pixbuf: %s", err ? err->message : "unknown error");
        g_clear_error(&err);
    }
    return pixbuf;
}

gboolean art_fetch(const gchar *art_url, ArtResult **out_result, const gchar *cache_dir, gsize cache_max_items) {
    if (!art_url || !art_url[0]) return FALSE;

    art_cache_init(cache_dir, cache_max_items);

    gchar *cached = art_cache_get(art_url);
    if (cached) {
        GdkPixbuf *pixbuf = art_load_pixbuf(cached);
        if (pixbuf) {
            ArtResult *ar = g_new0(ArtResult, 1);
            ar->pixbuf = pixbuf;
            ar->cached_path = g_strdup(cached);
            ar->is_temp = FALSE;
            *out_result = ar;
            return TRUE;
        }
    }

    gchar *local_path = NULL;
    gboolean is_temp = FALSE;

    if (g_str_has_prefix(art_url, "file://")) {
        local_path = g_filename_from_uri(art_url, NULL, NULL);
    } else if (g_str_has_prefix(art_url, "http://") || g_str_has_prefix(art_url, "https://")) {
        local_path = art_download_http(art_url, cache_dir);
        is_temp = TRUE;
    } else if (g_str_has_prefix(art_url, "data:")) {
        local_path = art_handle_data_uri(art_url, cache_dir);
        is_temp = TRUE;
    }

    if (!local_path) return FALSE;

    GdkPixbuf *pixbuf = art_load_pixbuf(local_path);
    if (!pixbuf) {
        if (is_temp) unlink(local_path);
        g_free(local_path);
        return FALSE;
    }

    art_cache_put(art_url, local_path);

    ArtResult *ar = g_new0(ArtResult, 1);
    ar->pixbuf = pixbuf;
    ar->cached_path = local_path;
    ar->is_temp = is_temp;
    *out_result = ar;
    return TRUE;
}

void art_result_free(ArtResult *ar) {
    if (!ar) return;
    if (ar->pixbuf) g_object_unref(ar->pixbuf);
    if (ar->is_temp && ar->cached_path) unlink(ar->cached_path);
    g_free(ar->cached_path);
    g_free(ar);
}