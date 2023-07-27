#!/bin/bash
mkdir -p ~/.config/xkb/compat
mkdir -p ~/.config/xkb/keycodes
mkdir -p ~/.config/xkb/rules
mkdir -p ~/.config/xkb/symbols
mkdir -p ~/.config/xkb/types

ln -s symbols/xkb-customization-for-accents /home/andre/.config/xkb/symbols/custom
