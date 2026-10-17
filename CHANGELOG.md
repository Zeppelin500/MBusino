# MBusino change log

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [Unreleased]

### Added

- **M-Bus debug page in the web interface (MBusinoNano, MBusinoNano5S):** `http://<MBusino>/mbus` shows the last 10 polls (time, address, status OK / no data / bad frame, raw telegram in hex) and the last decoded telegram (header and records). It refreshes every 2 s, shows the time until the next poll and has a "Poll now" button. The data is also available as JSON at `/mbus.json`. The page is linked on the setup page. The M-Bus state machine keeps running only while MQTT is connected (unchanged), so the page shows the telegrams of the running MQTT session.
### Fixed

- **W5500 Ethernet works again on switch ports with auto-negotiation (MBusinoNano, MBusinoNano5S):** since 1.0.2 the W5500 was always forced to 10M full duplex. On a switch port with auto-negotiation (the default, e.g. UniFi) this results in a duplex mismatch (port falls back to half duplex), DHCP fails and the 2 s ETH restart loop makes the port flap. Ethernet now starts with auto-negotiation and switches to forced 10M full duplex (and back) if no IP is received within 15 s after link up, or if no link comes up at all within 8 s after `ETH.begin()`. Once a mode got an IP, it is kept until the next boot. The working link mode is stored in EEPROM and used on the next boot, so equipment that needs forced 10M (Fritzbox 7590, PoE extenders) keeps working.
- The DHCP timeout now starts at the first link up after `ETH.begin()`; link flapping no longer restarts it. After unplugging a working cable it starts again at the next link up.
- MQTT now also connects via Ethernet while the setup AP is running, if Ethernet got its IP after the AP was started.
- **Exactly one transport at a time (MBusinoNano, MBusinoNano5S):** as soon as Ethernet has an IP, the setup AP and a connected Wi-Fi STA are shut down. Previously the AP stayed on forever once it had been started, and a Wi-Fi connection could stay active next to Ethernet; the device then had two IPs and MQTT flipped between them on every link change.
- **Controlled restart after losing a working Ethernet connection (MBusinoNano, MBusinoNano5S):** if an established Ethernet connection (link/IP) does not come back within 15 s, the device restarts and the boot re-picks the transport (Ethernet -> Wi-Fi -> setup AP). Previously a lost IP triggered a restart after only 1 s, while a pure link loss triggered none at all (the device could stay unreachable).
- The "no link" timeout was reduced from 20 s to 8 s; link up is decided at PHY level well below 1 s, so strict equipment (Fritzbox 7590, PoE extenders) falls back to forced 10M full duplex noticeably faster.
- **Crash when the setup AP was switched off (MBusinoNano, MBusinoNano5S):** the mDNS component started by `ArduinoOTA.begin()` died with a Guru Meditation (Load access fault in `igmp_lookfor_group`) when a netif appeared or was torn down - i.e. exactly when Ethernet got its IP while the setup AP was being shut down. Disabling mDNS for the OTA service removes the crash.

### Added

- **Ethernet link mode setting in the web interface (MBusinoNano, MBusinoNano5S):** "automatic" (default, as described above), "auto negotiation" or "forced 10M full duplex". The fixed modes skip the automatic switching. The active mode is published as `<name>/settings/ethLinkMode`.
- Verbose `[ETH]` / `[NET]` serial logging for every transport and link mode decision (boot state, mode switch, AP shutdown, Wi-Fi shutdown, Ethernet loss and recovery), so the behaviour can be followed on the serial console during a test.

### Changed

- **OTA is no longer advertised via mDNS (MBusinoNano, MBusinoNano5S):** the device is no longer reachable as `<name>.local`. The web interface and ArduinoOTA keep working over the IP; the mDNS service was started only by `ArduinoOTA.begin()` and is not used anywhere else in the firmware.

## [1.2.0] - 2026-10-14

### Added

