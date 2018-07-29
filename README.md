# RSCP Client for E3DC

This software connects to a configured E3DC device and fetches the current data every configured seconds. (Default is every second)
Settings should be made within the `settings.h` file and the `Makefile` file should also be modified. In this setup, the code is transfered to another host and compiled on it. This is proberly not that what you want.
After that, the program can be compiled with the command `make`.

On the target device, the program can be startet by executing the binary or by running `run.sh`.
The difference is, that if a problem occures the main program will exit. `run.sh` will execute the program in an endless loop where it get's automatically restarted.

If available, I recomment to use systemd to start the binary directly. In this way every output of the program is logged and the program can be restarted on failure.

Another option is the use of `/etc/rc.local`. In that case, the following line is added before `exit 0` (You have to adjust the path. Every output of the program is logged to syslog as "RSCP-client"):

```bash
/root/ownRSCP/run.sh 2>&1 | /usr/bin/logger -t RSCP-client &
```

## Provided data

The following data will be provided within the file specified with `TARGET_FILE` in `settings.h`:

```json
{
    "battery": {
        "charge": 74.19355010986328,
        "current": -30.799999237060547,
        "cycles": 167,
        "training": 0,
        "voltage": 48.810001373291016
    },
    "idle_block": {
        "0_charge": {
            "active": false,
            "day": 0,
            "end": "21:0",
            "start": "1:0",
            "type": 0
        },
        "0_discharge": {
            "active": false,
            "day": 0,
            "end": "21:0",
            "start": "1:0",
            "type": 1
        },
        "1_charge": {
            "active": false,
            "day": 1,
            "end": "21:0",
            "start": "1:0",
            "type": 0
        },
        "1_discharge": {
            "active": false,
            "day": 1,
            "end": "21:0",
            "start": "1:0",
            "type": 1
        },
        "2_charge": {
            "active": false,
            "day": 2,
            "end": "21:0",
            "start": "1:0",
            "type": 0
        },
        "2_discharge": {
            "active": false,
            "day": 2,
            "end": "21:0",
            "start": "1:0",
            "type": 1
        },
        "3_charge": {
            "active": false,
            "day": 3,
            "end": "21:0",
            "start": "1:0",
            "type": 0
        },
        "3_discharge": {
            "active": false,
            "day": 3,
            "end": "21:0",
            "start": "1:0",
            "type": 1
        },
        "4_charge": {
            "active": false,
            "day": 4,
            "end": "21:0",
            "start": "1:0",
            "type": 0
        },
        "4_discharge": {
            "active": false,
            "day": 4,
            "end": "21:0",
            "start": "1:0",
            "type": 1
        },
        "5_charge": {
            "active": false,
            "day": 5,
            "end": "21:0",
            "start": "1:0",
            "type": 0
        },
        "5_discharge": {
            "active": true,
            "day": 5,
            "end": "20:0",
            "start": "12:0",
            "type": 1
        },
        "6_charge": {
            "active": false,
            "day": 6,
            "end": "21:0",
            "start": "1:0",
            "type": 0
        },
        "6_discharge": {
            "active": true,
            "day": 6,
            "end": "20:0",
            "start": "12:0",
            "type": 1
        }
    },
    "meta": {
        "autarky": 99.14571380615234,
        "consumption": 99.48719787597656,
        "operation_mode": 3,
        "serial_number": "S10-XXXXXXXXXXXX",
        "timestamp": 1532904452
    },
    "pm": {
        "active_phases": 7,
        "lm0_state": true,
        "power1": -179.0,
        "power2": -134.0,
        "power3": 265.0,
        "voltage1": 231.27999877929688,
        "voltage2": 232.22999572753906,
        "voltage3": 229.00999450683594
    },
    "power": {
        "add": 0,
        "bat": -1499,
        "grid": -8,
        "home": 1413,
        "pv": 0
    },
    "prog": {
        "mem_rss": 840.0,
        "mem_vm": 2456.0
    },
    "pvi": {
        "dc0_current": 0.0,
        "dc0_power": 0.0,
        "dc0_voltage": 159.0,
        "dc1_current": 0.0,
        "dc1_power": 0.0,
        "dc1_voltage": 161.0,
        "on_grid": true,
        "system_mode": 2
    }
}
```

Some fields are hard to understand what the value means which are provided by them. The following list expains some fields of the json data:

- `battery->training` 0 - Not in training / 1 - trainingmode discharge / 2 - trainingmode charge
- `meta->operation_mode` 0: DC / 1: DC-MultiWR / 2: AC / 3: HYBRID / 4: ISLAND
- `pvi->system_mode` IdleMode = 0, / NormalMode = 1, / GridChargeMode = 2, / BackupPowerMode = 3

## Attention

The file defined in `TARGET_FILE` will be rewritten every configured interval, which is by default every second. This can be very bad for systems like Raspberry Pi with SD cards as disk. To prevent high amounts of disk writes, the following line should be added to `/etc/fstab` to write the file only to memory:

```
tmpfs /mnt/RAMDisk tmpfs nodev,nosuid,size=8M 0 0
```

## Licence

```
Copyright (c) 2018 Thomas Bella <thomas@bella.network>

MIT Licence

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## Used libraries

- RSCP example application from E3DC
  `The authors or copyright holders, and in special E3DC can not be held responsible for any damage caused by the software. Usage of the software is at your own risk. It may not be issued in copyright terms as a separate work.`
- AES by Chris Lomont
- JSON for Modern C++ - Copyright (C) 2013-2018 Niels Lohmann - MIT Licence
