# crwm

This is the development/experimental branch, it is usually missing features and might not even compile.

Crwm (Column Row Window Manager, pronounced crumb) is a manual tiling window manager for X somewhat inspired by plan9's acme text editor, as well as a variety of window managers such as bspwm, i3wm, rio, dwm, and others.

The essential premise behind crwm is that a series of columns each containing a series of rows is a functional and efficent way of managing the tiling window space.
It produces practical ways of managing visible windows, and keeps the system as a whole managable in the eyes of both the programmer and the user.

## Usage

This is the development/experimental branch, there are no permenant usage instructions for the development/experimental branch.
Read the source code or something.

## Installing

Compile using `make` or `make compile`, and then install it to /usr/local/bin by running `make install` as root.

To uninstall just remove the executable from wherever it was installed (eg: `rm /usr/local/bin/crwm`)

## Thanks

- I used the [xwm](https://github.com/mcpcpc/xwm) by [Michael Czigler](https://github.com/mcpcpc) as a reference which was incredibly helpful due to the lack of documention on xcb
- Thanks to [Ivan Tikhonov](https://github.com/ITikhonov) for their [program that closes windows with xcb](https://github.com/ITikhonov/wm/blob/master/wmclose.c), of which the code to do so is based off
