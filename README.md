# mpirs-notify

Show a desktop notification for the current track from an MPRIS media player. The notification includes the player, artist, title, and any available album details and cover art. No background service is needed.

## Build and run

On Linux, build with a C11 compiler, Meson, Ninja, and pkg-config. Install the development files for `dbus-1`, `libnotify`, `glib-2.0`, `gio-2.0`, and `gdk-pixbuf-2.0`.

```sh
meson setup build --prefix "$HOME/.local"
meson compile -C build
./build/mpirs-notify
```

Run from your desktop session with an MPRIS player playing. To install for your user, run `meson install -C build`. If `build` was already configured with a different prefix, first run `meson configure build --prefix "$HOME/.local"`. Bind `~/.local/bin/mpirs-notify` to a desktop shortcut for one-key access.

After installation, use `mpirs-notify --list-players` to see which players are on the session bus, or `mpirs-notify --print-metadata` to inspect the selected track without sending a notification. Run `mpirs-notify --version` to check the installed version.

## Configure

Copy [the sample config](data/mpirs-notify.conf.example) to `${XDG_CONFIG_HOME:-$HOME/.config}/mpirs-notify/config.ini`. Set `[notification]` → `timeout_ms` to change how long the notification is displayed. To ignore players, set `[player]` → `ignore_players` to a semicolon-separated list of bus-name fragments, such as `firefox;chromium`.

## License

[GNU GPL version 3 or later](LICENSE) (`GPL-3.0-or-later`).
