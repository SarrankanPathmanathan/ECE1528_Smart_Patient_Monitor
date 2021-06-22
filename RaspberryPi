import paho.mqtt.client as mqtt
import json
from kafka import KafkaProducer
from kafka.errors import KafkaError
import logging
from datetime import datetime;

KAFKA_URL = '192.168.2.47'
KAFKA_PORT = '9092'
producer = KafkaProducer(bootstrap_servers=KAFKA_URL+':'+KAFKA_PORT, value_serializer=lambda v: json.dumps(v).encode('utf-8'))
KAFKA_TOPIC_DISTANCE = 'Distance'
KAFKA_TOPIC_TEMP = 'Temperature'
KAFKA_TOPIC_MOTION = 'Motion'
KAFKA_TOPIC_BIOMETRIC = "Biometric"

MQTT_TOPIC_DISTANCE="v1/Ultrasonicsensor/distance"
MQTT_TOPIC_TEMP = "v1/DHTsensor/roomtempandhumidity"
MQTT_TOPIC_BIOMETRIC = "v1/Adafruit/fingerprint"

MQTT_BROKER_IP="192.168.2.87"
MQTT_BROKER_PORT=1883

def on_connect(client, userdata, flags, rc):
    print("MQTT bridge: connected to MQTT broker")
    client.subscribe(MQTT_TOPIC_DISTANCE)
    client.subscribe(MQTT_TOPIC_TEMP)
    client.subscribe(MQTT_TOPIC_BIOMETRIC)

def on_message(client, userdata, message):
    jsonmsg=json.loads(message.payload)
    dateTimeObj = datetime.now()
    jsonmsg["timestamp"] = dateTimeObj = dateTimeObj.strftime("%d-%b-%Y (%H:%M:%S)")
    if(jsonmsg["id"] == "arduino_Distance"):
        future = producer.send(KAFKA_TOPIC_DISTANCE, json.dumps(jsonmsg))
    elif(jsonmsg["id"] == "arduino_DHT11"):
        future = producer.send(KAFKA_TOPIC_TEMP, json.dumps(jsonmsg))
    elif(jsonmsg["id"] > 0):
        future = producer.send(KAFKA_TOPIC_BIOMETRIC, json.dumps(jsonmsg))
        
    # Block for 'synchronous' sends
    #try:
        #record_metadata = future.get(timeout=10)
    #except KafkaError:
        # Decide what to do if produce request failed...
       # logging.exception("Failed")
      #  pass
    # Successful result returns assigned partition and offset
    #print (record_metadata.topic)
    #print (record_metadata.partition)
    #print (record_metadata.offset)
 
mqttc=mqtt.Client()
mqttc.on_connect = on_connect
mqttc.on_message = on_message
mqttc.connect(MQTT_BROKER_IP, int(MQTT_BROKER_PORT))
mqttc.loop_forever()
