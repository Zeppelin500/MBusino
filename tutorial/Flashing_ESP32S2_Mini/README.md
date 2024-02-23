#Flashing ESP32 S2 Mini

For the ESP32 S2 are 3 different .bin files created.

* MBusino3S.ino.bootloader.bin
* MBusino3S.ino.partitions.bin
* MBusino3S.ino.bin 

###Use for Flashing https://adafruit.github.io/Adafruit_WebSerial_ESPTool/

* connect the S2 Mini with USB
* press and hold the boot button, press the reset button, release the boot botton. 
* press **connect** and choose the USP Port
* if connected click erase ans wait for finish
* may you have connect again
* if connected load all 3 .bin files as follows with the offset

* Offset 0x 1000    MBusino3S.ino.bootloader.bin
* Offset 0x 1000    MBusino3S.ino.partitions.bin
* Offset 0x 1000    MBusino3S.ino.bin 

--> done

<img src="https://github.com/Zeppelin500/MBusino/blob/main/pictures/AdafruitESPtool.png" width="400">



