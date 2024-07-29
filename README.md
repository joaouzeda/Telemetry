<h1 align="center"> Telemetry  </h1>

# *Project Description*
The objective is to create a monitoring system equipment that allows the user to track the voltage and current readings of a machine, as well as the time it is in use. It works by reading analog data from a shunt, which is published to an MQTT broker in JSON format. The data can be exported to Prometheus and visualized on a Grafana dashboard. 

# *Project Features*

1. Read the system voltage using a shunt and an INA266
2. Implement an hour meter based on current calculation
3. Publish all necessary information to the broker


# *Technologies Used*
1. ESP32 
3. MQTT
4. WiFi
6. RTOS
7. Arduino component
9. INA226 + shunt
10. Raspberry pi3

# *General Information*

Compiler: VsCode 1.9.1 <br/>
Espressif: 5.1.4 <br/>
MCU: ESP32 <br/>
Board: Dev module 38 pins <br/>
Date: 2024, July <br/>

# Autores

| [<img loading="lazy" src="https://avatars.githubusercontent.com/u/55409817?v=4" width=115><br><sub>João Uzêda</sub>](https://github.com/joaouzeda) | 
| :---: |

# *Badges*

![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white)
![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Espressif](https://img.shields.io/badge/espressif-E7352C.svg?style=for-the-badge&logo=espressif&logoColor=white)
![Mosquitto](https://img.shields.io/badge/mosquitto-%233C5280.svg?style=for-the-badge&logo=eclipsemosquitto&logoColor=white)
![Prometheus](https://img.shields.io/badge/Prometheus-E6522C?style=for-the-badge&logo=Prometheus&logoColor=white)
![Grafana](https://img.shields.io/badge/grafana-%23F46800.svg?style=for-the-badge&logo=grafana&logoColor=white)
![Raspberry Pi](https://img.shields.io/badge/-RaspberryPi-C51A4A?style=for-the-badge&logo=Raspberry-Pi)

