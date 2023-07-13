# crwm
crwm (Column Row Window Manager, pronounced crumb) is
a manual tiling window manager for X somewhat inspired by
plan9's acme text editor, as well as a variety of window
managers such as bspwm, dwm, i3wm, and rio.

It was made in part due to disatisfaction with the lack of layout
flexibility in dwm, while most alternatives offered too much
flexibility, more than what seemed necessary. 

## Usage
Starting the window manager is done by running `crwm`.
If you use an `.xinitrc` file, it would be added to there.
Window operations are issued through an seperate program,
`crwmctl`. To see some examples of how to use crwmctl,
see `crwmkeys.c`.

In normal use, these commands aren't entered
manually but instead via a hotkey manager. A sane
default is provided in the form of `crwmkeys`, but any
hotkey manager or equivalent will work, such as sxhkd.
crwmkeys should be run at the same time that crwm is.

When a new window is opened, it is opened in the column
to the right of the focused window. If there are no columns
to the right of the focused window or there is no focused
window, a new column is created.

This is also true of moving windows left and right, while
moving windows up and down only reorders them within the
same column. The screen's height and width are shared between
the rows and columns according to their weight, where a row or
column with more weight gets more real estate allocated to it.

### Hotkeys
The default hotkeys for crwmkeys are pretty simple.
The default modifier key is windows/command key.

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

### Options
Configuring the various visual settings (padding, margins, borders,
etc) and some non-visual settings, is done using command line
options provided to `crwm`.

| Option | Default Parameter | Action |
| --- | --- | --- |
| `-h` | none | Prints a help message display the possible command line options.
| `-t` | `"123456789"` | Sets the number of tables and their labels. Each character represents a single table and its label.
| `-f` | `/tmp/crwm.d/$DISPLAY` | Changes the file that crwm reads window operations from that crwmctl has sent.
| `-p` | `0` | Sets the padding size between windows.
| `-m` | `0` | Sets the margin size for all sides.
| `-u,d,l,r` | `0` | Sets the margin size for the top, bottom, left, right side.
| `-s` | `#9eeeee` | Sets the border colour around the focused window
| `-n` | `#55aaaa` | Sets the border colour around unfocused windows
| `-b` | `1` | Sets the border thickness of all windows.


## Installing
Compile using `make` and then install it to /usr/local/bin
by running `make install` as root. It can be uninstalled with
`make uninstall`, also run as root. `make clean` will remove
the compiled files from the local directory.

### Requirements
- xcb (for crwm and crwmkeys)
- xcb-randr (for crwm)
- xcb-keysyms (for crwmkeys)

## Attributions
- I used the [xcb window manager](https://github.com/mcpcpc/xwm)
by [Michael Czigler](https://github.com/mcpcpc) as a reference
and to practice with xcb, which was incredibly helpful due to
the lack of accessible explicit documention
- [Ivan Tikhonov](https://github.com/ITikhonov) made 
[a program that closes windows using xcb]
(https://github.com/ITikhonov/wm/blob/master/wmclose.c),
of which the code to do so in this project is based off of
- Thanks to [suckless.org](https://suckless.org) for having
a fully featured [wm](https://dwm.suckless.org/) which
contains functioning examples of how to manage windows
