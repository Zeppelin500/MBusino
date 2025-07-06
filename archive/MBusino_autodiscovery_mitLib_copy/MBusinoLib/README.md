# MBusinoLib - an Arduino M-Bus Decoder Library

[![version](https://img.shields.io/badge/version-0.7.1-brightgreen.svg)](CHANGELOG.md)
[![license](https://img.shields.io/badge/license-GPL--3.0-orange.svg)](LICENSE)


## Documentation

The **MBusinoLib** library enables Arduino devices to decode M-Bus (Meterbus) RSP_UD telegrams. (Answer from Slave to Master with data records)

Most M-Bus devices should be supported.

Tested at ESPs, Arduino MKR and Uno R4.

### Credits

**MBusinoLib** based at the AllWize/mbus-payload library but with much more decode capabilities. mbus-payload's encode capabilities are not supported.

Thanks to **AllWize!** for the origin library https://github.com/allwize/mbus-payload 

Thanks to **HWHardsoft** and **TrystanLea** for the M-Bus communication for MBusino: https://github.com/HWHardsoft/emonMbus and https://github.com/openenergymonitor/HeatpumpMonitor

### Class: `MBusinoLib`

Include and instantiate the MBusinoLib class. The constructor takes the size of the allocated buffer.

```c
#include <MBusinoLib.h>

MBusinoLib payload(uint8_t size);
```

- `uint8_t size`: The maximum payload size to send, e.g. `255`

## Decoding

### Method: `decode`

Decodes payload of a whole M-Bus telegram as byte array into a JsonArray (requires ArduinoJson library). The result is an array of objects. The method call returns the number of decoded fields or 0 if error.

```c
uint8_t decode(uint8_t *buffer, uint8_t size, JsonArray& root);
```

same line from the example
```c
uint8_t fields = payload.decode(&mbus_data[Startadd], packet_size - Startadd - 2, root); 
```

Example JSON output:

```
[
{
    "value_scaled": 22.06,
    "units": "C",
    "name": "external_temperature_min"
  },
]
```

Example extract the JSON

```c
      for (uint8_t i=0; i<fields; i++) {
        const char* name = root[i]["name"];
        const char* units = root[i]["units"];           
        double value = root[i]["value_scaled"].as<double>(); 
        const char* valueString = root[i]["value_string"];   

        //...send or process the Values
      }
```
### possible contained records
only contained records will be sended

* **["value_scaled"]** contains the value of the record as 64 bit real
* **["value_string"]** contains the value of the record as ASCII string (only for Time/Dates and special variable lengs values)
* **["units"]** contains the unit of the value as ASCII string
* **["name"]** contains the name of the value as ASCII string incl. the information of the function field (min, max, err or nothing for instantaneous)
* **["subUnit"]** countains the transmitted sub unit
* **["storage"]** countains the transmitted storage number
* **["tariff"]** countains the transmitted tariff

There are more records available but you have to delete the out comment in the library.

* **["vif"]** contains the VIF(E) as HEX in a string.
* **["code"]** contains the library internal code of the VIF
* **["scalar"]** contains the scalar of the value
* **["value_raw"]** contains the row value


### Method: `getError`

Returns the last error ID, once returned the error is reset to OK. Possible error values are:

* `MBUS_ERROR::NO_ERROR`: No error
* `MBUS_ERROR::BUFFER_OVERFLOW`: Buffer cannot hold the requested data, try increasing the buffer size. When decoding: incomming buffer size is wrong.
* `MBUS_ERROR::UNSUPPORTED_CODING`: ~~The library only supports 1,2,3 and 4 bytes integers and 2,4,6 or 8 BCD.~~
* ~~`MBUS_ERROR::UNSUPPORTED_RANGE`: Couldn't encode the provided combination of code and scale, try changing the scale of your value.~~
* `MBUS_ERROR::UNSUPPORTED_VIF`: When decoding: the VIF is not supported and thus it cannot be decoded.
* `MBUS_ERROR::NEGATIVE_VALUE`: ~~Library only supports non-negative values at the moment.~~

```c
uint8_t getError(void);
```


## References

* [The M-Bus: A Documentation Rev. 4.8 - Appendix](https://m-bus.com/assets/downloads/MBDOC48.PDF)
* [Dedicated Application Layer (M-Bus)](https://datasheet.datasheetarchive.com/originals/crawler/m-bus.com/ba82a2f0a320ffda901a2d9814f48c24.pdf) by H. Ziegler

## License


The MBusinoLib library is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

The MBusinoLib library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with the MBusinoLib library.  If not, see <http://www.gnu.org/licenses/>.
