# Flashing ESP32 S2 Mini

For the ESP32 S2 are 3 different .bin files created.

**MBusino**

* MBusino.ino.bootloader.bin
* MBusino.ino.partitions.bin
* MBusino.ino.bin 

https://github.com/Zeppelin500/MBusino/tree/main/src/MBusino/build/esp32.esp32.lolin_s2_mini

**MBusino3S**

* MBusino3S.ino.bootloader.bin
* MBusino3S.ino.partitions.bin
* MBusino3S.ino.bin 

https://github.com/Zeppelin500/MBusino/tree/main/src/MBusino3S/build/esp32.esp32.lolin_s2_mini

### Use for Flashing https://adafruit.github.io/Adafruit_WebSerial_ESPTool/

* connect the S2 Mini with USB
* press and hold the boot button, press the reset button, release the boot botton. 
* press **Connect** and choose the USP Port
* if connected click **Erase** and wait for finish
* may you have to connect again
* if connected, load all 3 .bin files as follows with the offset

**MBusino**

* Offset 0x 1000    MBusino.ino.bootloader.bin
* Offset 0x 8000    MBusino.ino.partitions.bin
* Offset 0x 10000    MBusino.ino.bin 

**MBusino3S**

* Offset 0x 1000    MBusino3S.ino.bootloader.bin
* Offset 0x 8000    MBusino3S.ino.partitions.bin
* Offset 0x 10000    MBusino3S.ino.bin 

--> done

<img src="https://github.com/Zeppelin500/MBusino/blob/main/pictures/AdafruitESPtool.png" width="400">



