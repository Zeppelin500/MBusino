# MBusino
ESP8266 D1 Mini Shield for meassuring and sending some values with MQTT

- M-Bus e.g. heatmeter 
- OneWire 5x z.B. DS18B20, temperature
- I²C z.B. BME280, temperatur, r. humidity, air pressure

* the "old Sensostar only" code is archived.

## Hardware
The PCB is designed with fritzing.
M-Bus is provided by a piggyback M-Bus master.
https://de.aliexpress.com/item/33008746192.html --> the Master!

Beside the USB power supply of D1 Mini is no other adapter necessary.

The Board is usable for only M-Bus (see MBusino_naked code) or only sensor meassuring as well.  

you can use 2,54mm terminals or JST XH to connect the DS18B20

You will find a 3D-printable PCB case inside the case folder.  

To save mony, I place omnibus orders for all parts.

## calibration capabilities 

Most DS18B20 are faked and out of specifications.
You can calibrate the sensors by sending a MQTT message to MBusino.

### 1. Set the average of all DS sensors
* MQTT Topic: MBusino/calibrateAverage
* MQTT Payload: no matter

Make the average of the connected sensors and add an offset to every sensor. After the calibration, all sensors will show the same value.
You have to bring all connected sensors to the same environment, wait a little bit and send the calibration message.

### 2. Calibrate to a connected Bosch BME280 Sensor
* MQTT Topic: MBusino/calibrateBME
* MQTT Payload: no matter

Make an offset for every DS sensor based at the meassured value of the BME sensor. After the calibration, all sensors will show the same value.
You have to bring all connected sensors to the same environment, wait a little bit and send the calibration message.

### 3. Calibration by hand
* MQTT Topic: MBusino/calibrateSensor
* MQTT Payload: Number of the Sensor to calibrate (only the number). e.g.S3 = 3
* MQTT Topic: MBusino/calibrateValue
* MQTT Payload: a float number e.g. -0.15

Manipulate the offset of a single sensor by sending Sensor numbers and values. The transmitted value will be added to the current offset. First chose the sensor, then send the value. If a sensor is chosed, you can manipulate multiple times the same sensor.


### 4. Set all offsets to 0
* MQTT Topic: MBusino/calibrateSet0
* MQTT Payload: no matter

Self explanatory.


## known issues
- Flashing over USB is only possible if the M-Bus master is not connected. OTA update work fine.

- Do not use 2 Boards without changing the name. #define MBUSINO_NAME "MBusino"

- M-Bus is only tested with a "Engelmann Sensostar U" at the moment, but should work with other M-Bus divices. If you have M-Bus issues, let me know.

- Code will only work with the modified MBUSPayload library. 

https://github.com/Zeppelin500/mbus-payload

## Credits
* AllWize for the library to decode M-Bus
* HWHardsoft for the M-Bus communication

## current topics

https://github.com/Zeppelin500/MBusino/discussions



## Lizenz
****************************************************
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************
![PCB](/pictures/MBusino_V05_Leiterplatte.png)
![Bild](pictures/MBusino.jpg)
![Bild](pictures/case.jpg)
