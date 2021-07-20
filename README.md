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
 - Node.js Express Web Application
 - Arduino Bluetooth Serial Command Line for Patient Instructions
 - Google Firebase (Realtime Database, Cloud Functions)

# Circuit Design 

![Smart Patient Monito2r_bb](https://user-images.githubusercontent.com/8919416/126339795-52cf4294-9d41-4075-819e-a30671d63507.png)


# Patient Instruction Stream via BLE

![217629556_4378074202313501_6731666657314229874_n](https://user-images.githubusercontent.com/8919416/125673137-148aace8-6ba3-4d4f-81a6-d26ce8bc54f9.jpg)


# Firebase Data Model

<img width="205" alt="datamodel" src="https://user-images.githubusercontent.com/8919416/125973839-c3a84cae-d0d0-4c6b-98fc-e0a54feae88d.PNG">

# Instructions

1. Build the circuit as shown above.
2. Apply PatientTestPublisher.ino to Sensor Publisher Microcontroller.
3. Apply PatientTestSubscriber.ino to Sensor Subscriber Microntroller.
4. Setup Kafka, Zookeeper and Kakfa-Firebase Aggregator on Docker.
5. Run MQTT broker on raspberry pi and run MQTT-bridge.py.
6. Ensure communication is possible between raspberry pi and Docker instances (WSL)
7. Upon correct fingerprint, data model is populated into Firebase Real-time Database given it has been setup against the Kafka-Firebase Aggregator.



