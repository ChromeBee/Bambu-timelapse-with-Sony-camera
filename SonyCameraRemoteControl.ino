//----------------------------------------------------------------------------------------------------------------------
// This program is based on: WiFiClient from ESP libraries
//
// Camera handling by Reinhard Nickels https://glaskugelsehen.wordpress.com/
// tested with DSC-HX90V, more about protocol in documentation of CameraRemoteAPI https://developer.sony.com/develop/cameras/
// 
// Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
// http://creativecommons.org/licenses/by-sa/3.0/
//
// Requires Arduino IDE with esp8266 core: https://github.com/esp8266/Arduino install by boardmanager
//----------------------------------------------------------------------------------------------------------------------
//
// Added new interrupt handler to deal with input pin connected to Other ESP going low.
// added Status led to show that it is connected to Camera WiFi
// Modified by John Crombie August 2023
 
#include <ESP8266WiFi.h>
#define DEBUG 1
#define BUTTON D5   // pushbutoon on GPIO2
#define OTHER_ESP D6 // ESP reading MQTT data
#define STATUS_LED D7 // shows connection to camera

volatile int counter;
volatile bool take_photo = false;

 
const char* ssid     = "DIRECT-S1C2:DSC-RX100M5";  // Camera SSID
const char* password = "9vUZpHvx";     // Camera WPA2 WiFi password
 
const char* host = "192.168.122.1";   // fixed IP of camera
const int httpPort = 8080;
 
char JSON_1[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"getVersions\",\"params\":[]}";
char JSON_2[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"startRecMode\",\"params\":[]}";
char JSON_3[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"startLiveview\",\"params\":[]}";
char JSON_4[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"stopLiveview\",\"params\":[]}";
char JSON_5[] = "{\"version\":\"1.0\",\"id\":1,\"method\":\"actTakePicture\",\"params\":[]}";
// char JSON_6[]="{\"method\":\"getEvent\",\"params\":[true],\"id\":1,\"version\":\"1.0\"}";
 
 
unsigned long lastmillis;
 
WiFiClient client;


// connect to Camera WiFi
// moved to seperate routine so it can be called again
// if connection lost.
void wifiConnect()
{
   while (WiFi.status() != WL_CONNECTED) {   // wait for WiFi connection
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  delay(1000);
  digitalWrite(STATUS_LED, HIGH); // got connection
  httpPost(JSON_1);  // initial connect to camera
  httpPost(JSON_2); // startRecMode
  httpPost(JSON_3);  //startLiveview  - in this mode change camera settings  (skip to speedup operation)
} 


// Sends json message to the camera
void httpPost(char* jString) {
  if (DEBUG) {Serial.print("Msg send: ");Serial.println(jString);}
  Serial.print("connect to camera at ");
  Serial.println(host);
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  else {
    Serial.print("connected to camera for post ");
    Serial.print(host);
    Serial.print(":");
    Serial.println(httpPort);
  }
 
  // We now create a URI for the request
  String url = "/sony/camera/";
 
  Serial.print("URL: ");
  Serial.println(url);
 
  // This will send the request to the server
  client.print(String("POST " + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n"));
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(strlen(jString));
  // End of headers
  client.println();
  // Request body
  client.println(jString);
  Serial.println("wait for data");
  lastmillis = millis();
  while (!client.available() && millis() - lastmillis < 8000) {} // wait 8s max for answer
 
  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  Serial.println();
  Serial.println("----closing camera post connection----");
  Serial.println();
  client.stop();
}
 
void ICACHE_RAM_ATTR button_pulleddown()  // Interrupt handler for button
{
  counter++;
}

void ICACHE_RAM_ATTR esp_pulleddown()  // Interrupt handler for other ESP8266
{
  take_photo = true;;
}

// debounce routine 
boolean buttonpressed() {  // function to check if pressed
  if (counter!=0) {  
    counter=0;
    delay(10);     // je nach Schalter 
    if (counter==0 && !digitalRead(BUTTON)) return true;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(10);
 
  Serial.println("setup buttons");
  pinMode(BUTTON, INPUT_PULLUP);
  // pinMode(OTHER_ESP, INPUT_PULLUP); // not needed when other ESP is connected but needed if open circuit
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);

// both inputs do the same thing, i.e. set the flag to cause a picture to be taken
  Serial.println("setting interrupts");
  attachInterrupt(digitalPinToInterrupt(BUTTON), button_pulleddown, FALLING);  // handled by interrupt to debounce
  Serial.println("now second");
  attachInterrupt(digitalPinToInterrupt(OTHER_ESP), esp_pulleddown, FALLING);  // same interrupt handles code control
 
  // We start by connecting to a WiFi network
 
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  wifiConnect();
}
 
void loop() {

  if (take_photo || buttonpressed()){
    Serial.println("pressed..");
    // httpPost(JSON_4); //stopLiveview    (skip to speedup operation)
    httpPost(JSON_5);  //actTakePicture
    // httpPost(JSON_3);  //startLiveview    (skip to speedup operation)
    take_photo = false;
    }

    // check if we are still connected to the camera WiFi
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("lost WiFi to Camera");
      digitalWrite(STATUS_LED, LOW);
      wifiConnect();
    }
  
    yield();
}
