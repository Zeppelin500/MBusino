
# MBusino_calibrate 

MBusino with calibration capabilitys. 

Most DS18B20 are faked and out of specifications.
A new code is in development to find a solution for calibration.

You can calibrate the sensors by sending a MQTT message to MBusino

## 1. Set the average of all DS sensors
### MQTT Topic: CMBusino/calibrateAverage
### MQTT Payload: no matter

Make the average of the connected sensors and add an offset to every sensor. After the calibration, all sensors will show the same value.
You have to bring all connected sensors to the same environment, wait a little bit and send the calibration message.

## 2. Calibrate to a connected Bosch BME280 Sensor
### MQTT Topic: CMBusino/calibrateBME
### MQTT Payload: no matter

Make an offset for every DS sensor based at the meassured value of the BME sensor. After the calibration, all sensors will show the same value.
You have to bring all connected sensors to the same environment, wait a little bit and send the calibration message.

## 3. Calibration by hand
### MQTT Topic: CMBusino/calibrateSensor
### MQTT Payload: Number of the Sensor to calibrate (only the number). e.g.S3 = 3
### MQTT Topic: CMBusino/calibrateValue
### MQTT Payload: a float number e.g. -0.15

Manipulate the offset of a single sensor by sending Sensor numbers and values. The transmitted value will be added to the current offset. First chose the Sensor, then send the value. If a sensor is chosed, you can manipulate multiple times the same sensor.


## 4. Set all offsets to 0
### MQTT Topic: CMBusino/calibrateSet0
### MQTT Payload: no matter

Self explanatory.


## Licence
****************************************************
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************

