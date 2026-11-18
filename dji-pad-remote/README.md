# DJI Pad Remote

Control a DJI camera with a Bluetooth gamepad, using an **ESP32-C6-LCD-1.47** as the bridge.

```
 8BitDo controller  ──BLE──►  ESP32-C6 + 1.47" screen  ──BLE──►  DJI camera
   (buttons)                  (translates + shows status)        (record / modes)
```

It's built on DJI's own open-source remote demo
([dji-sdk/Osmo-GPS-Controller-Demo](https://github.com/dji-sdk/Osmo-GPS-Controller-Demo)).
DJI's code isn't copied into this folder. `apply.sh` downloads it and applies a small patch (`dji-demo.patch`, about 40 lines).

## ⚠️ Camera support

DJI's demo officially supports the **Osmo Action 4 / 5 Pro / 6 and Osmo 360**.
**Osmo Pocket 3 is not on DJI's list**, so it may not connect. Try an Action or 360 first.

## Controls (defaults)

| Controller | Camera action |
|---|---|
| **A** | Start / stop recording (takes a photo in Photo mode) |
| **B** | Video mode |
| **Y** | Photo mode |
| **X** | Camera's QS quick-switch |
| **D-pad ← / →** | Previous / next mode |
| **Start** | Find and connect the nearest camera |
| Board BOOT button | Same as DJI's demo: hold = connect, tap = record |

Every controller numbers its buttons differently, so the letters above are a guess.
Press a button and the screen shows **`Last: BTN n`**. Put the right numbers in
`pad/pad_config.h` and reflash.

## Build and flash

1. Install **ESP-IDF v5.5**. The easiest way is the *ESP-IDF* extension for VS Code.
2. Run the setup script (on Windows use Git Bash):
   ```bash
   ./apply.sh                 # creates ./dji-pad-remote-build
   ```
3. Build and flash. Plug the board in by USB-C:
   ```bash
   cd dji-pad-remote-build
   idf.py build flash monitor
   ```

## First use

1. Power the board. The screen shows `PAD SCAN`.
2. Put the controller in **Bluetooth pairing mode**. The screen changes to `PAD OK`.
   After the first time it reconnects by itself.
3. Turn on the camera and press **Start**. You'll see `CAM SCAN`, then `CAM OK`. On the
   first connection, approve the pairing on the camera screen if it asks.
4. Press **A** to record. The screen shows a red **REC**.

## Files

| File | What it does |
|---|---|
| `pad/pad_config.h` | **Edit this.** Button numbers, mode list, screen pins |
| `pad/gamepad.c` | Connects the controller, turns presses into camera commands, shares the Bluetooth radio with DJI's code |
| `pad/hid_layout.c` | Reads the controller's own description of its buttons, so no controller model is hard-coded |
| `pad/display.c`, `pad/ui.c` | Screen driver and status screen |
| `dji-demo.patch` | The small change to DJI's demo (routes Bluetooth events through `gamepad.c`) |

## Troubleshooting

* **Black screen**: check the pins in `pad_config.h` against your board's wiki.
  Wrong or negative colours: flip `LCD_INVERT`. Picture shifted: change `LCD_X_GAP`.
* **Controller never shows `PAD OK`**: make sure it's in Bluetooth mode, not 2.4G.
  The serial monitor prints `controller found:` when the board sees it.
* **`Layout: RAW`**: the board couldn't work out the controller's button layout. The
  monitor prints raw reports instead. Send those to me and I'll add support.
* **`MODE FAIL`**: that camera doesn't support that mode. Remove it from `PAD_MODE_CYCLE`.

## Status

The code type-checks against the ESP-IDF v5.5 headers, and the button-layout parser
has a unit test. **It hasn't been built with the ESP toolchain or tested on real
hardware yet.** Expect the first flash to need a little tuning.
