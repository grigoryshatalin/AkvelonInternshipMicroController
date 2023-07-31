# AkvelonInternshipMicroController
This project utilizes an ESP8266 microcontroller in conjunction with an MQ-9 gas sensor and a DHT-22 temperature sensor to create a sophisticated monitoring system. The hardware setup involves connecting the sensors to the microcontroller, enabling precise data acquisition of gas levels and temperature. The project integrates with Azure IoT Hub for data streaming, and analysis of such data in PowerBI.

The process involves:

* Hardware Setup: Connect the MQ-9 gas sensor and DHT-22 temperature sensor to the ESP8266 microcontroller.

* Azure IoT Hub and Data Stream Setup: Create an Azure IoT Hub and configure the microcontroller with device information for data streaming.

* Software Setup: Clone the repository, install Visual Studio Code and the Platform IO extension, and upload the code to the microcontroller.

* Data Analysis and Sending to IoT Hub: The microcontroller collects and processes data from the sensors before packaging and transmitting it to the Azure IoT Hub.

By following these steps, the AkvelonInternshipMicroController project provides a reliable and technically advanced system for gas and temperature monitoring, offering valuable insights for various applications.

## Hardware Setup
In this project, an ESP8266 microcontroller was utilized along with a MQ-9 gas sensor and a DHT-22 temperature sensor. The hardware configuration for the setup is detailed as follows:

MQ-9 Gas Sensor:

- Analog output (A0) -> Analog input (A0) of the ESP8266
- Digital output (D0) -> Digital input (D0) of the ESP8266
- Ground (GND) -> Ground (GND) of the ESP8266
- Voltage supply (VCC) -> 3V power supply

DHT-22 Temperature Sensor:

- Ground (GND) -> Ground (GND) of the ESP8266
- Voltage supply (VCC) -> 3V power supply
- Signal output (S) -> Digital input (D2) of the ESP8266

The use of the ESP8266 microcontroller, in conjunction with the MQ-9 and DHT-22 sensors, enables precise monitoring and data acquisition of gas levels and temperature, contributing to a more sophisticated and technically advanced system.



## Azure IOT Hub and Data Stream Setup
Resources from Microsoft:
- [creating an iot hub](https://learn.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal)
- [intro to data streaming with azure](https://learn.microsoft.com/en-us/training/modules/introduction-to-data-streaming/3-understand-event-processing)

Replace the following lines of code in your secrets.h:
```cpp
// Azure IoT
#define IOT_CONFIG_IOTHUB_FQDN "[your Azure IoT host name].azure-devices.net"
#define IOT_CONFIG_DEVICE_ID "Device ID"
#define IOT_CONFIG_DEVICE_KEY "Device Key"
```


With your device ID and device key which may be found when navigating to ‘devices’ in your azure iot hub:

- Navigate to your iot hub
- Navigate to the **'Device Management'** menu
- Navigate to the **'Devices'** menu
- Navigate to your chosen device
- Copy the **'Device ID'** into "Device ID"
- Copy the **'Primary Key'** into "Device Key"

If no such devices exist on step 4, simply create one and follow the previous steps

## Software Setup
1. Clone this repository `git clone https://github.com/grigoryshatalin/AkvelonInternshipMicroController.git`

2. [Install VS Code](https://code.visualstudio.com/)

3. Add the [Platform IO extension](https://platformio.org/install/ide?install=vscode)

3. Open the cloned project in VS Code with Platform IO and navigate to the 📁include directory. 

4. Make a copy of `secrets.h.example` and remove the .example extension. Then fill in your information from the Azure setup step and your WIFI name and password.

5. Finally plug in your device and click `build` in the VS Code bottom toolbar.

6. Once the build is successful, you can click `upload` to send the code to the microcontroller.
7. Open the serial monitor to verify the sensors are working
