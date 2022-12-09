# ![Crwm](https://user-images.githubusercontent.com/20104594/206655715-e9bb4c1d-16bd-46d0-935b-863a32f4071a.svg)

Crwm (Column Row Window Manager, pronounced crumb) is a manual tiling window manager for X somewhat inspired by plan9's acme text editor, as well as a variety of window managers such as bspwm, i3wm, rio, dwm, and others.

The essential pitch behind crwm is that a series of columns each containing a series of rows is a functional and efficent way of managing the tiling window space. It produces practical ways of managing visible windows, and keeps the system as a whole managable in the eyes of both the programmer and the user.

## Usage

To make crwm your default window manager, replace the command to run your window manager in your `.xinitrc` file with `crwm`.

The default keybindings for crwm are pretty simple. Default modifier key is meta/windows/command key.

| Keys | Action |
| --- | --- |
| `mod + q` | closes focused window
| `mod + shift + q` | quits window manager
| `mod + d` | opens launcher (default: `dmenu_run`)
| `mod + enter` | opens terminal (default: `st`)
| `mod + [up,down,left,right]` | moves focus [up,down,left,right]
| `mod + shift + [up,down,left,right]` | moves focused window [up,down,left,right]
| `mod + [1-9]` | moves to page [1-9]
| `mod + shift + [1-9]` | moves focused window to page [1-9]
| `mod + [c,v]` | shrinks focused window [horizontally,vertically]
| `mod + shift + [c,v]` | grows focused window [horizontally,vertically]

All configuration is currently done by editing the source code, although this may be subject to change.

## Installing

Compile using `make` or `make compile`, and then install it to /usr/local/bin by running `make install` as root.

To uninstall just remove the executable from wherever it was installed (eg: `rm /usr/local/bin/crwm`)

## Thanks

- I used the [xwm](https://github.com/mcpcpc/xwm) by [Michael Czigler](https://github.com/mcpcpc) as a reference which was incredibly helpful due to the lack of documention on xcb
- Thanks to [Ivan Tikhonov](https://github.com/ITikhonov) for their [program that closes windows with xcb](https://github.com/ITikhonov/wm/blob/master/wmclose.c), of which the code to do so is based off
- Thanks to [suckless.org](suckless.org) for having a fully featured wm which works at least better than mine
- Logo made using [dotgrid](https://100r.co/site/dotgrid.html) made by [Hundred Rabbits](https://100r.co) aka Rek & Devine
