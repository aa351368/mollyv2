# molly

Linux tools for the "Big Red Button" USB device manufactured by Dream Cheeky.

![Big Red Button](big-red-button.png)

Allows running arbitrary shell scripts in response to open, close and button press events.

## Build

The project uses CMake.

    cmake .
    make

## Configure
UDEV rule for the device (inspiration from https://github.com/drewnoakes/molly): 

~~~
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="1d34", ATTRS{idProduct}=="000d", MODE="0664", GROUP="plugdev", SYMLINK+="big_red_button"
~~~

This udev rule identifies it better than the original code did

Put into something like 
/etc/udev/rules.d/50-big-red-button.rules

Reload the rules:
~~~
 sudo udevadm control --reload-rules
~~~
Verify it is working by unplug / replug and 

ls -l /dev/big_red_button

## Test

Plug in the button (if already plugged in, unplug then replug it) and check that `/dev/big_red_button` exists.

Test your button is working via the `molly-test` command. It will log all state transitions.

    $ ./molly-test
    Closed from Unknown
    OPEN...
    PRESS!!!
    Closed from LidOpen

## Daemon

The project includes `mollyd`, a Linux daemon process that runs silently in the background.

> Currently this daemon's behaviour is hard coded. Future changes will make this configurable, and hence more broadly useful.

## TODO

Currently the basic functionality is there but some settings are hard coded. Some work is needed to make this utility broadly useful.

It would also be good to have this daemon start and stop in response to the USB device being added and removed, avoiding the need to poll.
