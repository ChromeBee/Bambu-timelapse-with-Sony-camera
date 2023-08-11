// sketch that connects to home WiFi and then to Bambu Lab printer to receive MQTT messages
// It then checks for changes to the values of stg_cur and layer_num to determine if a timelapse photo should be taken
// by John Crombie 
// August 2023
// 10-Aug=2023 - added code to handle a servo operated shutter button and giving the device a name based on its IP address

#include "Arduino.h"
#include "ESP8266WiFi.h"
#include <WiFiClientSecure.h>
#include "PubSubClient.h"
#include "WiFiUdp.h"
#include "ArduinoJson.h"

#include <Servo.h>

#define ShutterOutput D6
#define STATUS_LED D7 // shows connection to Bambu
// #define ServoOutput D5 // servo output that presses the shutter ; comment out this #define to exclude all servo code
#ifdef ServoOutput
#define TestButton D8 // pull to ground to trigger servo to take a photo. Used to check servo movements
#endif

// global constants
const char* ssid = "WiFi_SSID"; // Home WiFi SSID
const char* password = "wifi_password"; // Home WiFi password
const char* Bambu_IP = "192.168.1.135"; // IP address of Bambu printer
const uint16_t mqtt_server_port = 8883;  // default MQTT port of Bambu printers
const char* BambuSN = "Serial_Number"; // Bambu printer serial number
const char* BambuLANcode = "Lan_Code";  // Bambu printer LAN access code (password)
const char* BambuUsername = "bblp"; // default Bambu printer username
#ifdef ServoOutput
const int maxServoMovement = 15; // amount of movement in degrees needed to take a picture. Can be positive or negative number
const int midPos = 90;
#endif

// global variables
char DeviceName[20];
char mqttTopic[35];
char Previous_gcode_state[20];
char gcode_state[20];
int Previous_layer_num = -1;
int Previous_stg_cur = -1;
int stg_cur;
int layer_num;
bool par_change = false;
bool stg_cur_change = false;
bool layer_num_change = false;
bool take_photo = false;
bool bambuConnected = false;
#ifdef ServoOutput
Servo cameraServo;
#endif


WiFiClientSecure wifiClient;
WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP);
PubSubClient mqttClient(wifiClient);


// called when MQTT message is available
void ICACHE_RAM_ATTR callback(char* topic, byte* payload, unsigned int length) {
  //  Serial.print("Message arrived on topic: '");
  //  Serial.print(topic);
  if(!bambuConnected) {
    bambuConnected = true;
    digitalWrite(STATUS_LED, HIGH);
  }
  StaticJsonDocument<10000> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }


// Check if it contains the keys we are interested in and if they have changed from the last message
  if (doc.containsKey("print")) {
    /* This prints the entire message don't do it unless needed for debug */
    /*
    Serial.println(F("===== JSON Data ====="));
    serializeJsonPretty(doc, Serial);
    Serial.println(F("======================"));
    */

    stg_cur_change = false;
    layer_num_change = false;
  
    // check for changes in key values that we need
    if (doc["print"].containsKey("stg_cur")){
      stg_cur = doc["print"]["stg_cur"];
      if (stg_cur != Previous_stg_cur) {
        par_change = true;
       }
    }
    if (doc["print"].containsKey("gcode_state")) {
      strcpy(gcode_state,doc["print"]["gcode_state"]);
      if (strcmp(gcode_state,Previous_gcode_state)) {
        par_change = true;
      }
    }
    if (doc["print"].containsKey("layer_num")) {
      layer_num = doc["print"]["layer_num"];
      if (layer_num != Previous_layer_num) {
        par_change = true;
        layer_num_change = true;
      }
    }

    // based on a change and the current values, should a photo be taken

    if ((layer_num_change) && (stg_cur == 0)) {
      take_photo = true;
      Serial.println("take photo now");
    }
   
    layer_num_change = false;   
    
    // update old values to current values
    if (par_change) {
      Serial.println(F("===== JSON Data ====="));
      serializeJsonPretty(doc, Serial);
      Serial.println(F("======================"));
      Serial.print("Previous_stg_cur : ");
      Serial.print(Previous_stg_cur);
      Serial.print(", Current stg_cur : ");
      Serial.print(stg_cur);
      Serial.print(", Previous_gcode_state : ");
      Serial.print(Previous_gcode_state);
      Serial.print(", Current gcode_state : ");
      Serial.print(gcode_state);
      Serial.print(", Previous_layer_num : ");
      Serial.print(Previous_layer_num);
      Serial.print(", Current layer_num : ");
      Serial.println(layer_num);
      par_change=false;
      Previous_stg_cur = stg_cur;
      strcpy(Previous_gcode_state,gcode_state);
      Previous_layer_num = layer_num;
    }


  }

}

