# Air-Quality-Sensor
I built an air quality sensor array to measure 3D-printer emissions using an ESP32 microcontroller and atmospheric, VOC, and particulate sensors. I wrote my own embedded C++ code using PlatformIO and developed a live web interface using the wifi capabilities of the ESP32. I also developed a PCB of the design.  

# Overview

This project implements an air quality monitor system to monitor 3D printing emissions using an ESP32, BME680, SGP40, and SPS30 sensors. 

3D printing emissions are typically broken down into volatile organic compounds (VOC) and ultrafine particles. These can be harmful when inhaled and are associated with higher risks for various medical conditions. The sensors used were chosen to detect these particles. 


# Hardware

The project uses an ESP32-WROVER-B mounted on the ESP32 DevKit C. This provides a microcontroller with a variety of peripherals to connect the sensors, as well as WIFI capabilities to host its own webserver. 

The BME680 is a sensor used to detect typical atmospheric conditions - temperature, humidity, etc. However, it also returns a rough indication of VOC count using its gas resistance measurement, which can be compared to the VOC index calculated by the SGP40. Gas resistance is measured in kOhm. The absolute value is meaningless, but deviation over time represents an increase or decrease in VOCs. A lower resistance denotes an increase in VOC count. 

The SGP40 is used to detect VOCs. Given temperature and humidity inputs, it returns a VOC index between 1 (ideal air quality) and 500 (very poor air quality). This index has a baseline of 100, and deviations represent changes in VOC over time. 

The SPS30 sensor is used to determine ultrafine particle concentrations. It detects particle numbers concentrations in the range of 0-3000/cm³ and mass concentrations in the range of  1-1000µg/m³. It also returns a measure of typical particle size. 
The number and mass concentration outputs represent distinct bins. There are five particle concentration bins - PM0.5 to PM10 - and four mass concentration bins - PM1 to PM10. The number following "PM" refers to the upper bound of the particle diameter range in micrometres (e.g. PM2.5 covers all particles from 0.3 μm up to 2.5 μm in diameter). 


# Circuit Description

The circuit is relatively simple; SGP40 and BME680 sensors are connected to the ESP32 via a shared I2C bus, SPS30 via UART as recommended to prevent electromagnetic interference. I'm also running a 5V rail for the SPS30, and a 3V3 rail for everything else. 

![alt text](https://github.com/ldmay/Air-Quality-Sensor/blob/main/Complete%20Breadboard%20Design.jpeg "Image of the completed breadboard design")

Seen here is the working implementation of the design on a breadboard, powered by a battery pack and sending data via wifi to the web server. 

# Firmware

Project written in C++ using PlatformIO, using the following libraries:
lib_deps = 
	adafruit/Adafruit BME680 Library@^2.0.6
	adafruit/Adafruit Unified Sensor
	adafruit/Adafruit BusIO
	adafruit/Adafruit SGP40 Sensor@^1.1.4
	sensirion/Sensirion Gas Index Algorithm
	sensirion/Sensirion UART SPS30@^1.0.0
	esp32async/ESPAsyncWebServer@^3.10.3
	esp32async/AsyncTCP@^3.4.10

# PCB Design

I developed a full PCB design for the circuit using KiCAD, including schematic, PCB layout, and export to Gerber files for manufacture. 

Unfortunately, I was in for a surprise when I was about to send the design to JLCPCB and realised that PCBA fees are far higher than a simple PCB board: 

![alt text](https://github.com/ldmay/Air-Quality-Sensor/blob/main/PCB/Cost%20Total.png "Total cost of getting a PCB manufactured, USD")

Some of the components such as the sensors require precise surface mounting equipment for which I don't have the equipment myself, so unfortunately there was no way around this fee and I wasn't able to test the design itself. 

![alt text](https://github.com/ldmay/Air-Quality-Sensor/blob/main/PCB/3D%20PCB.png "3D View of the finished PCB design")

# Results

