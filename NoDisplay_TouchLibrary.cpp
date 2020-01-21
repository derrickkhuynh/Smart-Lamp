/*Wifi Clock Lamp designed by Derrick Huynh
UCLA EE Student 
Designed from 12/26/2019 to 1/20/2020

NOTES: THIS IS THE NON TFT VERSION. THIS IMPLEMENTS ONLY A ON/OFF FUNCTION OF LAMP, CUSTOMS COLORS, AND PAIR MODE.

This was programmed in PlatformIO IDE running on VS Code. 

This version is designed with the CapactiveSensor library in mind. Note this version is untested as
the capacitive library was obsolesced.

The on message is sent automatically by the program, but the custom color must be manually done using a MQTT Websocket or
MQTT phone app AND MUST BE IN HTML COLOR CODE - any other code (RGB etc.) WILL NOT WORK. 

A custom mobile app will probably not be developed :p but who knows
*/

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <NeoPixelBus.h>
#include <CapacitiveSensor.h>

/*||*//////////TODO: Set up pins, wifi info, mqtt server//////////*||*/
/*||*/ #define neoPin 13 // Neopixel data pin                    /*||*/ 
/*||*/ #define numPixels 12 //number of Neopixels                /*||*/
/*||*/ #define sendPin 8 // Touch sensor                         /*||*/ 
/*||*/ #define receivePin 14 // Touch sensor                     /*||*/ 
/*||*/                                                           /*||*/
/*||*/ const String myClientId = "";                             /*||*/ 
/*||*/ const String otherClientId = "";                          /*||*/ 
/*||*/                                                           /*||*/
/*||*/ const char* ssid = "";                                    /*||*/ 
/*||*/ const char* password = "";                                /*||*/
/*||*/ const char* mqttServer = "";                              /*||*/ 
/*||*/ const int mqttPort = 0;                                    /*||*/ 
/*||*/ const char* mqttUser = "";                                /*||*/ 
/*||*/ const char* mqttPassword = "";                            /*||*/ 
/*||*/////////////////////////////////////////////////////////////*||*/

//Initalize library objects
WiFiClient wifiClient;
PubSubClient client(wifiClient);
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> neopixels(numPixels, neoPin);
CapacitiveSensor cs_4_2 = CapacitiveSensor(sendPin, receivePin);

//custom vars core to the lamp
bool touch = false; //bool to determine if led is on
const int smoothness = 2000; //smoothness determines the speed at which colors change or when lamp turns on/off
unsigned long lastMillis; //var to store when it last published its on/off flag to MQTT server

//define some commonly used colors
const RgbColor blueGray(51, 119, 255);
const RgbColor black(0,0,0);
const RgbColor pink(255,20,147);

//colors to store current and previous colors for restoration
RgbColor currColor;
RgbColor prevColor = black;

//function begins wifi connection
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
    randomSeed(micros());
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

//function to connect to MQTT server
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(myClientId.c_str(),mqttUser,mqttPassword)) {
      Serial.println("connected");
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
}

//function to set the color of LEDs to input RgbColor
void setColor(RgbColor newColor) {
  prevColor = currColor;
  for(int i = 0; i < smoothness; i++) {
    RgbColor tempColor = RgbColor::LinearBlend(prevColor, newColor, (float(i)/ float(smoothness))); //slowly transition to the current Color
    for(int j = 0; j < neopixels.PixelCount(); j++) { //for each pixel, set it to the new color
      neopixels.SetPixelColor(j, tempColor);
    }
    if(neopixels.IsDirty()) {neopixels.Show(); } //TFT that tempcolor
  }
  currColor = newColor;
  touch = true;
}

//turns off the LEDs
void turnOff() {
  prevColor = currColor;
  for(int i = smoothness; i > 0; i--) {
    RgbColor tempColor = RgbColor::LinearBlend(black, currColor, (float(i)/ float(smoothness))); //slowly transition to the black from current Color
    for(int j = 0; j < neopixels.PixelCount(); j++) { //for each pixel, set it to the new color
      neopixels.SetPixelColor(j, tempColor);
    }
    if(neopixels.IsDirty()) {neopixels.Show(); } //display that tempcolor
  }
  currColor = black;
  touch = false;
}

//MQTT Callback function that runs when a message is received from the subscribed topic.
void callback(char* topic, byte* payload, unsigned int length) {
  char *msg = (char *) payload;
  msg[length] = '\0';

  Serial.println("Message Received!: ");
  Serial.println(msg);
  Serial.println("On Topic: ");
  Serial.println(topic);
  if((String) topic == (otherClientId+"/on")) { //if the topic is the other lamp's on/off flag
    if(touch && (*msg == 'Y')) { //if my lamp and her lamp are both on, set color to pink
      setColor(pink);
    }
    if(touch && (*msg == 'N')) { //if her lamp turned off, set color to the prev color
      //fix bug where it would turn off, or revert to pink if 'N' message was sent twice
      //color falls back to standard blueGray if wack stuff occurs
      if(prevColor == black || prevColor == pink) {
        prevColor = blueGray;
      }
      setColor(prevColor);
    }
  }
  else if((String) topic == (myClientId+"/color")) { //receive HTML color code form websocket and update the color
    //converts the 0xABCDEF string to a double, casts to uint32_t and makes a HTMLColor out of that
    uint32_t htmlColor = String((msg)).toDouble();
    setColor(RgbColor(HtmlColor(htmlColor)));
  }
}

void setup() {
  neopixels.Begin();
  neopixels.Show(); // initialize all pixels to "off"

  cs_4_2.set_CS_AutocaL_Millis(10000); // (0xFFFFFFFF) to turn off sensor autocalibrate

  Serial.begin(115200);
  Serial.setTimeout(500);// Set time out
  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  reconnect();

  //subscribe to other lamp's topics
  client.subscribe((otherClientId + "/on").c_str());
  client.subscribe((myClientId + "/color").c_str());

  //when first turned on, turn on lamp to blueGray default
  setColor(blueGray);
  currColor = blueGray;
  lastMillis = millis();
}

void loop() {
  long sense = cs_4_2.capacitiveSensor(30);
  Serial.print(sense);
  Serial.print("\t");


  //if the touch sensor is activated, alternate 
  if (sense > 250) {
    if(touch == false) {
      setColor(blueGray);
      client.publish((myClientId+"/on").c_str(), "Y");
    }
    else if(touch == true) {
      turnOff();
      client.publish((myClientId+"/on").c_str(), "N");
    }
  } 

  //if 30 secs elapsed since last update to MQTT server
  if(millis() - lastMillis == 30*1000)
  {
    if(touch) {
      client.publish((myClientId+"/on").c_str(), "Y");
    }
    else {
      client.publish((myClientId+"/on").c_str(), "N");
    }
  }
  client.loop();
  reconnect();
}
