#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>


const char*ssid = "PATHWIFI";
const char*pass = "4A6F776427A2";
const char*mqttBroker = "192.168.2.87";
const int mqttPort =1883;
const int LEDBLUE = D13; // D4 - gpio2
const int LEDGREEN = D10;
SoftwareSerial HM19;

///TEST VARIABLES//
bool instructed = false;
bool done = false;
bool testBegin = false;
bool reactionTimeCompleted = false;
bool tempHumidityCompleted = false;
bool heartRateCompleted = false;

///Globals
float temperature;
float humidity;
int BPM;
String patientId;
int monitorId = 0;
uint addr = 0;
int distance = 5000;

struct{
  uint val = 0;
}data;

WiFiClient espClient2;
PubSubClient client(espClient2);

void setup(){
  Serial.begin(115200);
  delay(10);
  EEPROM.begin(512);
  EEPROM.get(addr,data);
  HM19.begin(9600, SWSERIAL_8N1, D2, D3, false, 95, 11); // set HM10 serial at 9600 baud rate
  pinMode(LEDBLUE, OUTPUT);
  pinMode(LEDGREEN, OUTPUT);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid,pass);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP Address is: ");
  Serial.println(WiFi.localIP());
  client.setServer(mqttBroker, mqttPort);
  client.setCallback(ReceivedMessage);
  while(!client.connected()) {
    Serial.println("Connecting to MQTT broker...");
    if(client.connect("ESP8266Client2")) {
      Serial.println("connected");
    }else{
      Serial.print("failed state ");
      Serial.print(client.state());
      delay(2000);
     }
  }
  client.subscribe("v1/Ultrasonicsensor/distance");
  client.subscribe("v1/DHTsensor/roomtempandhumidity");
  client.subscribe("v1/Adafruit/fingerprint");
  client.subscribe("v1/test/testData");
  client.subscribe("v1/pulseSensor/Heart");
  digitalWrite(LEDGREEN,HIGH); //Ready.
}

void ReceivedMessage(char*topic, byte*payload, unsigned int length) {
  Serial.print("Message received in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for(int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  StaticJsonDocument<256>  doc;
  deserializeJson(doc, payload, length);
  if(doc["id"] == "arduino_Distance"){
    distance = doc["distance"];
    monitorId = doc["monitorId"];
  }else if(doc["id"] == "arduino_PulseSensor"){
    BPM = doc["bpm"];
  }else if(doc["id"] == "arduino_DHT11"){
    temperature = doc["temperature"];
    humidity = doc["humidity"];
  }else if(doc["id"] > 0){
    if(doc["id"] == 1){
      patientId = "Patient_1";
      testBegin = true;
    }else if(doc["id"] == 2){
      patientId = "Patient_2";
      testBegin = true;
    }
  }
}


DynamicJsonDocument reactionTimeTest(){
  DynamicJsonDocument response(1024);
  char attributes[100];
  if(distance < 10) { 
    digitalWrite(LEDBLUE, HIGH);
    delay(5000);
    digitalWrite(LEDBLUE,LOW);
    unsigned long currentTime = millis();
    int count = 0;
    while(distance < 10){
      client.loop();
    }
    unsigned long reactionTime = millis() - currentTime;
    response["reactionTime"] = reactionTime;
    HM19.println("Reaction Time Recorded :) Test 1/4 completed");
    reactionTimeCompleted = true;
    instructed = false;
    return response;   
  }else{
     reactionTimeCompleted = false;
    digitalWrite(LEDBLUE, LOW);
  }
}

DynamicJsonDocument temperatureHumidityTest(){
  DynamicJsonDocument response(1024);
  char attributes[100];
  delay(10000);
  client.loop();
  response["temperature"] = temperature;
  response["humidity"] = humidity;
  tempHumidityCompleted = true;
  instructed = false;
  digitalWrite(LEDBLUE, HIGH);
  HM19.println("Temperature and Humidity Recorded :) Test 2/4 completed");
  return response;
}

DynamicJsonDocument HeartRateTest(){
  DynamicJsonDocument response(1024);
  char attributes[100];
  delay(10000);
  client.loop();
  response["BPM"] = BPM;
  instructed = false;
  digitalWrite(LEDBLUE, HIGH);
  HM19.println("Heart Rate Recorded :) Test 3/4 completed");
  heartRateCompleted = true;
  return response;
}

void loop() {
  DynamicJsonDocument reactionTime(1024);
  DynamicJsonDocument temperatureHumidity(1024);
  DynamicJsonDocument heartRate(1024);
  if(testBegin == true){
    if(!reactionTimeCompleted){
      if(!instructed){
        HM19.println("Please remain seated.");
        HM19.println("Place your right hand infront of the distance sensor until the BLUE LED has turned on");
        HM19.println("Remove your hand once the BLUE LED has turned off");
        instructed = true;
      }
     reactionTime = reactionTimeTest();
    }
    if(!tempHumidityCompleted && reactionTimeCompleted){
      if(!instructed){
        HM19.println("Please place your right index finger on the temperature sensor");
        delay(1000);
        HM19.println("Please wait until the BLUE LED has turned on");
        instructed = true;
      }
      temperatureHumidity = temperatureHumidityTest();
      delay(2000);
      digitalWrite(LEDBLUE, LOW);
    }
    if(!heartRateCompleted && tempHumidityCompleted && reactionTimeCompleted){
      if(!instructed){
        HM19.println("Please place a finger on the pulse sensor to measure BPM");
        delay(1000);
        HM19.println("Use the enclosure to ensure obscurity between sensor and finger");
        delay(1000);
        HM19.println("Pleae wait until the BLUE LED has turned on");
        instructed = true;
      }
      heartRate = HeartRateTest();
      delay(2000);
      digitalWrite(LEDBLUE, LOW);
    } 
    if(tempHumidityCompleted && reactionTimeCompleted && heartRateCompleted && !done){
      sendTestData(reactionTime,temperatureHumidity,heartRate);
      done = true;
      testBegin = false;
    }
  }
  client.loop();
}
 
void sendTestData(DynamicJsonDocument reactionTimeTest, DynamicJsonDocument temperatureHumidityTest, DynamicJsonDocument heartRate){
  DynamicJsonDocument testData(1024);
  char attributes[1000];
  int testNumber = data.val;
  testData["id"] = testNumber;
  testData["patientId"]= patientId;
  testData["monitorId"] = monitorId;
  testData["status"] = "Completed";
  testData["reactionTime"] = reactionTimeTest["reactionTime"];
  testData["temperature"] = temperatureHumidityTest["temperature"];
  testData["humidity"] = temperatureHumidityTest["humidity"];
  testData["bpm"] = heartRate["BPM"];
  serializeJson(testData, attributes);
  client.publish("v1/test/testData",attributes);
  data.val += 1;
  EEPROM.put(addr,data);
  EEPROM.commit();
}