// connect to home WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
   delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
}

// Connect to MQTT server
void connect() {
  IPAddress localIP;
  char lastNumber[5];
  uint8_t to_convert;

   while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");

    localIP = WiFi.localIP();
    strcpy(DeviceName,"BCamCon-");
    Serial.print("IP Address : ");
    Serial.println(localIP);
    Serial.print(" to append ");
    Serial.println(localIP[3]);
    to_convert = localIP[3];
    lastNumber[0] = (to_convert/100) +48;  // hundreds digit
    lastNumber[1] = (to_convert/10 - 10*(to_convert/100)) +48; //tens digit
    lastNumber[2] = (to_convert - 10*(to_convert/10)) +48;  // unit digit
    lastNumber[3] = '\0';  // terminate array
    strcat (DeviceName,lastNumber);

    Serial.print("ClientID : ");
    Serial.println(DeviceName);
    Serial.print("Username : ");
    Serial.println(BambuUsername);
    Serial.print("Password : ");
    Serial.println(BambuLANcode);
    if (mqttClient.connect(DeviceName, BambuUsername, BambuLANcode)) {
      Serial.println("connected");
      mqttClient.subscribe(mqttTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" will try again in 5 seconds");
      delay(5000);
    }
   }
  }

#ifdef ServoOutput
void ICACHE_RAM_ATTR button_pulleddown()  // Interrupt handler for test button to test servo movement
{
  take_photo = true;
}
#endif


 void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  wifiClient.setInsecure();
  setup_wifi();


  strcpy(mqttTopic,"device/");
  strcat(mqttTopic,BambuSN);
  strcat(mqttTopic,"/report");

  mqttClient.setBufferSize(10000);
  mqttClient.setServer(Bambu_IP, 8883);
  mqttClient.setCallback(callback);

  pinMode(ShutterOutput, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(ShutterOutput, HIGH); // Don't take photo
  digitalWrite(STATUS_LED, LOW); // not connected

  #ifdef ServoOutput
  cameraServo.attach(ServoOutput);
  cameraServo.write(midPos);
  pinMode(TestButton,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TestButton), button_pulleddown, FALLING);  // interrupt routine for testing servo movement
  #endif
}

void loop() {
  long shutterDelay;
  // put your main code here, to run repeatedly:
   if (!mqttClient.connected()) {
     bambuConnected = false;
     digitalWrite(STATUS_LED, LOW);
     if (WiFi.status() != WL_CONNECTED) { // check if WiFi is also disconnected
       setup_wifi();
     }
    connect();
   }
  if (take_photo) {
    digitalWrite(ShutterOutput, LOW); // take photo
    #ifdef ServoOutput
    cameraServo.write(midPos+maxServoMovement);
    #endif
    // delay 200ms to give camera control ESP time to react
    shutterDelay = millis();
    while (millis() - shutterDelay < 200) {
      yield();
    }
    digitalWrite(ShutterOutput, HIGH); // back to normal state
    #ifdef ServoOutput
    cameraServo.write(midPos);
    #endif
    take_photo = false;  
  }
  mqttClient.loop();
  yield();
}
