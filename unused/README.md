# unused/

Upstream BOSSA sources that are not part of the Arduino/ESP32 library build.
The Arduino build only compiles `src/` ([library specification](https://arduino.github.io/arduino-cli/library-specification/)),
so anything in this directory is excluded without being deleted.

Files were moved here with `git mv` and are otherwise unmodified, so upstream
changes to them rebase cleanly via git rename detection.

Contents:

- Desktop GUI (`Bossa*.cpp/h`, wxWidgets), CLI (`bossac.cpp`, `bossash.cpp`,
  `Command`, `Shell`, `CmdOpts`) and host serial ports / port factories —
  never compiled on Arduino (they were `#ifndef ARDUINO`-guarded before the move).
- SAM-family flash drivers (`EfcFlash`, `EefcFlash`, `D2xNvmFlash`,
  `D5xNvmFlash`) — the Arduino build only flashes targets whose bootloader
  supports chip identification (nRF52840 on the UNO R4 WiFi); the SAM chip-ID
  probing that instantiated these drivers is `#ifndef ARDUINO`-guarded in
  `src/Device.cpp`.
- `BossaArduino.{cpp,h}` — reference Arduino wrapper (not upstream code;
  added on this branch). Shows how to drive the core: a `SerialPort`
  implementation over `HardwareSerial`, a `FlasherObserver` progress bar,
  and the UNO R4 WiFi boot/reset GPIO sequence with SPIFFS staging.
  Consumers now vendor this themselves; `src/` contains only the
  platform-neutral SAM-BA engine (abstract `SerialPort` + `FlasherObserver`).

To restore a file to the build, `git mv` it back into `src/` and, for the SAM
flash drivers, re-enable the guarded section in `src/Device.cpp`.
