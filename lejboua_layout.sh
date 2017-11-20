#!/bin/bash

xkbcomp -I$HOME/projs/personal/xkb_spike $HOME/projs/personal/xkb_spike/my_map :0

xcape -t 140 -e 'Hyper_R=space;Shift_L=Escape;Shift_R=Delete'
