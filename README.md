# SplitNX #

LiveSplit Server plugin for autosplitting on the Nintendo Switch!

## Usage ##

### Splitting With a Controller ###

This is the easiest way to use splitting for newcomers, and is always on.
Certain button combinations will trigger splits for livesplit. These
key-combos currently can not be changed.

To trigger hold all 4 "Shoulder Buttons" (ZL/L/R/ZR), and then press one of:

  - `+`: Attempt to connect to livesplit server
  - `-`: Reload configuration file.
  - `A`: Split/Start
  - `B`: Undo
  - `X`: Skip
  - `Y`: Reset
  - Left on the DPAD: write first memory address value in autosplitter to log file.

### Splitting Based on Memory Values ###

Similar to how autosplit scripts work it is possible to automatically split
based on the condition of a piece of memory. The format is described below,
but we don't describe how to identify these values as that's highly dependent
on each game.

TODO(xxx): document file which just contains memory autosplit.

<!--
[Offset Address from Heap start] [operator] [size] [value]

Example:
`0x61BC93B6 >= u32 1001` will check whether or not the unsigned 32 bit integer at HEAP + 0x61BC93B6 is greater or equal to the value of 1001. If so it will split for that split index.

Hook in auto-splitters?
-->

## Building ##

### SysModule ###

The sysmodule is the actual thing always running on your switch, listening for
keypresses, and communicating with LiveSplit.

Inside the `sysmodule` directory:

1. Ensure you have `switch-mpg123` installed: `dkp-pacman -S switch-mpg123`
2. Run `make dist`.
3. Copy the files within the `dist` folder to your SD Card.
4. Boot up your switch with CFW (Atmosphere/Kosmos).