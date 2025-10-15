# molly

I made these changes in an afternoon and relied heaivly on LLMs, it works for me but run at your own risk.

Linux tools for the "Big Red Button" USB device manufactured by Dream Cheeky, with updates.

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

This udev rule identifies it, where as the original version did not for me. Watch dmesg to see if it matches for you

Put into something like:
/etc/udev/rules.d/50-big-red-button.rules

Reload the rules:
~~~
 sudo udevadm control --reload-rules
~~~
Verify it is working by unplug / replug: 

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

The project includes `mollyd`, a daemon process that runs in the background. By default, it logs button events to syslog. It can be configured to run any command in response to events.

### Configuration

The daemon loads configuration with the following priority (from lowest to highest):
1.  Values from `/etc/mollyd.conf`.
2.  Values from a custom config file specified with `--config`.
3.  Command-line arguments (e.g., `--device`).

**Example `/etc/mollyd.conf`:**
```ini
# Molly Daemon Configuration
device = /dev/big_red_button

# Example: Simulate pressing the 'Enter' key. Requires 'xdotool' to be installed.
on_press = xdotool key Return

# Do nothing when the lid is opened or closed.
on_open = 
on_close = 
poll_interval_ms = 50 # How often to check the device state
```

### Systemd Service

The service will exit if the button is not detected, but is setup to restart always so it will work again pretty quickly once it's plugged back in.

1.  **Install the daemon:**
    ```sh
    sudo cp mollyd /usr/local/bin/
    ```
2.  **Install the service file:**
    ```sh
    sudo cp mollyd.service /etc/systemd/system/
    ```
3.  **Enable and start the service:**
    ```sh
    sudo systemctl enable --now mollyd.service
    ```

You can view the daemon's logs using `journalctl` to ensure it is behaving as expected:
```sh
journalctl -u mollyd -f
```

## TODO

It would also be good to have this daemon start and stop in response to the USB device being added and removed, avoiding the need to poll.
