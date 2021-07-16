# ECE1528 Smart Patient Monitoring System

This project is for ECE1528 graduate course at the University of Toronto.
It explores the utilization of ESP8266 WeMos D1 R1 microcontrollers and associated sensors in developing a smart patient monitoring system. The patients and doctors play their corresponding roles in this project.

The **patient** interacts with the monitor by connecting to it using their phone and via Bluetooth low energy. Once their finger print is provided for authorization, they are prompted with a series of instructions to perform health based tests. These include reaction time, temperature and heart rate.

The **doctor** can retrieve their patient's results via their own android app which retrieves information on patients hosted on Google Firebase's realtime database.

# Hardware
 - 2x WeMos D1R1 Microcontrollers (Sensor Actuator publishing)
 - Blue status LED
 - Red status LED
 - Green Status LED
 - HM-19 BLE module
 - Adafruit Fingerprint Sensor
 - DHT11 Temperature and Humidity Sensor
 - Ultrasonic Distance Sensor
 - Raspberry Pi (Gateway)
 - Passive Buzzer

# Software
 - Arduino IDE
 - Apache Kafka (Kafka-Firebase Aggregation)
 - Python MQTT-Kafka Bridge
 - Docker (Apache Kafka + Zookeeper)
 - Relevant sensor libraries and tutorials for interrupts
 - Android Studio APK for Doctor APP
 - Arduino Bluetooth Serial Command Line for Patient Instructions
 - Google Firebase (Realtime Database, Cloud Functions)

# Circuit Design 

![Smart Patient Monitor_bb](https://user-images.githubusercontent.com/8919416/125973052-9fcd69ad-4243-4a0e-9ebb-e91560286304.png)

# Patient Instruction Stream via BLE

![217629556_4378074202313501_6731666657314229874_n](https://user-images.githubusercontent.com/8919416/125673137-148aace8-6ba3-4d4f-81a6-d26ce8bc54f9.jpg)
