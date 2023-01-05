# SplitNX
LiveSplit Server plugin for autosplitting on the Nintendo Switch!

Simply `make` and then move the SplitNX.ovl file to your `switch/.overlays` directory on your SD card. Make sure to install Tesla as well, you can find instructions [here](https://gbatemp.net/threads/tesla-the-nintendo-switch-overlay-menu.557362/).

Includes both basic autosplitting from memory and manual splitting with controller.
All controls require you to hold ZR + R (ZL and L must not be pressed as well!), and then press:
A - Split/Start
Y - Undo Split
X - Skip Split
Minus - Go back to options screen
Plus - Toggle the GUI for LiveSplit time/split name

The autosplitting is read in from `/split/splitter.txt` on your SD card which has the following format:

First line: IP of computer

Second line: Port of LiveSplit Server (typically 16384)

Every line after that will correspond to a memory autosplit corresponding with the index of the split in LiveSplit (so the 3rd line will describe autosplitting for the first split, and so on):

[Memory region type i.e. `heap` or `main`] [Offset Address from Heap start] [operator] [size in bits, multiple of 8] [value to compare with]

Example:
`heap 0x61BC93B6 >= 32 1001` will check whether or not the 32 bit integer at HEAP + 0x61BC93B6 is greater or equal to the value of 1001. If so it will split for that split index.

Any questions feel free to ask.

Example of it running: https://youtu.be/8Q5mv84v8-g