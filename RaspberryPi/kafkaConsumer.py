#!/usr/bin/python3 python
from kafka import KafkaConsumer
import json
import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish



KAFKA_URL = '192.168.0.19'
KAFKA_PORT = '9092'
MQTT_TOPIC_EMERGENCY = "v1/patient/emergency"

MQTT_BROKER_IP="192.168.0.20"
MQTT_BROKER_PORT=1883

print("Running")

consumer = KafkaConsumer(
    'Alerts',
     bootstrap_servers=[KAFKA_URL+':'+KAFKA_PORT],
     auto_offset_reset='earliest',
     enable_auto_commit=True,
     group_id='my-group',
     value_deserializer=lambda m: json.loads(m.decode('utf-8')))
    
def run_consumer():
    for message in consumer:
            if(message.value):
                publish.single(MQTT_TOPIC_EMERGENCY, payload="emergency", qos=0, retain=False, hostname=MQTT_BROKER_IP,
                port=1883, client_id="", keepalive=60, will=None, auth=None, tls=None,
                protocol=mqtt.MQTTv311, transport="tcp")
                print("EMERGENCY PUBLISHED TO PATIENT")

if __name__ == '__main__':
    run_consumer()
