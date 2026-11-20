# MPRIS Notify — C Implementation Proposal

## Problem Statement

Create a fast, zero-dependency C program that:
1. Queries the currently playing media via MPRIS (D-Bus)
2. Extracts: artist, title, year, album, album art
3. Displays a desktop notification with the track info and album cover
4. Shows which player is playing (using player icon if available)
5. Triggered by a keyboard shortcut (Fn + media key)

## Architecture Overview

```
+------------------------------ mpirs-notify ------------------------------+
|  1. D-Bus Client (libdbus-1)                                            |
|     +- Query org.mpris.MediaPlayer2.*.Player.Metadata                  |
|     +- Query org.mpris.MediaPlayer2 Identity, DesktopEntry             |
|  2. Metadata Parser                                                     |
|     +- Extract xesam:title, xesam:artist, xesam:album,                |
|         xesam:contentCreated (year), mpris:artUrl, mpris:trackid      |
|  3. Art Fetcher                                                         |
|     +- Download file:// / http:// / https:// artUrl to /tmp            |
|  4. Player Icon Resolver                                                |
|     +- Use DesktopEntry to load themed icon, fallback to generic       |
|  5. Notification Sender (libnotify)                                    |
|     +- notify_notification_new() + set_image_from_pixbuf()             |
+--------------------------------------------------------------------------+
```

## Dependencies (Minimal, System-Standard)

| Library | Purpose | Ubiquity |
|---------|---------|----------|
| libdbus-1 | D-Bus communication | All GNU/Linux desktops |
| libnotify | Desktop notifications | All freedesktop.org desktops |
| glib-2.0 | Variant parsing, file I/O, main loop | All GNU/Linux desktops |
| gio-2.0 | File download, async I/O | All GNU/Linux desktops |
| gdk-pixbuf-2.0 | Image loading for notification | All GTK desktops |

No external downloads, no pkg-config packages beyond standard desktop stack.

## MPRIS Metadata Fields (Per freedesktop.org Specification)

