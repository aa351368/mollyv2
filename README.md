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

The project includes `mollyd`, a daemon process that runs in the background. It can be configured via command-line arguments and a configuration file.

### Configuration

The daemon loads configuration with the following priority (from lowest to highest):
1.  Hard-coded default values.
2.  Values from `/etc/mollyd.conf`.
3.  Values from a custom config file specified with `--config`.
4.  Command-line arguments (e.g., `--device`).

**Example `/etc/mollyd.conf`:**
```ini
# Molly Daemon Configuration
device = /dev/big_red_button
on_press = /usr/bin/aplay /opt/sounds/staples-that-was-easy.wav
on_open = /usr/local/bin/notify-send "Lid Opened"
on_close =
poll_interval_ms = 50
```

### Systemd Service

The most robust way to run the daemon is with the provided `systemd` service. This will ensure it starts on boot and is restarted automatically if it fails.

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
    sudo systemctl enable mollyd.service
    sudo systemctl start mollyd.service
    ```

You can view the daemon's logs using `journalctl`:
```sh
journalctl -u mollyd -f
```

## TODO

It would also be good to have this daemon start and stop in response to the USB device being added and removed, avoiding the need to poll.
