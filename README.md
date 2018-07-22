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
        "charge": 0.0,
        "current": -0.0,
        "cycles": 160,
        "training": 0,
        "training_desc": "0 - Nicht im Training / 1 - Trainingmodus Entladen / 2 - Trainingmodus Laden",
        "voltage": 44.90999984741211
    },
    "meta": {
        "autarky": 5.7694783210754395,
        "consumption": 100.0,
        "operation_mode": 3,
        "operation_mode_desc": "0: DC / 1: DC-MultiWR / 2: AC / 3: HYBRID / 4: ISLAND",
        "serial_number": "S10-XXXXXXXXXXXX",
        "timestamp": 1532287276
    },
    "pm": {
        "active_phases": 7,
        "lm0_state": true,
        "power1": -95.0,
        "power2": 3628.0,
        "power3": 1021.0,
        "voltage1": 232.91000366210938,
        "voltage2": 226.33999633789063,
        "voltage3": 229.6699981689453
    },
    "power": {
        "add": 0,
        "bat": 0,
        "grid": 4563,
        "home": 4839,
        "pv": 276
    },
    "prog": {
        "mem_rss": 840.0,
        "mem_vm": 2456.0
    },
    "pvi": {
        "dc0_current": 0.3499999940395355,
        "dc0_power": 158.0,
        "dc0_voltage": 445.0,
        "dc1_current": 0.3100000023841858,
        "dc1_power": 139.0,
        "dc1_voltage": 444.0,
        "on_grid": true,
        "system_mode": 1,
        "system_mode_desc": "IdleMode = 0, / NormalMode = 1, / GridChargeMode = 2, / BackupPowerMode = 3"
    }
}
```

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