Source: MPRIS v2 Metadata Guidelines (https://www.freedesktop.org/wiki/Specifications/mpris-spec/metadata/)

### MPRIS-Specific Fields (prefix mpris:)

| Field | D-Bus Key | Type | Description |
|-------|-----------|------|-------------|
| Track ID | mpris:trackid | object_path (string) | Unique identity for this track |
| Length | mpris:length | int64 (microseconds) | Track duration in microseconds |
| Art URL | mpris:artUrl | string (URI) | Location of album art image |

### Common XESAM Properties (prefix xesam:)

| Field | D-Bus Key | Type | Description |
|-------|-----------|------|-------------|
| Title | xesam:title | string | Track title |
| Artist | xesam:artist | array of strings (as) | Track artist(s) |
| Album Artist | xesam:albumArtist | array of strings (as) | Album artist(s) |
| Album | xesam:album | string | Album name |
| Track Number | xesam:trackNumber | integer | Track number on album disc |
| Disc Number | xesam:discNumber | integer | Disc number on album |
| Year | xesam:contentCreated | string (ISO 8601) | When track was created (year component) |
| Genre | xesam:genre | array of strings (as) | Genre(s) |
| Composer | xesam:composer | array of strings (as) | Composer(s) |
| URL | xesam:url | string (URI) | Location of media file |
| BPM | xesam:audioBPM | integer | Beats per minute |
| Lyrics | xesam:asText | string | Track lyrics |
| Comment | xesam:comment | array of strings (as) | Freeform comments |
| Use Count | xesam:useCount | integer | Play count |
| User Rating | xesam:userRating | float (0.0-1.0) | User rating |
| Auto Rating | xesam:autoRating | float (0.0-1.0) | Auto-generated rating |
| Lyricist | xesam:lyricist | array of strings (as) | Lyricist(s) |
| First Used | xesam:firstUsed | string (ISO 8601) | First play date |
| Last Used | xesam:lastUsed | string (ISO 8601) | Last play date |

Key for this project: xesam:title, xesam:artist, xesam:album, xesam:contentCreated (year), mpris:artUrl

## Player Discovery Strategy

1. List all session bus names matching org.mpris.MediaPlayer2.*
2. Filter for active players - query PlaybackStatus != "Stopped"
3. Prefer player with PlaybackStatus == "Playing"
4. Secondary: most recently active (PropertiesChanged timestamp tracking)
5. Skip players in ignore_players config list
6. Fallback: first available with CanControl == true

## Player Identification and Icon

Each MPRIS player exposes:
- Identity property (string, e.g. "VLC media player", "Spotify")
- DesktopEntry property (string, e.g. "vlc", "spotify")

Icon resolution:
1. Use DesktopEntry as icon name in freedesktop.org icon theme
2. If themed icon not found, try lowercase Identity as fallback
3. Final fallback: generic "audio-x-generic" or "media-optical"

The player name/icon appears in notification as:
- App name: player Identity (via notify_notification_set_app_name)
- App icon: resolved icon name (via notify_notification_set_app_icon)

## Art Handling

| Art URL Scheme | Handling |
|----------------|----------|
| file:// | Use directly (load via gdk_pixbuf_new_from_file) |
| http:// / https:// | Download to /tmp/mpirs-notify-XXXXXX.jpg via g_file_download |
| data: | Decode base64, write to temp file, load via gdk-pixbuf |
| Missing | No album art image; notification shows player icon only |

Cache: Keep last N art files in /tmp with LRU eviction to avoid re-downloads.

## Notification Format (Per Desktop Notifications Spec v1.3)

```
+------------------------------------------------------+
| [player icon]  VLC media player                       |
| Artist Name - Track Title                             |
| 1975 - Album Name                                     |
| [album art thumbnail]                                 |
+------------------------------------------------------+
```

- App name: player Identity (e.g. "VLC media player", "Spotify")
- App icon: resolved from DesktopEntry (themed icon)
- Summary: "Artist - Title" (truncated to 128 chars, dash separator)
- Body: "Year - Album" (year omitted if unknown; dash separator)
- Image: album art via notify_notification_set_image_from_pixbuf (GdkPixbuf)
- Timeout: 5000 ms (configurable)
- Urgency: low (NOTIFY_URGENCY_LOW)
- Category: x-mpris.now-playing

Per spec: Image priority is image-data > image-path > app_icon. We use image-data via GdkPixbuf for best quality. App icon shows player identity.

## Keyboard Integration

### Option A: Window Manager / Desktop Environment Binding (Recommended)

| DE/WM | Binding Method |
|-------|----------------|
| KDE Plasma | System Settings - Shortcuts - Custom Shortcut - mpirs-notify |
| GNOME | Settings - Keyboard - Shortcuts - Custom - mpirs-notify |
| Sway/i3 | bindsym XF86AudioMedia exec mpirs-notify |
| Hyprland | bind = , XF86AudioMedia, exec, mpirs-notify |
| X11 (xbindkeys) | "mpirs-notify"  XF86AudioMedia |

Key codes: XF86AudioMedia (often Fn+media), XF86AudioPlay, XF86AudioNext, XF86AudioPrev

### Option B: systemd User Service (for daemon mode later)

```ini
# ~/.config/systemd/user/mpirs-notify.service
[Unit]
Description=MPRIS Now-Playing Notifier
[Service]
Type=oneshot
ExecStart=/usr/local/bin/mpirs-notify
[Install]
WantedBy=default.target
```

## C Implementation Plan

### File Structure

```
mpirs-notify/
├── src/
│   ├── main.c           # Entry point, argument parsing
│   ├── dbus.c           # D-Bus connection, player discovery
│   ├── metadata.c       # Metadata extraction, parsing
│   ├── art.c            # Art fetching, caching, GdkPixbuf loading
│   ├── player_icon.c    # Player icon resolution from DesktopEntry
│   ├── notify.c         # libnotify wrapper
│   └── util.c           # String helpers, logging
├── include/
│   └── mpirs-notify.h   # Shared types, constants
├── meson.build          # Build system
└── README.md
```

### Core Data Structures

```c
// include/mpirs-notify.h
#ifndef MPIRS_NOTIFY_H
#define MPIRS_NOTIFY_H

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

typedef struct {
    gchar *title;              // xesam:title
    gchar **artists;           // xesam:artist (NULL-terminated array)
    gchar *album;              // xesam:album
    gchar *album_artist;       // xesam:albumArtist (first entry)
    guint32 year;              // parsed from xesam:contentCreated (ISO 8601)
    guint32 track_number;      // xesam:trackNumber
    guint32 disc_number;       // xesam:discNumber
    gchar *art_url;            // mpris:artUrl
    gchar *track_id;           // mpris:trackid
    gint64 length_us;          // mpris:length
    gchar *url;                // xesam:url
    gchar *player_name;        // bus name (e.g. "org.mpris.MediaPlayer2.spotify")
    gchar *player_identity;    // Identity property
    gchar *player_desktop_entry; // DesktopEntry property
} MprisMetadata;

typedef struct {
    GdkPixbuf *pixbuf;         // Loaded album art (owned)
    gchar *cached_path;        // Local path to art file (if downloaded)
    gboolean is_temp;          // TRUE if we downloaded it (cleanup on exit)
} ArtResult;

typedef struct {
    gchar *icon_name;          // Icon name for themed lookup
    GdkPixbuf *pixbuf;         // Loaded icon pixbuf (optional, for fallback)
} PlayerIcon;

// Public API
gboolean mpris_discover_player(gchar **out_bus_name, gchar **out_identity);
gboolean mpris_get_metadata(const gchar *bus_name, MprisMetadata **out_metadata);
void mpris_metadata_free(MprisMetadata *md);

gboolean art_fetch(const gchar *art_url, ArtResult **out_result);
void art_result_free(ArtResult *ar);

gboolean player_icon_resolve(const MprisMetadata *md, PlayerIcon **out_icon);
void player_icon_free(PlayerIcon *pi);

gboolean notify_show(const MprisMetadata *md, const ArtResult *art,
                     const PlayerIcon *player_icon, gint timeout_ms);

// Config
typedef struct {
    gint timeout_ms;
    gboolean show_year;
    gboolean show_album;
    gsize truncate_chars;
    gchar *cache_dir;
    gsize cache_max_items;
    gchar *fallback_icon;
    gchar **ignore_players;    // NULL-terminated array
} Config;

Config *config_load(void);
void config_free(Config *cfg);

#endif
```

### D-Bus Implementation (src/dbus.c)

Key functions:
1. dbus_connect_session() -> DBusConnection*
2. dbus_list_mpris_players() -> GList* of bus names
3. dbus_get_playback_status(bus_name) -> "Playing"/"Paused"/"Stopped"
4. dbus_get_metadata(bus_name) -> GVariant* (raw Metadata dict a{sv})
5. dbus_get_player_identity(bus_name) -> string
6. dbus_get_player_desktop_entry(bus_name) -> string

Player Selection Algorithm:
```c
// Priority order:
// 1. PlaybackStatus == "Playing"
// 2. Most recently active (PropertiesChanged timestamp tracking)
// 3. First available with CanControl == true
// 4. Skip players in ignore_players config list
```

### Metadata Parsing (src/metadata.c)

GVariant dict parsing (a{sv}):
- xesam:title -> string
- xesam:artist -> array of strings (as) -> take first, join with ", "
- xesam:albumArtist -> array of strings (as) -> take first
- xesam:album -> string
- xesam:trackNumber -> uint32
- xesam:discNumber -> uint32
- xesam:contentCreated -> string (ISO 8601) -> parse year (first 4 digits)
- mpris:artUrl -> string
- mpris:trackid -> object_path (string)
- mpris:length -> int64
- xesam:url -> string
- xesam:genre -> array of strings

### Art Fetching (src/art.c)

- file:// -> strip prefix, gdk_pixbuf_new_from_file()
- http(s):// -> g_file_download() to /tmp/mpirs-notify-XXXXXX.jpg, then load
- data: -> base64 decode, write to temp file, load via gdk_pixbuf_new_from_file()
- Cache: hash art_url -> cached_path (GHashTable)
- Return ArtResult with GdkPixbuf (ref'd) and path

### Player Icon Resolution (src/player_icon.c)

```c
// 1. Try DesktopEntry as icon theme name (e.g. "vlc", "spotify")
// 2. Try gdk_pixbuf_new_from_file_if_exists for common icon paths
// 3. Fallback: "audio-x-generic" or "media-optical"
// 4. Return PlayerIcon with icon_name (for notify) and optional pixbuf
```

### Notification (src/notify.c)

```c
// notify_init("mpirs-notify")
// NotifyNotification *n = notify_notification_new(summary, body, player_icon->icon_name)
// notify_notification_set_app_name(n, player_identity)
// if (player_icon->pixbuf) notify_notification_set_app_icon_from_pixbuf(n, player_icon->pixbuf)
// notify_notification_set_urgency(n, NOTIFY_URGENCY_LOW)
// notify_notification_set_timeout(n, timeout_ms)
// notify_notification_set_category(n, "x-mpris.now-playing")
// if (art && art->pixbuf) notify_notification_set_image_from_pixbuf(n, art->pixbuf)
// notify_notification_show(n, &error)
// g_object_unref(n)
// notify_uninit()
```

## Build System (meson.build)

```meson
project('mpirs-notify', 'c',
  default_options: ['warning_level=3', 'werror=false', 'c_std=c11',
                    'optimization=2', 'strip=true'])

# Dependencies
dbus_dep = dependency('dbus-1', required: true)
notify_dep = dependency('libnotify', required: true)
glib_dep = dependency('glib-2.0', required: true)
gio_dep = dependency('gio-2.0', required: true)
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', required: true)

# Version
version = '0.1.0'
meson.add_global_arguments('-DVERSION="@0@"'.format(version), language: 'c')

# Executable
executable('mpirs-notify',
  'src/main.c', 'src/dbus.c', 'src/metadata.c',
  'src/art.c', 'src/player_icon.c', 'src/notify.c', 'src/util.c',
  dependencies: [dbus_dep, notify_dep, glib_dep, gio_dep, gdk_pixbuf_dep],
  install: true)

# Install config example
install_data('data/mpirs-notify.conf.example',
  install_dir: join_paths(get_option('sysconfdir'), 'xdg'),
  rename: 'mpirs-notify.conf')

# Install desktop file for autostart (optional)
install_data('data/mpirs-notify.desktop',
  install_dir: join_paths(get_option('datadir'), 'applications'))
```

## Performance Characteristics

| Metric | Target | Notes |
|--------|--------|-------|
| Startup latency | < 50 ms | Cold start, no daemon |
| D-Bus round-trip | < 10 ms | Local session bus |
| Art load (cached) | < 5 ms | File exists, gdk-pixbuf |
| Art download (network) | < 500 ms | Depends on network |
| Memory footprint | < 2 MB RSS | No heavy runtime |
| Binary size | ~150 KB | Stripped, optimized (-Os) |

## Configuration (INI-style, $XDG_CONFIG_HOME/mpirs-notify/config.ini)

```ini
[notification]
timeout_ms = 5000
urgency = low
show_year = true
show_album = true
truncate_chars = 128

[art]
cache_dir = /tmp/mpirs-notify-art
cache_max_items = 20
fallback_icon = audio-x-generic

[player]
prefer_playing = true
ignore_players = firefox,chrome,brave,chromium,plasma-browser-integration
```

## Installation

```bash
# Build
meson setup build
meson compile -C build

# Install (user)
meson install -C build --prefix ~/.local

# Or system-wide (requires root)
sudo meson install -C build
```

## Usage

```bash
# Show notification for currently playing track
mpirs-notify

# With custom timeout
mpirs-notify --timeout 3000

# List available MPRIS players (debug)
mpirs-notify --list-players

# Print metadata to stdout (debug)
mpirs-notify --print-metadata

# Version
mpirs-notify --version
```

## Testing Checklist

- [ ] Spotify (native + flatpak + snap)
- [ ] VLC
- [ ] mpv + mpris plugin (mpv-mpris)
- [ ] Rhythmbox
- [ ] Lollypop
- [ ] Firefox/Chrome (YouTube, SoundCloud, Bandcamp)
- [ ] KDE Plasma browser integration
- [ ] Audacious
- [ ] Clementine
- [ ] Sayonara
- [ ] No player running (graceful exit, return code 1)
- [ ] Multiple players (selects playing one)
- [ ] Missing artUrl (player icon only)
- [ ] http/https artUrl (download + display works)
- [ ] file:// artUrl (direct load works)
- [ ] data: artUrl (decode + display works)
- [ ] Long artist/title truncation
- [ ] Special characters (UTF-8)
- [ ] Year = 0 / missing contentCreated (omit from body)
- [ ] Album missing (show only year)
- [ ] Config file loading
- [ ] Player icon resolution (DesktopEntry -> themed icon)

## Future Enhancements

1. Daemon mode - listen for PropertiesChanged signals, show on track change
2. Wayland protocol - use org.kde.kwin.Thumbnailer for better art
3. Album art as action button - click to open player/URL
4. History - log played tracks to file (~/.local/share/mpirs-notify/history.jsonl)
5. Multiple output backends - libnotify, dunstify, mako, swaync, notify-send
6. MPRIS control - --play-pause, --next, --prev flags

## Why This Approach

| Requirement | Solution |
|-------------|----------|
| Fast | No daemon, direct D-Bus call, minimal allocations, -Os |
| Zero deps | Only standard desktop libraries (dbus, glib, notify, gdk-pixbuf) |
| Keyboard-triggered | Oneshot binary, binds to any key via WM/DE |
| Album art | Supports file://, http://, https://, data: via gdk-pixbuf |
| Year + Album | XESAM xesam:contentCreated + xesam:album (widely supported) |
| Player identity | Identity + DesktopEntry properties, themed icon lookup |
| C language | No runtime, small binary, full control, portable |

## References (Official Specifications)

- MPRIS 2.2 Specification (https://specifications.freedesktop.org/mpris/latest/)
- MPRIS v2 Metadata Guidelines (https://www.freedesktop.org/wiki/Specifications/mpris-spec/metadata/)
- Desktop Notifications Specification v1.3 (https://specifications.freedesktop.org/notification-spec/latest/)
- D-Bus C API (https://dbus.freedesktop.org/doc/api/)
- libnotify C API (https://developer.gnome.org/libnotify/)
- GIO File Download (https://docs.gtk.org/gio/method.File.download.html)
- GdkPixbuf API (https://docs.gtk.org/gdk-pixbuf/)

---

Next Step: Approve this proposal and I will implement the complete project with meson.build, all src/*.c files, and installation instructions.