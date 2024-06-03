# **Telemetry**


# *Project Description*

The objective is to create an equipment monitoring system that allows the 
user to track the voltage and current readings of a system, as well as the
time the system has been in operation. It works by reading analog data from
a shunt, which is published to an MQTT broker in JSON format. The data can be
exported to Prometheus and viewed on a Grafana dashboard. In addition to the
system readings, the project also reads an RFID card and publishes the data
following the same steps as the telemetry data.

# *Project Features*

1. Read the system voltage using a shunt and an INA266
2. Implement an hour meter based on current calculation
3. Read the RFID card
4. Publish all necessary information to the broker

# *Technologies Used*

1. ESP32 
2. MQTT
3. WiFi
4. RTOS
5. Arduino component
6. INA226 + shunt
7. Raspberry pi3

# *General Information*

Compiler: VsCode 1.88.1  <br/>
MCU: ESP32  <br/>
Board: Dev module 38 pins <br/>
Date: 2024, June <br/>
Author: [@joaouzeda](https://github.com/joaouzeda)

# *Badges*

![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white)
![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Espressif](https://img.shields.io/badge/espressif-E7352C.svg?style=for-the-badge&logo=espressif&logoColor=white)
![Mosquitto](https://img.shields.io/badge/mosquitto-%233C5280.svg?style=for-the-badge&logo=eclipsemosquitto&logoColor=white)
![Prometheus](https://img.shields.io/badge/Prometheus-E6522C?style=for-the-badge&logo=Prometheus&logoColor=white)
![Grafana](https://img.shields.io/badge/grafana-%23F46800.svg?style=for-the-badge&logo=grafana&logoColor=white)
![Raspberry Pi](https://img.shields.io/badge/-RaspberryPi-C51A4A?style=for-the-badge&logo=Raspberry-Pi)

