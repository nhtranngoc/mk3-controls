# Iron Man MK3 Helmet Control Interface

## Overview

This is the full stack control interface for my Iron Man Mk3 helmet replica, as a birthday gift to @ivanempire.

### mDNS

The IP address of an IoT device may vary from time to time, so it’s impracticable to hard code the IP address in the webpage. In this example, we use the `mDNS` to parse the domain name `esp-home.local`, so that we can alway get access to the web server by this URL no matter what the real IP address behind it. See [here](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/protocols/mdns.html) for more information about mDNS.

**Notes: mDNS is installed by default on most operating systems or is available as separate package.**

### Deploying frontend

The pages are built with Vue, and deployed to the on-chip SPI Flash. Be mindful to keep the size below 2MB.

### About frontend framework

We are using [Vue](https://vuejs.org/) alongside [vuetify](https://vuetifyjs.com/) for frontend framework.

## How to use

### Hardware Required

To run this example, you need an ESP32 dev board (e.g. ESP32-WROVER Kit, ESP32-Ethernet-Kit) or ESP32 core board (e.g. ESP32-DevKitC). An extra JTAG adapter might also needed if you choose to deploy the website by semihosting. For more information about supported JTAG adapter, please refer to [select JTAG adapter](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/jtag-debugging/index.html#jtag-debugging-selecting-jtag-adapter). Or if you choose to deploy the website to SD card, an extra SD slot board is needed.

#### Pin Assignment:

Only if you deploy the website to SD card, then the following pin connection is used in this example.

| ESP32  | SD Card |
| ------ | ------- |
| GPIO2  | D0      |
| GPIO4  | D1      |
| GPIO12 | D2      |
| GPIO13 | D3      |
| GPIO14 | CLK     |
| GPIO15 | CMD     |


### Configure the project

Open the project configuration menu (`idf.py menuconfig`). 

In the `Example Connection Configuration` menu:

* Choose the network interface in `Connect using`  option based on your board. Currently we support both Wi-Fi and Ethernet.
* If you select the Wi-Fi interface, you also have to set:
  * Wi-Fi SSID and Wi-Fi password that your esp32 will connect to.
* If you select the Ethernet interface, you also have to set:
  * PHY model in `Ethernet PHY` option, e.g. IP101.
  * PHY address in `PHY Address` option, which should be determined by your board schematic.
  * EMAC Clock mode, GPIO used by SMI.

In the `Example Configuration` menu:

* Set the domain name in `mDNS Host Name` option.
* Choose the deploy mode in `Website deploy mode`, be mindful to pick the SPI Nor flash option, as the helmet hardware currently does not include JTAG connector or an SD card slot.
* Set the mount point of the website in `Website mount point in VFS` option, the default value is `/www`.

### Build and Flash

After the webpage design work has been finished, you should compile them by running following commands:

```bash
cd front/controls-ui
yarn 
yarn build
```

After a while, you will see a `dist` directory which contains all the website files (e.g. html, js, css, images).

Run `idf.py -p PORT flash monitor` to build and flash the project..

(To exit the serial monitor, type ``Ctrl-]``.)

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.

## Example Output

### Render webpage in browser

In your browser, enter the URL where the website located (e.g. `http://esp-home.local`). You can also enter the IP address that ESP32 obtained if your operating system currently don't have support for mDNS service.

Besides that, this example also enables the NetBIOS feature with the domain name `esp-home`. If your OS supports NetBIOS and has enabled it (e.g. Windows has native support for NetBIOS), then the URL `http://esp-home` should also work.

![esp_home_local](https://dl.espressif.com/dl/esp-idf/docs/_static/esp_home_local.gif)

### ESP monitor output

We can set the visor state (on/off, or 0/1) alongside LED brightness.

In the *Light* page, after we set up the light color and click on the check button, the browser will send a post request to ESP32, and in the console, we just print the color value.

```bash
I (6115) example_connect: Connected to Ethernet
I (6115) example_connect: IPv4 address: 192.168.2.151
I (6325) esp-home: Partition size: total: 1920401, used: 1587575
I (6325) esp-rest: Starting HTTP Server
I (128305) esp-rest: File sending complete
I (128565) esp-rest: File sending complete
I (128855) esp-rest: File sending complete
I (129525) esp-rest: File sending complete
I (129855) esp-rest: File sending complete
I (137485) esp-rest: Light control: red = 50, green = 85, blue = 28
```

## Troubleshooting

1. Error occurred when building example: `...front/web-demo/dist doesn't exit. Please run 'npm run build' in ...front/web-demo`.
   * When you choose to deploy website to SPI flash, make sure the `dist` directory has been generated before you building this example.