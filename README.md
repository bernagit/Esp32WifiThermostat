# ESP32 distribuited Thermometer

## Introduction

We made this project for an university exam, but also for improve our skill with microcontrollers, arduino and electonic in general. We started with the idea of creating a thermometer that can be viewed via telegram bot and during the implementation we decided to add some functionalities. Use of MQTT IOT protocol, photoresistor to measure brightness, use of multiple sensors, connectable from different WiFi networks, and display the information locally with a LCD.

- [ ] <img title="" src="./Files/Images/chart.png" alt="Project schema">

## How to build it

### Core of the system

##### Component List

- 1x ESP32 (with 5v output from USB)

- 1x 1602 LCD Display (not I2C version)

- 1x 10KΩ Trimmer

- 3x Button

- 3x 10KΩ Resistor

- 2x LED

##### Circuit

<img title="" src="./Files/Images/electric scheme/Core_schema.png" alt="">

### Sensor

##### Component List

- 1x ESP32 (with 5v output from USB)

- 1x 10KΩ Resistor

- 1x DHT11 Module

- 1x Photoresistor GL55

- 1x LED (or relay)

##### Photoresistor GL55

To measure enviroment brightness we decide tu use a photoresistor, a passive component that change its resistance according to the amount of light on it.

In total darkness it varies in order of MOhm, while in normal conditions it has a resistance in order of KOhm.

To connect the sensor with the microcontroller we have to make a voltage divider with a resistor of suitable value (in our case 10kOhm).

We also have to find a correct Y(gamma) value 

##### Circuit

<img title="" src="./Files/Images/electric scheme/Sensor_schema.png" alt="">

## Communication

#### Wifi

#### MQTT

### Security Disclaimer

To make comunication between sensors and core easier we decide to use an online [MQTT Broker](https://www.mqtt-dashboard.com) that not requires credentials. Consequently the messagges from sensor are public and anyone can see them and even worse can send false data that the core will display.

In a real application sensors will publish data on a private MQTT Broker with authentication (User, Password). In this way the security of the system improves a lot.

#### Telegram

#### MultiCore
