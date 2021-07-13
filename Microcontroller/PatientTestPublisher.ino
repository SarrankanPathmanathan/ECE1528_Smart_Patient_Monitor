#include <DHT.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <Adafruit_Fingerprint.h>
 #include <Ticker.h>


const char*ssid = "Sarran Home";
const char*pass = "papaketherma#5690";
const char*mqttBroker = "192.168.0.20";
const int mqttPort =1883;
const int monitorID = 12345;
WiFiClient espClient1;
PubSubClient client(espClient1);
int status = WL_IDLE_STATUS;
unsigned long lastSend;
const int sendPeriod = 3000;

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
const char*temperature_id = "arduino_DHT11";
#define DHTPIN D12     // NodeMCU digital pin D3
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);// Initialize DHT sensor.
const char* temperatureTopic = "v1/DHTsensor/roomtempandhumidity";

//Pulse Sensor
const char*pulse_id = "arduino_PulseSensor";
#define pulsePin A0   //Analog input pin Number on ESP8266
#define blinkPin D15    //Led On Chip
////////////////
//LOOP VARIABLES
////////////////
volatile int BPM;                   // int that holds raw Analog in 0. updated every 2mS
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // int that holds the time interval between beats! Must be seeded!
volatile boolean Pulse = false;     // "True" when User's live heartbeat is detected. "False" when not a "live beat".
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.
const char* heartTopic = "v1/pulseSensor/Heart";


//////////////////////
//INTERRRUPTS VARIABLE
//////////////////////
Ticker flipper;
volatile int rate[10];                        // array to hold last ten IBI values
volatile unsigned long sampleCounter = 0;     // used to determine pulse timing
volatile unsigned long lastBeatTime = 0;      // used to find IBI
volatile unsigned long current;
volatile int P = 512;                         // used to find peak in pulse wave, seeded
volatile int T = 512;                         // used to find trough in pulse wave, seeded
volatile int thresh = 560;                    // used to find instant moment of heart beat, seeded 530du
volatile int amp = 0;                         // used to hold amplitude of pulse waveform, seeded
volatile boolean firstBeat = true;            // used to seed rate array so we startup with reasonable BPM
volatile boolean secondBeat = false;          // used to seed rate array so we startup with reasonable BPM
volatile unsigned long lastMillis = 0;        // used to determine pulse timing
volatile float tempSignal=0;
volatile int msTime = 0;

//Biometric
SoftwareSerial mySerial(D13, D14);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
bool authorized = false;


void setup() {
  Serial.begin(9600);
  InitWiFi();
  dht.begin();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(blinkPin,OUTPUT);
  delay(10);
  client.setServer(mqttBroker,mqttPort);
  client.subscribe("v1/distance/reactionTime");
  lastSend = 0;
  initBiometric();
  isAuthorizedPatient();
  interruptSetup();
}

void initBiometric(){
  while (!Serial); 
  delay(100);
  
  // set the data rate for the sensor serial port
  finger.begin(57600);
  
  if (finger.verifyPassword()) {
    //Serial.println("Found fingerprint sensor!");
  } else {
    //Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }
  
  finger.getTemplateCount();
  //Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" valid users.");
  //Serial.println("Waiting for valid finger..."); 
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
      //Serial.println("Authenticated Patient with ID #");
      //Serial.print(id);
      authorized = true;
    }
  }
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
       doc = getHeartBPM();
      if(doc["id"] != "error"){
      sendSensorData(heartTopic,doc);
    }
      lastSend = millis();
    }
    getHeartBeatData();
    client.loop(); 
}


void sendSensorData(const char* topic, DynamicJsonDocument jsonPayload){
  char attributes[1000];
  serializeJson(jsonPayload,attributes);
  client.publish(topic,attributes);
}

void getHeartBeatData(){
   Signal = analogRead(pulsePin);
   Serial.println(Signal);  
}

DynamicJsonDocument getTemperatureAndHumidityData() {
  DynamicJsonDocument doc(1024);
  //Serial.println("Collecting temperature and humidity data.");// Reading temperature or humidity takes about 250 milliseconds!
  float h = dht.readHumidity();
  float t = dht.readTemperature();// Temperature is in Celsius by default
  if(isnan(h) || isnan(t)) {
    //Serial.println("Failed to read from DHT sensor!");
    doc["id"] = "error";
    return doc;
  }
  String temperature = String(t);
  String humidity = String(h);// Just debug messages
  // Prepare a JSON payload string
  doc["id"] = temperature_id;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["monitorId"] = monitorID;
  doc["status"] = "OK";
  return doc;
}


DynamicJsonDocument getDistanceData() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(10);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration=pulseIn(echoPin, HIGH);
  distance=duration*0.034/2;
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
  //Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  //Serial.print(" with confidence of "); Serial.println(finger.confidence);
  DynamicJsonDocument doc(1024);
  doc["id"] = finger.fingerID;
  doc["monitorID"] = monitorID;
  doc["status"] = "OK";
  char attributes[100];
  serializeJson(doc, attributes);
  client.publish( "v1/Adafruit/fingerprint", attributes );
  return finger.fingerID; 
}

