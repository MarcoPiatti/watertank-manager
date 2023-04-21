# ESP-32 Watertank manager

This project is based on the esp-idf restful server template.

After developing, the pages should be deployed to one of the following destinations:

* SPI Flash - which is recommended when the website after built is small (e.g. less than 2MB).
* SD Card - which would be an option when the website after built is very large that the SPI Flash have not enough space to hold (e.g. larger than 2MB).

To run this example, you need an ESP32 dev board (e.g. ESP32-WROVER Kit, ESP32-Ethernet-Kit) or ESP32 core board (e.g. ESP32-DevKitC). An extra JTAG adapter might also needed if you choose to deploy the website by semihosting. For more information about supported JTAG adapter, please refer to [select JTAG adapter](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/jtag-debugging/index.html#jtag-debugging-selecting-jtag-adapter). Or if you choose to deploy the website to SD card, an extra SD slot board is needed.

### Configure the project

Open the project configuration menu (`idf.py menuconfig`).

In the `Example Connection Configuration` menu:

* Choose the network interface in `Connect using`  option based on your board. Currently we support both Wi-Fi and Ethernet.
* If you select the Wi-Fi interface, you also have to set:
  * Wi-Fi SSID and Wi-Fi password that your esp32 will connect to. (current config is set to insert them by stdin)
* If you select the Ethernet interface, you also have to set:
  * PHY model in `Ethernet PHY` option, e.g. IP101.
  * PHY address in `PHY Address` option, which should be determined by your board schematic.
  * EMAC Clock mode, GPIO used by SMI.

In the `Example Configuration` menu:

* Set the domain name in `mDNS Host Name` option.
* Choose the deploy mode in `Website deploy mode`, currently we support deploy website to host PC, SD card and SPI Nor flash.
  * If we choose to `Deploy website to host (JTAG is needed)`, then we also need to specify the full path of the website in `Host path to mount (e.g. absolute path to web dist directory)`.
* Set the mount point of the website in `Website mount point in VFS` option, the default value is `/www`.

* create a file `secrets.h` that contains your whatsapp api key string and phone numbers for notifiers.