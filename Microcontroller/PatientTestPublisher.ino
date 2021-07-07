#include <DHT.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <Adafruit_Fingerprint.h>

const char*ssid = "PATHWIFI";
const char*pass = "4A6F776427A2";
const char*mqttBroker = "192.168.2.87";
const int mqttPort =1883;
const int monitorID = 12345;
WiFiClient espClient1;
PubSubClient client(espClient1);
int status = WL_IDLE_STATUS;
unsigned long lastSend;
const int sendPeriod = 1000;

//All required Sensor Declarations

//Ultra Sonic Distance Sensor
bool distanceTested = false;
const int trigPin = D3; 
const int echoPin = D2;
const char*distance_id = "arduino_Distance"; 
long duration;
int distance;
const char* distanceTopic = "v1/Ultrasonicsensor/distance";

//Temperature
bool temperatureTested = false;
const char*temperature_id = "arduino_DHT11";
#define DHTPIN D12     // NodeMCU digital pin D3
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);// Initialize DHT sensor.
const char* temperatureTopic = "v1/DHTsensor/roomtempandhumidity";

//Pulse Sensor
bool pulseTested = false;
int PulseSensorPurplePin = A0;        // Pulse Sensor PURPLE WIRE connected to ANALOG PIN 0
int LED15 = D15;   //  The on-board Arduion LED
int Signal;                // holds the incoming raw data. Signal value can range from 0-1024
int Threshold = 544;            // Determine which Signal to "count as a beat", and which to ignore.

//Biometric
SoftwareSerial mySerial(D13, D14);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
bool authorized = false;


void setup() {
  Serial.begin(115200);
  InitWiFi();
  dht.begin();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  delay(10);
  client.setServer(mqttBroker,mqttPort);
  client.subscribe("v1/distance/reactionTime");
  lastSend = 0;
  initBiometric();
  isAuthorizedPatient();
}


void loop() {
    DynamicJsonDocument doc(1024);
    if( !client.connected() ) {
      reconnect();
    }if( millis() - lastSend > sendPeriod) {
      doc = getDistanceData();      
      sendSensorData(distanceTopic,doc);
      doc = getTemperatureAndHumidityData();
      if(doc["id"] != "error"){
        sendSensorData(temperatureTopic,doc);
      }
      lastSend = millis();
    }
    client.loop(); 
}

void initBiometric(){
  while (!Serial); 
  delay(100);
  
  // set the data rate for the sensor serial port
  finger.begin(57600);
  
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }
  
  finger.getTemplateCount();
  Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" valid users.");
  Serial.println("Waiting for valid finger..."); 
}

void isAuthorizedPatient(){
  while(!authorized){
    if(!client.connected()){
      reconnect();
      delay(2000);
    }
    int id = getFingerprintIDez();
    delay(50);
    if(id > 0){
      Serial.println("Authenticated Patient with ID #");
      Serial.print(id);
      authorized = true;
    }
  }
}


void InitWiFi() {
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
}

void sendSensorData(const char* topic, DynamicJsonDocument jsonPayload){
  char attributes[1000];
  serializeJson(jsonPayload,attributes);
  client.publish(topic,attributes);
}


DynamicJsonDocument getTemperatureAndHumidityData() {
  DynamicJsonDocument doc(1024);
  Serial.println("Collecting temperature and humidity data.");// Reading temperature or humidity takes about 250 milliseconds!
  float h = dht.readHumidity();
  float t = dht.readTemperature();// Temperature is in Celsius by default
  if(isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    doc["id"] = "error";
    return doc;
  }
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\n");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print("*C\n");
  String temperature = String(t);
  String humidity = String(h);// Just debug messages
  Serial.print( "Sending temperature and humidity : [" );
  Serial.print( temperature ); 
  Serial.print( "," );
  Serial.print( humidity );
  Serial.print( "]   -> " );
  // Prepare a JSON payload string
  doc["id"] = temperature_id;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["monitorId"] = monitorID;
  doc["status"] = "OK";
  return doc;
}


DynamicJsonDocument getDistanceData() {
  Serial.println("Collecting ulstrasonic sensor data.");
  digitalWrite(trigPin, LOW);
  delayMicroseconds(10);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration=pulseIn(echoPin, HIGH);
  distance=duration*0.034/2;
  Serial.print("Distance: ");
  Serial.println(distance);// Just debug messages
  Serial.print( "Sending distance data : [" );
  Serial.print( distance );
  Serial.print( "]   -> " );// Prepare a JSON payload string
  DynamicJsonDocument doc(1024);
  doc["id"] = distance_id;
  doc["distance"] = distance;
  doc["monitorId"] = monitorID;
  doc["status"] = "OK";
  return doc;
}

int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;
  
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  DynamicJsonDocument doc(1024);
  doc["id"] = finger.fingerID;
  doc["monitorID"] = monitorID;
  doc["status"] = "OK";
  char attributes[100];
  serializeJson(doc, attributes);
  client.publish( "v1/Adafruit/fingerprint", attributes );
  return finger.fingerID; 
}

void reconnect() {
  // Loopuntilwe are reconnected
  while(!client.connected()) {
    status = WiFi.status();
    if( status != WL_CONNECTED) {
      WiFi.begin(ssid,pass);
      while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
    Serial.print("Connecting to Mosquitto broker");
   // Attempt to connect (clientId, username, password)
    if( client.connect("ESP8266 Device") ) {
        Serial.println( "[DONE]" );
    }else{
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 5000 );
      }
   }
}
