# Third-party notices

RetroPad Link's own source code is licensed under the Apache License 2.0 (see
[LICENSE](LICENSE) and [NOTICE](NOTICE)).

The firmware file (`.uf2`) is built from RetroPad Link together with JoypadOS
and the libraries below. Because one of them, SNESpad, is licensed under the
**GNU GPL v3.0**, the firmware file as a whole is distributed under the
**GNU GPL v3.0**. You may use, share and modify it under those terms.

## Components in the firmware

| Component | Licence | Source |
|---|---|---|
| JoypadOS | Apache-2.0 | https://github.com/joypad-ai/joypad-os |
| SNESpad | GPL-3.0 | https://github.com/RobertDaleSmith/SNESpad |
| joybus-pio | LGPL-3.0 | https://github.com/RobertDaleSmith/joybus-pio (fork of https://github.com/JonnyHaystack/joybus-pio) |
| libxsm3 | LGPL-2.1-or-later | https://github.com/RobertDaleSmith/libxsm3 (fork of https://github.com/InvoxiPlayGames/libxsm3) |
| Raspberry Pi Pico SDK | BSD-3-Clause | https://github.com/raspberrypi/pico-sdk |
| TinyUSB (bundled with the Pico SDK) | MIT | https://github.com/hathach/tinyusb |

The full licence text of each component is included with every release, in
`retropad-link-<version>-licenses.zip`.

## Getting the complete source

Every release names the exact commits it was built from: the RetroPad Link
tag, and the JoypadOS commit with its library submodules. The
[README](README.md#building) explains how to rebuild the firmware from them,
and the release workflow in
[`.github/workflows/build.yml`](.github/workflows/build.yml) is the exact
build used.