DynamicJsonDocument getHeartBPM(){
  DynamicJsonDocument doc(1024);
  if(msTime>10000)
    {
        noInterrupts();
        interrupts();
        msTime=0;
    }
    if (QS == true) {    // A Heartbeat Was Found
        // BPM and IBI have been Determined
        // Quantified Self "QS" true when arduino finds a heartbeat
        //serialOutputWhenBeatHappens();         // A Beat Happened, Output that to serial.
        QS = false;                            // reset the Quantified Self flag for next time
    }
  if(BPM >=0){
    doc["id"] = pulse_id;
    doc["bpm"] = BPM;
    doc["monitorId"] = monitorID;
    doc["status"] = "OK";
  }else{
    doc["id"] = "error";
  }
  return doc;
}

//  Decides How To OutPut BPM and IBI Data
void serialOutputWhenBeatHappens() {
    sendDataToSerial('B',BPM);                 // send heart rate with a 'B' prefix
    sendDataToSerial('Q',IBI);                 // send time between beats with a 'Q' prefix
}

void sendDataToSerial(char symbol, int data ) {
    Serial.print(symbol);
    Serial.println(data);
}

void interruptSetup() {
// Initializes Ticker to have flipper run the ISR to sample every 2mS as per original Sketch.
    flipper.attach_ms(2, ISRTr);
}

void ISRTr() {
    noInterrupts();                            // disable interrupts while we do this

    Signal = analogRead(pulsePin);             // read the Pulse Sensor
    current=millis();
    int difference = current-lastMillis;
    lastMillis=current;
    sampleCounter += difference;               // keep track of the time in mS with this variable
    msTime+=difference;
    int N = sampleCounter - lastBeatTime;      // monitor the time since the last beat to avoid noise

    //  find the peak and trough of the pulse wave
    if(Signal < thresh && N > (IBI/5)*3) {     // avoid dichrotic noise by waiting 3/5 of last IBI
        if (Signal < T) {                      // T is the trough
            T = Signal;                        // keep track of lowest point in pulse wave
        }
    }

    if(Signal > thresh && Signal > P) {        // thresh condition helps avoid noise
        P = Signal;                            // P is the peak
    }                                          // keep track of highest point in pulse wave

    //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
    // signal surges up in value every time there is a pulse
    if (N > 250) {                                   // avoid high frequency noise
        if ( (Signal > thresh) && (Pulse == false) && (N > (IBI/5)*3) ) {
            Pulse = true;                            // set the Pulse flag when we think there is a pulse
            digitalWrite(blinkPin,LOW);              // turn on pin 13 LED
            IBI = sampleCounter - lastBeatTime;      // measure time between beats in mS
            lastBeatTime = sampleCounter;            // keep track of time for next pulse

            if(secondBeat) {                         // if this is the second beat, if secondBeat == TRUE
                secondBeat = false;                  // clear secondBeat flag
                for(int i=0; i<=9; i++) {            // seed the running total to get a realisitic BPM at startup
                    rate[i] = IBI;
                }
            }

            if(firstBeat) {                          // if it's the first time we found a beat, if firstBeat == TRUE
                firstBeat = false;                   // clear firstBeat flag
                secondBeat = true;                   // set the second beat flag
                interrupts();                        // enable interrupts again
                return;                              // IBI value is unreliable so discard it
            }


            // keep a running total of the last 10 IBI values
            word runningTotal = 0;                   // clear the runningTotal variable

            for(int i=0; i<=8; i++) {                // shift data in the rate array
                rate[i] = rate[i+1];                 // and drop the oldest IBI value
                runningTotal += rate[i];             // add up the 9 oldest IBI values
            }

            rate[9] = IBI;                           // add the latest IBI to the rate array
            runningTotal += rate[9];                 // add the latest IBI to runningTotal
            runningTotal /= 10;                      // average the last 10 IBI values
            BPM = 60000/runningTotal;                // how many beats can fit into a minute? that's BPM!
            QS = true;                               // set Quantified Self flag
            // QS FLAG IS NOT CLEARED INSIDE THIS ISR
        }
    }

    if (Signal < thresh && Pulse == true) {  // when the values are going down, the beat is over

        digitalWrite(blinkPin,HIGH);           // turn off pin 13 LED
        Pulse = false;                         // reset the Pulse flag so we can do it again
        amp = P - T;                           // get amplitude of the pulse wave
        thresh = amp/2 + T;                    // set thresh at 50% of the amplitude
        P = thresh;                            // reset these for next time
        T = thresh;
    }

    if (N > 2500) {                            // if 2.5 seconds go by without a beat
        thresh = 530;                          // set thresh default
        P = 512;                               // set P default
        T = 512;                               // set T default
        lastBeatTime = sampleCounter;          // bring the lastBeatTime up to date
        firstBeat = true;                      // set these to avoid noise
        secondBeat = false;                    // when we get the heartbeat back
        BPM=0;
    }
    interrupts();                              // enable interrupts when youre done!
}

void InitWiFi() {
  WiFi.begin(ssid,pass);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void reconnect() {
  // Loopuntilwe are reconnected
  while(!client.connected()) {
    status = WiFi.status();
    if( status != WL_CONNECTED) {
      WiFi.begin(ssid,pass);
      while(WiFi.status() != WL_CONNECTED) {
        delay(500);
      }
    }
    if( client.connect("ESP8266 Device") ) {
        //Serial.println( "[DONE]" );
    }else{
      // Wait 5 seconds before retrying
      delay( 5000 );
      }
   }
}
