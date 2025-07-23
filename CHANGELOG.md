# MBusino change log

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [0.9.22b] - 2025-07-23

### Changed

- little bugfix MBusino5S Sensors
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
