# Flashing ESP32 C3 supermini for MBusino Nano

For the ESP32 C3 are 3 different .bin files created.

**MBusinoNano**

* MBusinoNano.ino.bootloader.bin
* MBusinoNano.ino.partitions.bin
* MBusinoNano.ino.bin 

https://github.com/Zeppelin500/MBusino/tree/main/src/MBusinoNano/build/esp32.esp32.makergo_c3_supermini


**MBusinoNano5S**

* MBusinoNano5S.ino.bootloader.bin
* MBusinoNano5S.ino.partitions.bin
* MBusinoNano5S.ino.bin 

https://github.com/Zeppelin500/MBusino/tree/main/src/MBusinoNano5S/build/esp32.esp32.makergo_c3_supermini

### Use for Flashing https://adafruit.github.io/Adafruit_WebSerial_ESPTool/

* connect the S2 Mini with USB
* press and hold the boot button, press the reset button, release the boot botton. 
* press **Connect** and choose the USP Port
* if connected click **Erase** and wait for finish
* may you have to connect again
* if connected, load all 3 .bin files as follows with the offset

**MBusino**

* Offset 0x 0000    MBusinoNano.ino.bootloader.bin
* Offset 0x 8000    MBusinoNano.ino.partitions.bin
* Offset 0x 10000    MBusinoNano.ino.bin 

**MBusinoNano5S**

* Offset 0x 0000    MBusinoNano5S.ino.bootloader.bin
* Offset 0x 8000    MBusinNano5S.ino.partitions.bin
* Offset 0x 10000    MBusinoNano5S.ino.bin 

--> done

<img src="https://github.com/Zeppelin500/MBusino/blob/main/pictures/AdafruitESPtool.png" width="400">



