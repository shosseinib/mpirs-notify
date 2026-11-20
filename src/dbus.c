#include "mpirs-notify.h"
#include <dbus/dbus.h>
#include <string.h>

static DBusConnection *dbus_conn = NULL;

DBusConnection *dbus_get_connection(void) {
    if (dbus_conn) return dbus_conn;
    DBusError err;
    dbus_error_init(&err);
    dbus_conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        dbus_error_free(&err);
        return NULL;
    }
    return dbus_conn;
}

static gboolean dbus_get_property(const gchar *bus_name, const gchar *path,
                                   const gchar *interface, const gchar *property,
                                   GVariant **out_value) {
    DBusConnection *conn = dbus_get_connection();
    if (!conn) return FALSE;

    DBusMessage *msg = dbus_message_new_method_call(bus_name, path,
                                                     "org.freedesktop.DBus.Properties", "Get");
    if (!msg) return FALSE;

    DBusMessageIter iter;
    dbus_message_iter_init_append(msg, &iter);
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &interface)) {
        dbus_message_unref(msg);
        return FALSE;
    }
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &property)) {
        dbus_message_unref(msg);
        return FALSE;
    }

    DBusError err;
    dbus_error_init(&err);
    DBusMessage *reply = dbus_connection_send_with_reply_and_block(conn, msg, 2000, &err);
    dbus_message_unref(msg);

    if (dbus_error_is_set(&err)) {
        dbus_error_free(&err);
        return FALSE;
    }

    DBusMessageIter reply_iter;
    dbus_message_iter_init(reply, &reply_iter);
    int arg_type = dbus_message_iter_get_arg_type(&reply_iter);
    if (DBUS_TYPE_VARIANT != arg_type) {
        dbus_message_unref(reply);
        return FALSE;
    }

    DBusMessageIter variant_iter;
    dbus_message_iter_recurse(&reply_iter, &variant_iter);
    int dbus_type = dbus_message_iter_get_arg_type(&variant_iter);
    GVariant *gv = NULL;

    switch (dbus_type) {
        case DBUS_TYPE_STRING: {
            const gchar *str;
            dbus_message_iter_get_basic(&variant_iter, &str);
            gv = g_variant_new_string(str);
            break;
        }
        case DBUS_TYPE_BOOLEAN: {
            dbus_bool_t val;
            dbus_message_iter_get_basic(&variant_iter, &val);
            gv = g_variant_new_boolean(val);
            break;
        }
        case DBUS_TYPE_INT32: {
            gint32 val;
            dbus_message_iter_get_basic(&variant_iter, &val);
            gv = g_variant_new_int32(val);
            break;
        }
        case DBUS_TYPE_UINT32: {
            guint32 val;
            dbus_message_iter_get_basic(&variant_iter, &val);
            gv = g_variant_new_uint32(val);
            break;
        }
        case DBUS_TYPE_INT64: {
            gint64 val;
            dbus_message_iter_get_basic(&variant_iter, &val);
            gv = g_variant_new_int64(val);
            break;
        }
        case DBUS_TYPE_DOUBLE: {
            gdouble val;
            dbus_message_iter_get_basic(&variant_iter, &val);
            gv = g_variant_new_double(val);
            break;
        }
        case DBUS_TYPE_OBJECT_PATH: {
            const gchar *path_str;
            dbus_message_iter_get_basic(&variant_iter, &path_str);
            gv = g_variant_new_string(path_str);
            break;
        }
        case DBUS_TYPE_ARRAY: {
            int element_type = dbus_message_iter_get_element_type(&variant_iter);
            if (element_type == DBUS_TYPE_STRING) {
                GPtrArray *arr = g_ptr_array_new_with_free_func(g_free);
                DBusMessageIter array_iter;
                dbus_message_iter_recurse(&variant_iter, &array_iter);
                while (dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_INVALID) {
                    const gchar *str;
                    dbus_message_iter_get_basic(&array_iter, &str);
                    g_ptr_array_add(arr, g_strdup(str));
                    dbus_message_iter_next(&array_iter);
                }
                gv = g_variant_new_strv((const gchar * const *)arr->pdata, arr->len);
                g_ptr_array_free(arr, TRUE);
            } else if (element_type == DBUS_TYPE_DICT_ENTRY) {
                GVariantBuilder builder;
                g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
                DBusMessageIter array_iter;
                dbus_message_iter_recurse(&variant_iter, &array_iter);
                while (dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_INVALID) {
                    DBusMessageIter entry_iter;
                    dbus_message_iter_recurse(&array_iter, &entry_iter);
                    const gchar *key;
                    dbus_message_iter_get_basic(&entry_iter, &key);
                    dbus_message_iter_next(&entry_iter);
                    int val_type = dbus_message_iter_get_arg_type(&entry_iter);
                    GVariant *val_gv = NULL;
                    switch (val_type) {
                        case DBUS_TYPE_STRING: {
                            const gchar *v; dbus_message_iter_get_basic(&entry_iter, &v);
                            val_gv = g_variant_new_string(v); break;
                        }
                        case DBUS_TYPE_BOOLEAN: {
                            dbus_bool_t v; dbus_message_iter_get_basic(&entry_iter, &v);
                            val_gv = g_variant_new_boolean(v); break;
                        }
                        case DBUS_TYPE_INT32: {
                            gint32 v; dbus_message_iter_get_basic(&entry_iter, &v);
                            val_gv = g_variant_new_int32(v); break;
                        }
                        case DBUS_TYPE_UINT32: {
                            guint32 v; dbus_message_iter_get_basic(&entry_iter, &v);
                            val_gv = g_variant_new_uint32(v); break;
                        }
                        case DBUS_TYPE_INT64: {
                            gint64 v; dbus_message_iter_get_basic(&entry_iter, &v);
                            val_gv = g_variant_new_int64(v); break;
                        }
                        case DBUS_TYPE_DOUBLE: {
                            gdouble v; dbus_message_iter_get_basic(&entry_iter, &v);
                            val_gv = g_variant_new_double(v); break;
                        }
                        case DBUS_TYPE_OBJECT_PATH: {
                            const gchar *v; dbus_message_iter_get_basic(&entry_iter, &v);
                            val_gv = g_variant_new_string(v); break;
                        }
                        case DBUS_TYPE_ARRAY: {
                            if (dbus_message_iter_get_element_type(&entry_iter) == DBUS_TYPE_STRING) {
                                GPtrArray *arr = g_ptr_array_new_with_free_func(g_free);
                                DBusMessageIter aiter;
                                dbus_message_iter_recurse(&entry_iter, &aiter);
                                while (dbus_message_iter_get_arg_type(&aiter) != DBUS_TYPE_INVALID) {
                                    const gchar *v; dbus_message_iter_get_basic(&aiter, &v);
                                    g_ptr_array_add(arr, g_strdup(v));
                                    dbus_message_iter_next(&aiter);
                                }
                                val_gv = g_variant_new_strv((const gchar * const *)arr->pdata, arr->len);
                                g_ptr_array_free(arr, TRUE);
                            }
                            break;
                        }
                        case DBUS_TYPE_VARIANT: {
                            DBusMessageIter variant_iter;
                            dbus_message_iter_recurse(&entry_iter, &variant_iter);
                            int variant_type = dbus_message_iter_get_arg_type(&variant_iter);
                            switch (variant_type) {
                                case DBUS_TYPE_STRING: {
                                    const gchar *v; dbus_message_iter_get_basic(&variant_iter, &v);
                                    val_gv = g_variant_new_string(v); break;
                                }
                                case DBUS_TYPE_BOOLEAN: {
                                    dbus_bool_t v; dbus_message_iter_get_basic(&variant_iter, &v);
                                    val_gv = g_variant_new_boolean(v); break;
                                }
                                case DBUS_TYPE_INT32: {
                                    gint32 v; dbus_message_iter_get_basic(&variant_iter, &v);
                                    val_gv = g_variant_new_int32(v); break;
                                }
                                case DBUS_TYPE_UINT32: {
                                    guint32 v; dbus_message_iter_get_basic(&variant_iter, &v);
                                    val_gv = g_variant_new_uint32(v); break;
                                }
                                case DBUS_TYPE_INT64: {
                                    gint64 v; dbus_message_iter_get_basic(&variant_iter, &v);
                                    val_gv = g_variant_new_int64(v); break;
                                }
                                case DBUS_TYPE_DOUBLE: {
                                    gdouble v; dbus_message_iter_get_basic(&variant_iter, &v);
                                    val_gv = g_variant_new_double(v); break;
                                }
                                case DBUS_TYPE_OBJECT_PATH: {
                                    const gchar *v; dbus_message_iter_get_basic(&variant_iter, &v);
                                    val_gv = g_variant_new_string(v); break;
                                }
                                case DBUS_TYPE_ARRAY: {
                                    if (dbus_message_iter_get_element_type(&variant_iter) == DBUS_TYPE_STRING) {
                                        GPtrArray *arr = g_ptr_array_new_with_free_func(g_free);
                                        DBusMessageIter aiter;
                                        dbus_message_iter_recurse(&variant_iter, &aiter);
                                        while (dbus_message_iter_get_arg_type(&aiter) != DBUS_TYPE_INVALID) {
                                            const gchar *v; dbus_message_iter_get_basic(&aiter, &v);
                                            g_ptr_array_add(arr, g_strdup(v));
                                            dbus_message_iter_next(&aiter);
                                        }
                                        val_gv = g_variant_new_strv((const gchar * const *)arr->pdata, arr->len);
                                        g_ptr_array_free(arr, TRUE);
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    if (val_gv) {
                        GVariant *variant_val = g_variant_new_variant(val_gv);
                        g_variant_ref_sink(variant_val);
                        g_variant_builder_add(&builder, "{sv}", key, variant_val);
                        g_variant_unref(variant_val);
                        // variant_val takes ownership of val_gv's reference, don't unref val_gv separately
                    }
                    dbus_message_iter_next(&array_iter);
                }
                gv = g_variant_builder_end(&builder);
            // Debug: iterate and print each entry
            GVariantIter debug_iter;
            g_variant_iter_init(&debug_iter, gv);
            const gchar *dkey;
            GVariant *dvalue;
            while (g_variant_iter_next(&debug_iter, "{&sv}", &dkey, &dvalue)) {
                g_variant_unref(dvalue);
            }
            }
            break;
        }
    }
    dbus_message_unref(reply);
    if (gv) {
        *out_value = gv;
        return TRUE;
    }
    return FALSE;
}

gboolean mpris_discover_player(gchar **out_bus_name, gchar **out_identity, gchar **out_desktop_entry, const Config *cfg) {
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

    DBusMessageIter reply_iter;
    dbus_message_iter_init(reply, &reply_iter);
    DBusMessageIter array_iter;
    dbus_message_iter_recurse(&reply_iter, &array_iter);

    GPtrArray *candidates = g_ptr_array_new_with_free_func(g_free);

    while (dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_INVALID) {
        const gchar *name;
        dbus_message_iter_get_basic(&array_iter, &name);
        if (g_str_has_prefix(name, "org.mpris.MediaPlayer2.")) {
            g_ptr_array_add(candidates, g_strdup(name));
        }
        dbus_message_iter_next(&array_iter);
    }
    dbus_message_unref(reply);

    if (candidates->len == 0) {
        g_ptr_array_free(candidates, TRUE);
        return FALSE;
    }

    gchar *best_bus = NULL;
    gchar *best_identity = NULL;
    gchar *best_desktop_entry = NULL;
    gboolean best_is_playing = FALSE;

    for (guint i = 0; i < candidates->len; i++) {
        const gchar *bus_name = (const gchar *)g_ptr_array_index(candidates, i);

        // Check if player should be ignored
        if (cfg && cfg->ignore_players) {
            for (gchar **ign = cfg->ignore_players; *ign; ign++) {
                if (g_strstr_len(bus_name, -1, *ign) != NULL) {
                    goto next_candidate;
                }
            }
        }

        GVariant *status_var = NULL;
        if (!dbus_get_property(bus_name, "/org/mpris/MediaPlayer2",
                                "org.mpris.MediaPlayer2.Player", "PlaybackStatus", &status_var)) {
            continue;
        }
        const gchar *status_str = g_variant_get_string(status_var, NULL);
        gchar *status = status_str ? g_strdup(status_str) : NULL;
        g_variant_unref(status_var);

        if (status && g_strcmp0(status, "Stopped") == 0) {
            g_free(status);
            continue;
        }

        GVariant *ident_var = NULL;
        dbus_get_property(bus_name, "/org/mpris/MediaPlayer2",
                          "org.mpris.MediaPlayer2", "Identity", &ident_var);

        GVariant *entry_var = NULL;
        dbus_get_property(bus_name, "/org/mpris/MediaPlayer2",
                          "org.mpris.MediaPlayer2", "DesktopEntry", &entry_var);

        gboolean is_playing = status && (g_strcmp0(status, "Playing") == 0);

        /* Prefer playing over paused: select if playing and (no best yet or best is paused) */
        if (is_playing && (!best_bus || !best_is_playing)) {
            g_free(best_bus);
            g_free(best_identity);
            g_free(best_desktop_entry);
            best_bus = g_strdup(bus_name);
            best_identity = ident_var ? g_strdup(g_variant_get_string(ident_var, NULL)) : NULL;
            best_desktop_entry = entry_var ? g_strdup(g_variant_get_string(entry_var, NULL)) : NULL;
            best_is_playing = TRUE;
        } else if (!best_bus) {
            /* Fallback: first non-stopped player */
            best_bus = g_strdup(bus_name);
            best_identity = ident_var ? g_strdup(g_variant_get_string(ident_var, NULL)) : NULL;
            best_desktop_entry = entry_var ? g_strdup(g_variant_get_string(entry_var, NULL)) : NULL;
            best_is_playing = is_playing;
        }


        if (ident_var) g_variant_unref(ident_var);
        if (entry_var) g_variant_unref(entry_var);
        g_free(status);

        if (is_playing && best_bus && best_is_playing) break;

    next_candidate:
        continue;
    }

    g_ptr_array_free(candidates, TRUE);

    if (!best_bus) return FALSE;

    *out_bus_name = best_bus;
    *out_identity = best_identity;
    *out_desktop_entry = best_desktop_entry;
    return TRUE;
}

gboolean mpris_get_metadata(const gchar *bus_name, MprisMetadata **out_metadata) {
    GVariant *meta_var = NULL;
    if (!dbus_get_property(bus_name, "/org/mpris/MediaPlayer2",
                            "org.mpris.MediaPlayer2.Player", "Metadata", &meta_var)) {
        return FALSE;
    }

    MprisMetadata *md = g_new0(MprisMetadata, 1);
    md->player_name = g_strdup(bus_name);

    GVariantIter iter;
    g_variant_iter_init(&iter, meta_var);
    const gchar *key;
    GVariant *value;

    while (g_variant_iter_next(&iter, "{&sv}", &key, &value)) {
        GVariant *unwrapped = g_variant_get_variant(value);
        if (g_strcmp0(key, "xesam:title") == 0) {
            md->title = util_strdup_variant_string(unwrapped);
        } else if (g_strcmp0(key, "xesam:artist") == 0) {
            md->artists = util_strdup_variant_string_array(unwrapped);
        } else if (g_strcmp0(key, "xesam:album") == 0) {
            md->album = util_strdup_variant_string(unwrapped);
        } else if (g_strcmp0(key, "xesam:albumArtist") == 0) {
            gchar **arr = util_strdup_variant_string_array(unwrapped);
            md->album_artist = arr && arr[0] ? g_strdup(arr[0]) : NULL;
            g_strfreev(arr);
        } else if (g_strcmp0(key, "xesam:trackNumber") == 0) {
            md->track_number = g_variant_get_uint32(unwrapped);
        } else if (g_strcmp0(key, "xesam:discNumber") == 0) {
            md->disc_number = g_variant_get_uint32(unwrapped);
        } else if (g_strcmp0(key, "xesam:contentCreated") == 0) {
            const gchar *str = g_variant_get_string(unwrapped, NULL);
            md->year = util_parse_year_from_iso8601(str);
        } else if (g_strcmp0(key, "mpris:artUrl") == 0) {
            md->art_url = util_strdup_variant_string(unwrapped);
        } else if (g_strcmp0(key, "mpris:trackid") == 0) {
            md->track_id = util_strdup_variant_string(unwrapped);
        } else if (g_strcmp0(key, "mpris:length") == 0) {
            md->length_us = g_variant_get_int64(unwrapped);
        } else if (g_strcmp0(key, "xesam:url") == 0) {
            md->url = util_strdup_variant_string(unwrapped);
        }
        g_variant_unref(unwrapped);
        g_variant_unref(value);
    }
    g_variant_unref(meta_var);

    GVariant *ident_var = NULL;
    if (dbus_get_property(bus_name, "/org/mpris/MediaPlayer2",
                           "org.mpris.MediaPlayer2", "Identity", &ident_var)) {
        md->player_identity = util_strdup_variant_string(ident_var);
        g_variant_unref(ident_var);
    }

    GVariant *entry_var = NULL;
    if (dbus_get_property(bus_name, "/org/mpris/MediaPlayer2",
                           "org.mpris.MediaPlayer2", "DesktopEntry", &entry_var)) {
        md->player_desktop_entry = util_strdup_variant_string(entry_var);
        g_variant_unref(entry_var);
    }

    *out_metadata = md;
    return TRUE;
}

void mpris_metadata_free(MprisMetadata *md) {
    if (!md) return;
    g_free(md->title);
    g_strfreev(md->artists);
    g_free(md->album);
    g_free(md->album_artist);
    g_free(md->art_url);
    g_free(md->track_id);
    g_free(md->url);
    g_free(md->player_name);
    g_free(md->player_identity);
    g_free(md->player_desktop_entry);
    g_free(md);
}