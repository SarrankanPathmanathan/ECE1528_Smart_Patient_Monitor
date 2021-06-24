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

WiFiClient espClient1;
PubSubClient client(espClient1);

//All required PIN Declarations

//Ultra Sonic Distance Sensor
const int trigPin = D3; 
const int echoPin = D2;
const char*distance_id = "arduino_Distance"; 
long duration;
int distance;

//Temperature
const char*temperature_id = "arduino_DHT11"; 
#define DHTPIN D12     // NodeMCU digital pin D3
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);// Initialize DHT sensor.

//Biometric
SoftwareSerial mySerial(D13, D14);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
String auth = "NO";

//Pulse Sensor
int PulseSensorPurplePin = A0;        // Pulse Sensor PURPLE WIRE connected to ANALOG PIN 0
int LED15 = D15;   //  The on-board Arduion LED
int Signal;                // holds the incoming raw data. Signal value can range from 0-1024
int Threshold = 544;            // Determine which Signal to "count as a beat", and which to ingore.

int status = WL_IDLE_STATUS;
unsigned long lastSend;

void setup() {
  Serial.begin(115200);
  InitWiFi();
  dht.begin();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  delay(10);
  client.setServer(mqttBroker,mqttPort);
  lastSend = 0;
  setupBiometric();
}


void setupBiometric(){
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nAdafruit finger detect test");

  // set the data rate for the sensor serial port
  finger.begin(57600);
  
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  finger.getTemplateCount();
  Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  Serial.println("Waiting for valid finger...");
  
}

void loop() {
  while(auth != "YES"){
    if(!client.connected()){
      reconnect();
      delay(2000);
    }
    int id = getFingerprintIDez();
    delay(50);
    if(id > 0){
      Serial.println("Authenticated Patient with ID #");
      Serial.print(id);
      auth = "YES";
    }
  }
  if( !client.connected() ) {
    reconnect();
  }if( millis() - lastSend > 8000 ) { 
    //Update and send only after 1 seconds
    getAndSendDistanceData();
    getAndSendTemperatureAndHumidityData();
    lastSend = millis();
  }
  client.loop();   
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

void getAndSendTemperatureAndHumidityData() {
  Serial.println("Collecting temperature and humidity data.");// Reading temperature or humidity takes about 250 milliseconds!
  float h = dht.readHumidity();
  float t = dht.readTemperature();// Temperature is in Celsius by default
  if(isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
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
  DynamicJsonDocument doc(1024);
  doc["id"] = temperature_id;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  char attributes[100];
  serializeJson(doc, attributes);
  client.publish( "v1/DHTsensor/roomtempandhumidity", attributes );
  Serial.println( attributes );
}

void getAndSendDistanceData() {
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
  char attributes[100];
  serializeJson(doc, attributes);
  client.publish( "v1/Ultrasonicsensor/distance", attributes );
  Serial.println( attributes );
  Serial.println("sent");
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); Serial.println(finger.confidence); 

  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
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
  char attributes[100];
  serializeJson(doc, attributes);
  client.publish( "v1/Adafruit/fingerprint", attributes );
  return finger.fingerID; 
}
