# RetroPad Link

Firmware for an RP2350 board that turns original Nintendo controllers into USB
gamepads, with a built-in controller tester on a small OLED screen.

Plug in a **SNES, NES, N64 or GameCube** controller and it shows up on your PC
as a USB gamepad. Hold two buttons for three seconds and the screen becomes a
tester: live button display, joystick wear test, rumble test, and diagnostics
that catch worn or bouncing buttons.

RetroPad Link is built on [JoypadOS](https://github.com/joypad-ai/joypad-os).
It plugs into an unmodified JoypadOS build tree (plus one small patch) rather
than forking it.

## Download

Ready-to-flash firmware is on the
[Releases page](https://github.com/joshfridey/retropad-link/releases). Download
the `.uf2` file and follow [Flashing](#flashing) below. It is built for the
Waveshare Core2350B with the wiring described under [Hardware](#hardware).

## Features

- **Four controller ports:** SNES, NES, N64 and GameCube.
- **One controller at a time.** The firmware checks the ports in the order
  SNES → NES → N64 → GameCube and uses the first controller it finds. Unplug
  it and it goes back to scanning.
- **USB gamepad output** via JoypadOS. It appears as *JoypadOS (SInput)*.
- **Controller tester** on a 128×64 OLED:
  - **Live:** every button lights up as you press it. N64 and GameCube also show
    their sticks (and GameCube its analog triggers).
  - **Joystick test** (N64, GameCube main stick and C-stick): traces the
    stick's outline, measures how far it reaches in each direction and where it
    rests, and rates it GOOD / FAIR / WORN / OFF CENTER.
  - **Rumble test** (GameCube): hold A to run the motor.
  - **Diagnostics:** counts which buttons have been pressed, and flags
    *chatter* (a button bouncing on and off) and *dropouts* (a held button
    briefly letting go), the usual signs of worn contacts.
  - **Raw:** the raw button bitmask, for debugging.

## Hardware

| Part | Notes |
|---|---|
| Waveshare Core2350B | RP2350B board. Any RP2350B board can work if you change the pins in `src/config.h`. |
| 1.3" SH1106 OLED, 128×64, I2C | Address `0x3C`. |
| Controller sockets | Replacement console ports or cut extension cables. |

### Wiring

All controllers run on **3.3 V**. Do not power the SNES, NES or N64 pads from
5 V: their data lines would then send 5 V into the RP2350, which isn't 5 V
tolerant. The only 5 V connection is the GameCube rumble motor.

Pins are set in [`src/config.h`](src/config.h).

**SNES**

| Socket pin | Signal | RP2350 |
|---|---|---|
| 1 | Power | 3V3 |
| 2 | Clock | GPIO5 |
| 3 | Latch | GPIO6 |
| 4 | Data | GPIO7 |
| 5, 6 | — | not connected |
| 7 | Ground | GND |

**NES** shares Clock and Latch with the SNES port and has its own Data line, so
both sockets can stay plugged in.

| Socket pin | Signal | RP2350 |
|---|---|---|
| 1 | Ground | GND |
| 2 | Clock | GPIO5 (shared with SNES) |
| 3 | Latch | GPIO6 (shared with SNES) |
| 4 | Data | GPIO15 |
| 5, 6 | Zapper only | not connected |
| 7 | Power | 3V3 |

**N64**

| Signal | RP2350 |
|---|---|
| Ground | GND |
| Data | GPIO14 |
| Power | 3V3 |

**GameCube**

| Socket pin | Signal | RP2350 |
|---|---|---|
| 1 | Rumble motor power | 5V (VBUS) |
| 2 | Data | GPIO29 |
| 3 | Ground | GND |
| 4 | Ground | GND |
| 5 | — | not connected |
| 6 | Logic power | 3V3 |

No external pull-up resistor is needed on the data line: the firmware turns on
the RP2350's internal pull-up. If detection is unreliable with a long cable, a
pull-up of about 1 kΩ from GPIO29 to 3V3 is the usual fix.

Connect **every** ground on the GameCube socket. Detection did not work until
all of them were wired: the controller may not join its grounds internally.

**OLED**

| OLED pin | RP2350 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO8 |
| SCL | GPIO9 |

OLED modules differ in pin order, so wire by the labels on yours.

**Before soldering a socket,** check its pin numbering with a multimeter's
continuity mode against a known pinout. Swapping power and ground can damage a
controller. Wire colours on extension cables are not standardised.

A large capacitor (220–470 µF) across 5V and GND near the GameCube socket helps
if the board resets when the rumble motor starts.

## Using it

1. Plug the board into a PC over USB.
2. Plug in a controller. The screen shows its logo and **CONNECTED**.
3. The controller now works as a USB gamepad.

### Test mode

Hold **Start + Select** for 3 seconds (**Start + Z** on N64 and GameCube).

| Action | SNES / NES | N64 / GameCube |
|---|---|---|
| Next page | Select + Right | Z + Right |
| Previous page | Select + Left | Z + Left |
| Back to Live | Select + Up | Z + Up |
| Exit test mode | Select + Down | Z + Down |
| Clear Diagnostics or a joystick test (hold 3 s) | Start + Select + Down | Start + Z + Down |

Each controller only gets the pages it can use. The dots in the top-right corner
show how many pages there are and which one you're on.

| Controller | Pages |
|---|---|
| SNES, NES | Live → Diagnostics → Raw |
| N64 | Live → Stick → Diagnostics → Raw |
| GameCube | Live → Stick → C-Stick → Rumble → Diagnostics → Raw |

**Joystick test:** rotate the stick slowly around its full edge a few times, then
let go so it can record where it rests. The rating appears once every direction
has been pushed at least halfway (until then it shows SPIN STICK). The reach
figures are the controller's own raw units. GameCube sticks are measured from
the centre the controller reports when plugged in, the same reference a
console uses.

**Diagnostics:** press every button. *PASS* counts buttons seen working.
*CHATTER* means a button switched on and off 4 or more times within 30 ms.
*DROPOUT* means a button held for at least 0.3 s let go for under 30 ms.

**Rumble:** hold A. The motor stops when you release A, change page, or unplug
the controller.

## Building

RetroPad Link builds inside a JoypadOS tree using CMake's
`CMAKE_PROJECT_INCLUDE` hook ([`retropad_hook.cmake`](retropad_hook.cmake)), so
JoypadOS's own files are not edited except for one patch.

**You need:** `git`, CMake, the `arm-none-eabi-gcc` toolchain and Python 3.
Known to build with CMake 4.3 and arm-none-eabi-gcc 15.2 on Fedora.

Clone both projects side by side:

```sh
git clone https://github.com/joshfridey/retropad-link.git
git clone https://github.com/joypad-ai/joypad-os.git
```

Check out the JoypadOS version RetroPad Link is built against and apply the
patch:

```sh
cd joypad-os
git checkout 969c232c
git submodule update --init --recursive
git apply ../retropad-link/patches/joypad-os.patch
cd ..
```

RetroPad Link also builds against the newest JoypadOS at the time of writing
(`9fa79f45`, October 2026), and the patch applies to it cleanly. That
combination hasn't been flashed and tested on hardware yet; `969c232c` has.

The patch adds the SH1106 OLED driver, an I2C write timeout so a missing screen
can't hang the firmware, and `snes_host_get_buttons()`.

Configure and build:

```sh
cmake -S joypad-os/src -B retropad-link/build -G "Unix Makefiles" \
  -DPICO_BOARD=waveshare_core2350b \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PROJECT_INCLUDE="$PWD/retropad-link/retropad_hook.cmake"

make -C retropad-link/build -j"$(nproc)" joypad_retropad_link
```

The firmware is `retropad-link/build/joypad_retropad_link.uf2`.

The same steps run automatically on GitHub for every push, in
[`.github/workflows/build.yml`](.github/workflows/build.yml).

## Flashing

**First time:** hold the board's **BOOT** button while plugging it in. An
`RP2350` drive appears. Copy the `.uf2` file onto it, and the board reboots
into the firmware.

**After that,** you can skip the button. The firmware has a USB serial port
(`/dev/ttyACM*` on Linux) that accepts text commands. Send `BOOTSEL` to reboot
into the bootloader:

```sh
stty -F /dev/ttyACM0 115200 raw -echo
printf 'BOOTSEL\r\n' > /dev/ttyACM0
```

The same port also accepts JoypadOS's `MODES` and `MODE=` commands to change the
USB output mode.

## Project layout

```
.github/workflows/       Firmware build and release workflow
retropad_hook.cmake      Attaches RetroPad Link to the JoypadOS build
retropad_target.cmake    Firmware target: sources, flags, PIO headers
patches/                 Patch applied to JoypadOS
src/
  config.h               All GPIO pin assignments
  version.h              Firmware version
  app.c                  Main loop: connection state, test mode, rumble test
  ui.c                   OLED screens
  diagnostics.c          Button chatter / dropout detection
  joystick_diag.c        Joystick reach, outline and rest tracking
  controller_state.c     Latest routed input event
  controllers/
    controller_manager.c Detects which controller is plugged in
    snes.c nes.c n64.c gamecube.c
                         One adapter per controller (init / task / connected)
    gc/                  GameCube driver (modified copy, see Credits)
  *_logo.h               OLED logo bitmaps
```

## Status

- GameCube joystick ratings use provisional thresholds and still need tuning
  against known-good controllers.
- PS1 / PS2 support is planned. JoypadOS has a driver for it; it needs the
  RP2350's third PIO block.

## Releasing

Releases are built by GitHub Actions. To publish one:

1. Set the new version in [`src/version.h`](src/version.h) and commit it.
2. Tag that commit with the same version and push the tag:

   ```sh
   git tag v0.1.0
   git push origin v0.1.0
   ```

The workflow builds the firmware and publishes a release with the `.uf2`, the
third-party licence texts, and the exact source commits it was built from. It
refuses to publish if the tag doesn't match `src/version.h`.

## Licence

RetroPad Link's source code is licensed under the
[Apache License 2.0](LICENSE). See [NOTICE](NOTICE) for the files that carry
other licences.

**Firmware files (`.uf2`) are distributed under the GNU GPL v3.0,** because
they include SNESpad, which is GPL-3.0. Each release lists the exact source it
was built from. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Credits

- **[JoypadOS](https://github.com/joypad-ai/joypad-os)** by Robert Dale Smith,
  Apache License 2.0. RetroPad Link builds on it, and
  `src/controllers/gc/gc_host.c` is a modified copy of its GameCube host
  driver. `patches/joypad-os.patch` modifies JoypadOS files.
- **[joybus-pio](https://github.com/JonnyHaystack/joybus-pio)** by
  JonnyHaystack, GNU LGPL 3.0. `src/controllers/gc/GamecubeController.c` is a modified
  copy of its GameCube controller reader, and stays under the LGPL 3.0.
- **[SNESpad](https://github.com/RobertDaleSmith/SNESpad)**, GNU GPL 3.0. Reads
  the SNES port, via JoypadOS's SNES host. (The NES port uses RetroPad Link's
  own reader, `src/controllers/nes.c`.)

Changes in the modified copies: raw stick readout and plug-in origin capture
for the joystick test, a handshake that tries several probe commands and bit
rates, and a different PIO block assignment.

Nintendo, SNES, NES, Nintendo 64 and GameCube are trademarks of Nintendo.
"Joypad" is a trademark of Robert Dale Smith. This project is not affiliated
with or endorsed by either.
