# ESP32-C6 Matter Light

Student project for DIOT Laboratory 3, Phase 1: deploying and inspecting a Matter-enabled RGB light. Follow the instructor's **P3 Matter, Phase 1** slides and submit your own observations to Moodle.

The current handout asks you to configure, build, commission, inspect, and test the supplied firmware. It does not ask you to implement missing C++ handlers: the RGB driver, Matter adapter, attribute traces, and Hue/Saturation support are already provided. Complete the configuration and investigation activities below; do not replace working code with placeholders.

## Requirements

- The laboratory ESP32-C6 board, with its RGB LED on GPIO 8 and BOOT button on GPIO 9.
- The ESP-IDF 6.0.1, the ESP-IDF VS Code extension, and the matching prepared ESP-Matter SDK.
- Windows + WSL environment. 
- A phone with SmartLife and access to the laboratory network. Use the account, permissions, and network settings provided by the instructor; app screens vary by version.

The ESP-Matter SDK is not downloaded by cloning this repository.

## Open and build

Clone the student project next to the prepared SDK:

```text
diot/
  esp-matter/
  esp32c6_matter_light/
```

```bash
git clone https://github.com/die-diot/esp32c6_matter_light.git
```

1. Open `esp32c6_matter_light` in VS Code. On Windows, reopen it in the course WSL environment.
2. Select the course ESP-IDF installation in the extension and set the target to **esp32c6**.
3. Open **ESP-IDF: SDK Configuration Editor**, then **Example Configuration > DIOT Matter Provisioning**.
4. Enter your assigned station BLE name and valid setup passcode using the slide naming convention. Leave the long-press duration at the instructed value. Save the configuration.
5. Select **ESP-IDF: Build your project**. The first build downloads managed components and can take several minutes.
6. Select the attached board's serial port, flash, and open the serial monitor using the extension. This repository does not choose a machine-specific serial port for you.

`main/Kconfig.projbuild` defines options, `sdkconfig.defaults*` supplies the shared baseline, and generated `sdkconfig` stores your local choices. Do not edit generated configuration by hand or commit it. The compiler creates `build/config/sdkconfig.h` with the corresponding `CONFIG_*` macros.

CMake resolves the adjacent SDK and its prepared Pigweed environment and propagates those paths to Ninja/GN. An existing `ESP_MATTER_PATH` can select another prepared SDK. Do not copy a teacher's absolute paths or virtual environment into this project.

If a terminal build is needed, activate the course ESP-IDF and ESP-Matter environments first. The normal student workflow is the ESP-IDF extension, not a copied local `build/` folder.

## Your Phase 1 activities

|  Activity | Where to work or inspect |
| --- | --- |
| Open the project, identify the dependency manifest and the C/C++ boundary, and trace the application startup. | `main/idf_component.yml`, `main/main.c`, `components/` |
| Explain the configuration files; calculate and configure your own station identity through the SDK Configuration Editor. | `main/Kconfig.projbuild`, generated `sdkconfig` |
| Build the firmware and prepare the phone while it builds. | ESP-IDF extension and SmartLife |
| Flash, record your own setup payload, commission the board, and identify commissioning events. | Serial monitor and SmartLife |
| Test phone control and a short BOOT press; compare physical LED behavior, app state, and serial events. | `main/main.c`, `diot_matter_toggle_light()`, serial monitor |
| Explain the difference between an RGB emitter, its local FSM, and an advertised Matter light type. | `components/diot/diot_led.c`, `components/diot_matter/diot_matter_devices.cpp` |
| Move the app controls, record the actual cluster/attribute trace, and find its handling branch. No extra log injection is needed. | `apply_attribute_update()` and serial monitor |
| Inspect the initial color mode and the separately registered Hue/Saturation feature; identify the Hue attribute symbol from the code. | `create_light_device()`, `apply_attribute_update()` |
| Hold BOOT for the configured long-press duration, release it, and record the actual factory-reset/reboot logs. | Board, serial monitor, SmartLife |

Use your own measurements, screenshots, and log excerpts. This repository does not include Moodle answer keys or a completed observation report.

## Code map

```text
main/main.c                         Application orchestration in C
main/Kconfig.projbuild              Station identity and button settings
components/fsm/                    Generic FSM engine in C
components/diot/                   Protocol-independent LED/button drivers in C
components/diot_matter/
  diot_matter.cpp                  Stack startup, events, reset, and local toggle
  diot_matter_devices.cpp          Light endpoint, clusters, features, callbacks
  diot_matter_led.c                Matter color/brightness to RGB conversion
  diot_commissioning.cpp           Laboratory commissioning data providers
  include/                        Headers, including the C-compatible API
```

The light driver has a single-state FSM with an update action. The C adapter stores the requested RGB/on-off state and immediately fires that FSM to write the WS2812 through `led_strip`. It is not a second periodic network task. The application loop periodically fires the button FSM; a short press updates Matter's `OnOff` attribute, which follows the same adapter-to-LED path as a phone request.

Do not modify `managed_components/`, the shared SDK, or board wiring for these activities. Read the prepared C++ integration where requested; student application code remains C.

## Reset and app behavior

- **RST/EN:** reboots the board and normally preserves firmware and persistent Matter credentials.
- **Short BOOT press:** toggles the light through its Matter attribute.
- **Long BOOT press, then release:** requests a Matter factory reset. The implementation dispatches `BUTTON_LONG_PRESS_UP`, not a reset exactly when the hold timer expires.
- **Erase flash:** also removes firmware; this is not needed for the normal button activity.

Factory reset removes commissioning credentials but keeps the program. Do not reset merely because you changed a log or rebuilt the same model. When switching between incompatible lab device models, follow the instructor's reset procedure. Adding another Matter administrator is a separate commissioning-window activity, not a reason to factory-reset an already commissioned node.

Hue/Saturation, XY, and white-temperature requests are converted to RGB. A WS2812 approximates white using RGB emitters; it is not a calibrated tunable-white lamp. App-specific White controls and grouping are not guaranteed by this firmware: record what actually happens. A serial `APP COMMAND` trace describes an attribute callback; it is not a packet capture or proof that every incoming command changed an attribute.

## Troubleshooting

- **Pigweed environment not found:** the adjacent SDK is missing its prepared `.environment/build_overrides/pigweed_environment.gni`. Complete the course ESP-Matter setup; do not change a cluster handler to fix a build environment.
- **No board in WSL:** attach the USB device using the lab procedure and select its port in the extension. Compiling does not require an attached board.
- **No BLE advertisement:** the board may already be commissioned. Follow the appropriate reset or sharing activity rather than repeatedly erasing flash.
- **Pairing fails:** check the assigned identity, the phone's Bluetooth/network permissions, and the laboratory Wi-Fi instructions. Never add Wi-Fi credentials to source files or Git.
- **Unexpected logs:** use the messages printed by your firmware/app version, not invented lines from an illustration. Keep setup payloads and account details out of public issues and screenshots.
