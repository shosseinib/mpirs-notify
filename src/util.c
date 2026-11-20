#include "mpirs-notify.h"
#include <string.h>
#include <stdlib.h>

void util_truncate_string(gchar **str, gsize max_len) {
    if (!str || !*str) return;
    gsize len = strlen(*str);
    if (len <= max_len) return;
    (*str)[max_len] = '\0';
}

gchar *util_join_strings(gchar **arr, const gchar *sep) {
    if (!arr || !arr[0]) return g_strdup("");
    if (!arr[1]) return g_strdup(arr[0]);

    GString *result = g_string_new(arr[0]);
    for (gsize i = 1; arr[i]; i++) {
        g_string_append(result, sep);
        g_string_append(result, arr[i]);
    }
    return g_string_free(result, FALSE);
}

guint32 util_parse_year_from_iso8601(const gchar *iso8601) {
    if (!iso8601 || strlen(iso8601) < 4) return 0;
    gchar year_str[5] = {0};
    strncpy(year_str, iso8601, 4);
    gint year = atoi(year_str);
    if (year > 0 && year < 2100) return (guint32)year;
    return 0;
}

gchar *util_strdup_variant_string(GVariant *variant) {
    if (!variant) return NULL;
    const gchar *str = g_variant_get_string(variant, NULL);
    return str ? g_strdup(str) : NULL;
}

gchar **util_strdup_variant_string_array(GVariant *variant) {
    if (!variant) return NULL;
    gsize len = g_variant_n_children(variant);
    gchar **arr = g_new0(gchar *, len + 1);
    for (gsize i = 0; i < len; i++) {
        GVariant *child = g_variant_get_child_value(variant, i);
        const gchar *str = g_variant_get_string(child, NULL);
        arr[i] = str ? g_strdup(str) : NULL;
        g_variant_unref(child);
    }
    return arr;
}