- **Home Assistant `device_class` for M-Bus volume now follows the meter medium (issue #67):** cumulative volume records are reported as `water` on water meters and `gas` on gas meters (anything else stays the generic `volume`). The medium is read from the already decoded M-Bus header and passed to `MBusinoLib::getDeviceClass()`. Previously every m³ value was published as `volume`, so water meters did not show up as water consumption in HA.
- Broader HA `device_class` coverage via MBusinoLib 0.10.0 (illuminance, irradiance, CO₂/CO, particulate matter PM1/2.5/10, wind speed, pH, battery, moisture, distance, sound pressure, apparent/reactive power, precipitation).

### Changed

- M-Bus consumption sensors now use `state_class: total_increasing` (Home Assistant detects counter resets/roll-overs automatically); instantaneous values use `measurement`.
- Depends on MBusinoLib 0.10.0.

## [1.1.2] - 2026-10-04

### Fixed

- **HA `expire_after` for M-Bus sensors now accounts for the slave rotation (MBusino5S, MBusinoNano5S):** it was previously `3 × mbusInterval` regardless of `mbusSlaves`, but each slave is only polled once per full rotation (`mbusSlaves × mbusInterval`). With 3 or more configured slaves the sensors could expire in Home Assistant between two of their updates and show "unavailable" depending on the polling phase (issue #65). Now `expire_after = 3 × mbusSlaves × mbusInterval`, preserving the "3 missed updates" rule per slave. Single-slave variants (MBusino, MBusinoNano) were already correct.

## [1.1.1] - 2026-08-02

### Fixed
- Guard `_vs_` and `_unit` MQTT publishes against NULL/empty values. Empty topics were published for records without ASCII values or units. Now only publishes when value actually exists.

## [1.1.0] - 2026-07-15

### ⚠️ Breaking Change

- **LWT topic changed:** The MQTT Last Will and Testament now uses `{name}/status` with payload `"offline"`, QoS 1, retained. Previously the LWT was published to the bare device name topic with payload `"I am going offline"`, QoS 0, not retained. If you subscribed to the old LWT topic, update your subscription to `{name}/status`.

### Added

- HA availability for all entities: every discovery payload now includes `availability_topic`, `payload_available`, `payload_not_available`. MBusino going offline → all HA entities immediately show unavailable.
- `expire_after` for M-Bus sensors (3× polling interval). If a single M-Bus slave stops responding while MBusino stays connected, only that slave's sensors become unavailable.
- `expire_after` for OneWire/BME280 sensors (3× sensor interval).
- Retained `"online"` status published on `{name}/status` after each MQTT connect.

### Fixed

- **LWT sprintf bug:** `mbusinoName` was used as format string, `/lastwill` suffix silently dropped. Fixed with `snprintf`.
- **value_template fallback removed:** Replaced `{{value_json if value_json is defined else 0}}` with `{{value_json}}`. Broken payloads now keep previous HA state instead of reporting 0.
- Autodiscover buffer increased from 500 to 1024 bytes.

### Dependencies

- MBusinoLib 0.9.20
- MBusCom 0.1.9
- ArduinoJson 7.4.3
- ESP Async WebServer 3.10.3 (ESP32Async)
- Async TCP 3.4.10 (ESP32Async)

## [1.0.4] - 2026-07-10

### Fixed

- Reverted buffer define to MBUS_DATA_SIZE for MBusCom v0.1.9 compatibility
- Depends on MBusCom 0.1.9 (fixed get_response timeout bug)

## [1.0.3] - 2026-04-28

### Added

- M-Bus Header autodiscovery for Home Assistant (all 4 variants)
- Header fields published via MQTT with type-aware loops
- Depends on MBusinoLib 0.9.20 (a_field → address)

## [1.0.2] - 2026-04-01

### Fixed

- W5500 Ethernet auto-negotiation: disable auto-negotiation and force 10BASE-T full-duplex before each ETH.begin() call. Fixes connection failures with strict gigabit equipment (Fritzbox 7590, PoE extenders, managed switches). Applied to MBusinoNano and MBusinoNano5S.

### Changed

- Updated binary files for MBusinoNano and MBusinoNano5S (C3 SuperMini)

## [1.0.1] - 2026-03-20

### Changed

- Buffer safety and code clarity improvements (all firmware variants)
- Merged LED flash for AP mode from upstream
- Compiled all firmware variants with ESP32Async libraries (ESP Async WebServer 3.10.3, Async TCP 3.4.10)
- Updated binary files for all boards

### Dependencies

- MBusinoLib 0.9.16
- MBusCom 0.1.6
- ArduinoJson 7.4.3
- ESP Async WebServer 3.10.3 (ESP32Async)
- Async TCP 3.4.10 (ESP32Async)
- ESP Async TCP 2.0.0 (ESP8266 only)

## [1.0.0] - 2026-01-13

### Changed

- Assigning all allowed M-Bus addresses (0-250) is now possible in the 5S firmware.
- credentials storrages (SSID,Password, Broker, MQTT-Pasword and User) are now 64 characters long.

## [0.9.24] - 2026-01-08

### Changed

- HA audodicovery, change "object_id":"xxx" "default_entity_id":"sensor.xxx"

## [0.9.23] - 2025-09-11

### Changed

- increase name variable at ha audodicovery
- MBusinoLib 0.9.15

## [0.9.22b] - 2025-07-23

### Changed

- little bugfix MBusino5S Sensor8
- all Files still 0.9.22

## [0.9.22] - 2025-07-21

### Changed

- MBusinoLib 0.9.14
- DS18B20 S6 with ESP32 S2 issue fixed

### Added

- Support of Slaves with multible response telegrams

## [0.9.21] - 2025-05-01

### Changed

- MBusinoLib 0.9.12
- Support for 5 Slaves (5S)

### Added

- MBusino Nano with Ethernet

## [0.9.20] - 2025-03-28

### Changed

- Improved WiFi stability for S2 mini (prevents crashes)

## [0.9.19] - 2025-02-27

### Changed

- Sourced the M-Bus communication out to a Arduino IDE accessable Library --> MBusCom

## [0.9.18] - 2025-02-16

### Changed

- rename outsourced code for the GUI-Server from server.h to guiServer.h because of Windows Compiling issues. (.bin files are not changed and sill 0.9.17)

### Added

- case file for Z-Version

## [0.9.17] - 2025-02-06

### Added

- MQTT topic FreeHeap

## [0.9.16] - 2025-01-30

### Changed

- fixed again problems with the webserver since 0.9.14

## [0.9.15] - 2025-01-28

### Changed

- fixed problems with the webserver since 0.9.14

## [0.9.14] - 2025-01-27

### Changed

- MBusinoLib 0.9.9
- changed the server syntax because of a new release of ESPasyncWebServer 
- Stop the whole main loop if the WIFI and MQTT connection is lost.

## [0.9.13] - 2025-01-13

### Changed

- MBusinoLib 0.9.8

## [0.9.12] - 2025-01-04

### Added

- M-Bus request polling per MQTT

## [0.9.11] - 2024-12-03

### Changed

- increased time between M-Bus request and get response for slow slaves 
- placed sensor1.setWaitForConversion(false) etc. to the right place, to set the OneWire communication async
- changed the BME280 topics to english language
- tutorial: add links to the .bin files

## [0.9.10] - 2024-11-16

### Changed

- increased size of jsonstring to 4092 
- add a GUI selection for raw telegram output to debug (only for MBusino)

## [0.9.9] - 2024-11-05

### Changed

- update to MBusinoLib 0.9.7 - HA Autodiscovery StateClass TOTAL for Energy

## [0.9.8] - 2024-06-21

### Changed

- hide MQTT Password

## [0.9.7] - 2024-05-14

### Added

- a switch for Homeassitant autodiscovery messages.

## [0.9.6] - 2024-05-14

### Added

- retained flag for Homeassitant autodiscovery messages.


## [0.9.5] - 2024-04-16

### Changed

- roll back from float to double for a better precision

## [0.9.4] - 2024-04-15

### Changed

- separate decodeing and sending of records for a better stability

- length of form text fields in html are now limited to the length of the variable

## [0.9.3] - 2024-04-11

### Added

- better real support

- support for more as one VIFEs

## [0.9.1] - 2024-03-12

### Added

- Home Assistant autodiscovery for MBusino3S. 

## [0.9.0] - 2024-03-11

### Added

- Home Assistant autodiscovery for MBusino. 


## [0.8.0] - 2024-02-26

### Added

- ESP32 S2 mini is now supported

- ESP32 S2 mini flashing tutorial.

### Changed

- Some improfements at Readme and tutorials

## [0.7.2] - 2024-02-22

### Added

- M-Bus Address

### Changed

- MBusinoAP with Accesspoint is now standard, renamed to MBusino. Old MBusino code without accesspoint is in archive

- Split the MBusino code in deferent .h files for a better clarity  

- English tutorial

- Some improfements at Readme and german tutorial

## [0.7.1] - 2024-01-31

### Changed

- new MBusino own http updater

## [0.7.0] - 2024-01-30
- Initial version with releases and changelog
