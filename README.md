NOTES
=====

`my.xkb` is the full "dumped" xkb configuration with:

 - xkbcomp :0 my.xkb (:0 is the X $DISPLAY)

 To apply it (after changing it):

 - xkbcomp my.xkb :0

 This is ugly since we have the whole dump of the configuration. Read here (https://a3nm.net/blog/xkbcomp.html) that we can just have the changes we made to a specific and existing layout.

First created the `my_map` with:

 - setxkbmap -layout us -variant altgr-intl -model pc105 -option caps:ctrl_modifier -print > my_map

Then created the `symbols` folder and the `accents` config file (it can have any name) and added my changes to it (to have immediate backtick, tilde and circumflex on AltGr+T|G|B, respectively).

Finally, added the `+accents(accents)` to the `xkb_symbols` entry on the `my_map` file.

To apply it use:

 - xkbcomp -I$HOME/xkb_spike ~/xkb_spike/my_map :0 (:0 is the X $DISPLAY)

To reset the keyboard layout:

 - setxkbmap -layout us -variant altgr-intl -model pc105 -option caps:ctrl_modifier
