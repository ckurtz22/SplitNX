# SplitNX
LiveSplit Server plugin for autosplitting on the Nintendo Switch!

Simply `make dist` and add the files within the dist folder to your SD card. Must run with CFW (Use Atmosphere or Kosmos or it will probably not work).
Includes both basic autosplitting from memory and manual splitting with controller.
All controls require you to hold all 4 shoulder buttons then press:
A - Split/Start
B - Undo Split
X - Skip Split
Y - Reset

The autosplitting is read in from `/split/splitter.txt` on your SD card which has the following format:

First line: IP of computer

Second line: Port of LiveSplit Server

Every line after that will correspond to a memory autosplit corresponding with the index of the split in LiveSplit (so the 3rd line will describe autosplitting for the first split, and so on):

[Offset Address from Heap start] [operator] [size] [value]

Example:
`0x61BC93B6 >= u32 1001` will check whether or not the unsigned 32 bit integer at HEAP + 0x61BC93B6 is greater or equal to the value of 1001. If so it will split for that split index.


This is super rough, just wanted to get something working for now, and I may or may not improve it later. If anyone wants to improve it then by all means go for it. Would be great to get it to hook into the already implemented AutoSplitters for LiveSplit but I have no clue how to do that.

Any questions feel free to ask.
