# ![crwm](https://user-images.githubusercontent.com/20104594/206676265-f699dbf5-7252-4c0f-9e55-e815089ea7b9.svg)

Crwm (Column Row Window Manager, pronounced crumb) is a manual tiling window manager for X somewhat inspired by plan9's acme text editor, as well as a variety of window managers such as bspwm, i3wm, rio, dwm, and others.

The essential pitch behind crwm is that a series of columns each containing a series of rows could be a functional and efficent way of managing the tiling window space. It produces practical ways of managing visible windows, and keeps the system as a whole managable in the eyes of both the programmer and the user.

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

### Problems/caveats

- All keybindings will be lost at random (uncommonly) and only sometimes will re-enable. This is related to what programs are currently being managed by crwm, many/most never have this issue, and some more than others.
- Multiscreen support is poor, to say the least. Screens must be configured before launch, and they will be treated as one big screen, likely with some non-visible areas getting used.

## Installing

Compile using `make` or `make compile`, and then install it to /usr/local/bin by running `make install` as root.

To uninstall just remove the executable from wherever it was installed (eg: `rm /usr/local/bin/crwm`)

### Requirements
- xcb
- xcb-keysyms
- lucida sans for X, if status bar is enabled

## Thanks

- I used the [xwm](https://github.com/mcpcpc/xwm) by [Michael Czigler](https://github.com/mcpcpc) as a reference and to practice with xcb which was incredibly helpful due to the lack of documention
- [Ivan Tikhonov](https://github.com/ITikhonov) made [a program that closes windows using xcb](https://github.com/ITikhonov/wm/blob/master/wmclose.c), of which the code to do so in this project is based off of
- Thanks to [suckless.org](https://suckless.org) for having a fully featured [wm](https://dwm.suckless.org/) which works at least better than mine
- Logo made using [dotgrid](https://100r.co/site/dotgrid.html) which is made by [Hundred Rabbits](https://100r.co) aka Rek & Devine
