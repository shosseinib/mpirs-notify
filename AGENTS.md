# mpirs-notify

`mpirs-notify` is a one-shot GNU/Linux C11 CLI that selects an MPRIS 2 player over the user's D-Bus session bus and sends a now-playing desktop notification via libnotify. The executable and configuration paths use `mpirs`, even though the protocol is `MPRIS`.

## Build and verify

Use a C11 compiler, Meson, Ninja, and pkg-config. Meson requires development files for `dbus-1`, `libnotify`, `glib-2.0`, `gio-2.0`, and `gdk-pixbuf-2.0`.

From the project root (the README adds `--prefix "$HOME/.local"` for user installs):

```sh
meson setup build
meson compile -C build
```

Run setup for a new build directory; compile again after source changes. The output is `build/mpirs-notify`. `build/build.ninja` and `build/compile_commands.json` are generated; edit `meson.build` to change the source list, dependencies, or flags. The build enables Meson warning level 3 without treating warnings as errors. There are no registered Meson tests or configured lint or CI workflows; `test_config/` contains INI samples, not a test runner.

The README documents these manual checks. Make sure `mpirs-notify` on `PATH` is the build you intend to check, not an older install:

- `mpirs-notify --version` checks the binary without D-Bus.
- `mpirs-notify --list-players` lists raw MPRIS bus names; it does not apply the ignore list or check playback state.
- `mpirs-notify --print-metadata` checks player selection and metadata without sending a notification.
- `mpirs-notify` checks the full path, including notification delivery. Use a desktop session with a notification service and an MPRIS player playing a track. The player checks can fail in headless shells or when no eligible player is running.

## Source layout and conventions

- `src/main.c` handles CLI options and orchestrates discovery, metadata, artwork, icon resolution, and notification. `src/dbus.c` uses libdbus for discovery and property reads, then converts values to GLib `GVariant`s. `src/metadata.c` formats and validates the resulting track data.
- `src/art.c` fetches artwork with GIO or decodes data URIs, then loads it with GdkPixbuf. `src/player_icon.c` resolves player icons; `src/notify.c` builds and sends the libnotify payload. `src/config.c` loads INI settings with `GKeyFile`.
- Shared types and declarations live in `include/mpirs-notify.h`. Use the existing four-space indentation, same-line braces, module-prefixed `snake_case` functions, and `static` helpers for file-local code. Result-producing APIs such as `mpris_get_metadata` and `art_fetch` return `gboolean` and fill `out_` parameters on success; their results have matching cleanup functions.

## Pitfalls

- `config_load()` reads only `$XDG_CONFIG_HOME/mpirs-notify/config.ini` (normally `~/.config/mpirs-notify/config.ini`). Meson's installed `xdg/mpirs-notify.conf` is a reference copy, not a runtime fallback. To use the sample under `test_config/`, point `XDG_CONFIG_HOME` at the absolute path to `test_config/`; the nested `mpirs-notify/config.ini` is the one read.
- `ignore_players` uses semicolons, as in `test_config/mpirs-notify/config.ini`. Discovery matches each item as a substring of the full bus name. It skips stopped players and prefers playing over paused regardless of `prefer_playing`.
- In the notification path, `config_load()` supplies a positive timeout even when there is no INI file, so it currently overrides `--timeout`. The legacy keys `show_year`, `show_album`, `truncate_chars`, and `fallback_icon` are parsed but not applied by the notification path.
- The artwork lookup table is process-local; downloaded and decoded temporary files are removed on cleanup, and `cache_max_items` is not enforced. Do not expect repeat invocations to reuse downloads.
