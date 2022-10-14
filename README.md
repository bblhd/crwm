# crwm

Crwm (pronounced crumb) is a tiling window manager for X somewhat inspired by plan9's acme text editor, as well as a variety of window managers such as bspwm, i3wm, rio, dwm, and others.

The essential concept behind crwm is that a series of columns each containing a series of rows is a functional and efficent way of managing the tiling window space. It produces practical ways of managing visible windows, and keeps the system as a whole managable in the eyes of both the programmer and the user.

## Usage

To make crwm your default window manager, replace the command to run your window manager in your `.xinitrc` file with `crwm`.

The default keybindings for crwm are pretty simple.

- `mod + q`: closes focused window
- `mod + shift + q`: quits window manager
- `mod + d`: opens dmenu_run
- `mod + enter`: opens terminal
- `mod + up|down|left|right`: moves focused window up|down|left|right

## Building

Compile using `make` or `make compile`, and then install it to /usr/local/bin by running `make install` as root.

## Thanks

- I used the [xwm](https://github.com/mcpcpc/xwm) by [Michael Czigler](https://github.com/mcpcpc) as a reference which was incredibly helpful due to the lack of documention on xcb
