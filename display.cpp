/*Wifi Clock Lamp designed by Derrick Huynh
UCLA EE Student 
Designed from 12/26/2019 to ??/??/2020

Note: This is the TFT Display version of the code.
Features include: On/Off, Pair mode, custom colors, custom message, current time and date, current weather. 

Custom color must be manually done using a MQTT Websocket or MQTT phone app 
AND MUST BE IN HTML COLOR CODE - any other code (RGB etc.) WILL NOT WORK. 

A custom mobile app will probably not be developed :p but who knows
*/

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <NeoPixelBus.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <Time.h>
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <TFT_eSPI.h>

/*||*//////////TODO: Set up pins, wifi info, mqtt server//////////*||*/
/*||*/ #define neoPin 13 // Neopixel data pin                    /*||*/ 
/*||*/ #define numPixels 12 //number of Neopixels                /*||*/
/*||*/ #define touchPin 25 // Touch sensor                       /*||*/
/*||*/ int GMToffset = -8; //offset from GMT                     /*||*/
/*||*/                                                           /*||*/
/*||*/ const String myClientId = "Derrick";                      /*||*/ 
/*||*/ const String otherClientId = "Teresa";                    /*||*/ 
/*||*/ const String currentCity = "Los Angeles, US";             /*||*/ 
/*||*/                                                           /*||*/
/*||*/ const char* ssid = "UCLA_WEB";                            /*||*/ 
/*||*/ const char* password = "";                                /*||*/
/*||*/ const char* mqttServer = "tailor.cloudmqtt.com";          /*||*/ 
/*||*/ const int mqttPort = 12370;                               /*||*/ 
/*||*/ const char* mqttUser = "katjvuka";                        /*||*/ 
/*||*/ const char* mqttPassword = "OHqW_g6W5XcX";                /*||*/ 
/*||*/                                                           /*||*/
/*||*/ const String APIkey = "419e38fc466d19614e3a9e73b3f715fc"; /*||*/
/*||*/////////////////////////////////////////////////////////////*||*/

//Initalize library objects
WiFiClient wifiClient;
PubSubClient client(wifiClient);
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> neopixels(numPixels, neoPin);
TFT_eSPI tft = TFT_eSPI();  //initialize the TFT display, pin setup is in USER_SETUP.h file in library
WiFiUDP ntpUDP;   // UDP client
NTPClient timeClient(ntpUDP); // NTP client

//custom vars core to the lamp
bool touch = false; //bool to determine if led is on
const int smoothness = 2000; //smoothness determines the speed at which colors change or when lamp turns on/off

const char* ntpServer = "pool.ntp.org";

//define some commonly used colors
const RgbColor blueGray(51, 119, 255);
const RgbColor black(0,0,0);
const RgbColor pink(255,20,147);

//colors to store current and previous colors for restoration
RgbColor currColor = black;
RgbColor prevColor = black;

//global vars used for the display portion of lamp
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?q=";

//vars for storing current time
String dayStamp = "";
int hourStamp = 0;
int minStamp = 0;
bool PM = false;
unsigned long lastUpdatedTime = 0;

//vars for storing current temp
int currentTemperature = 0;
String weatherForecast = "";
unsigned long lastUpdatedWeather = 0;

//store last message recevied
String oldMsg = "Have a nice day!";
String currentMsg = "";

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

//turns off the LEDs, display is always on
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

//function calls the NTP server to update local time
void updateTimeAndDate() {
    while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  String formattedDate = timeClient.getFormattedDate();
  
  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  // Extract time
  hourStamp = (formattedDate.substring(splitT+1,splitT+3)).toInt();
  minStamp = (formattedDate.substring(splitT+4,splitT+6)).toInt();
  if(hourStamp > 12) { //check if AM or PM
    hourStamp -= 12;
    PM = true;
  }
  else {
    PM = false;
  }
}

//function calls the API to update current weather
void getWeather() {
  HTTPClient http;
  http.begin(endpoint + currentCity + "&APPID=" + APIkey);
  int httpGET = http.GET();
  if (httpGET > 0) { //Check for the returning code
      String httpGOT = http.getString();
      //search through the returned string to find certain keywords, temp and weather
      //this is super ugly code, if better way, pls let me know
      for(int index = 0; index + 5 < httpGOT.length(); index++)
      {
        if(httpGOT[index] == 't' && httpGOT[index+1] == 'e' && httpGOT[index+2] == 'm' && httpGOT[index+3] == 'p' && httpGOT[index+5] == ':')
        {
          currentTemperature = (httpGOT.substring(index + 6, index+9)).toInt();
          currentTemperature = ((currentTemperature -273) * 9 / 5) + 32; 
        }
      }
      for(int index = 0; index + 5 < httpGOT.length(); index++)
      {
        if(httpGOT[index] == 'm' && httpGOT[index+1] == 'a' && httpGOT[index+2] == 'i' && httpGOT[index+3] == 'n' && httpGOT[index+5] == ':')
        {
          //find index of the next double quote and make a substring of that from
          int quoteindex = httpGOT.substring(index + 7, httpGOT.length()).indexOf('"');
          weatherForecast = httpGOT.substring(index + 7, index + 7 + quoteindex);
          //occurs at first occurence of "main" so stop searching
          break;
        }
      }
    }
  else {
    Serial.println("Error on HTTP request");
  }
  http.end(); //Free the resources
}

//helper function to draw a string, as library provided function does not work
void drawString(int* xpos, int* ypos, String input, uint8_t font) {
  for(int i = 0; i < input.length(); i++)
  {
    *xpos += tft.drawChar(input[i], *xpos, *ypos, font); //write out current city
  }
}

//function outputs to the display, the Fonts are customizable to change the display
//note the display I am using is a 320x240 pixel board.
void drawTimeandWeather() {
  const uint8_t timeFont = 6;
  const uint8_t cityFont = 4;
  const uint8_t forecastFont = 4;
  const uint8_t messageFont = 4;
  int xpos = 0;
  int ypos = 0; // Top left corner of clock text, about half way down

  //draws a rounded rectangle around the entirety of display
  tft.drawRoundRect(0, 0, 320, 240, 12 ,TFT_CYAN);

  //draw time info
  xpos += tft.drawNumber(hourStamp, xpos, ypos, timeFont);             // Draw hours
  xpos += tft.drawChar(':', xpos, ypos, timeFont);
  if (minStamp < 10) xpos += tft.drawChar('0', xpos, ypos, timeFont); // Add minutes leading zero
  xpos += tft.drawNumber(minStamp, xpos, ypos, timeFont);             // Draw minutes
  ypos += 22;
  if(PM) { drawString(&xpos, &ypos, "PM", 2); }                       //display AM/PM
  else { drawString(&xpos, &ypos, "AM", 2); }
  
  //draw weather info
  ypos = 50;                                           //lower the y position to write out temp
  xpos = 0;
  xpos += tft.drawNumber(currentTemperature, xpos, ypos, 4) + 1;
  xpos += tft.drawChar('o', xpos, ypos, 1);
  xpos += tft.drawChar('F', xpos, ypos, 4);
  //draw city
  ypos += 25;
  xpos = 0;
  drawString(&xpos, &ypos, currentCity, cityFont);
  //draw forecast
  ypos += 50;
  xpos = 0;
  drawString(&xpos, &ypos, weatherForecast, forecastFont);

  //draw message info
  xpos = 100;
  ypos = 0;
  drawString(&xpos, &ypos, currentMsg, messageFont);
}

//MQTT Callback function that runs when a message is received from the subscribed topic.
void callback(char* topic, byte* payload, unsigned int length) {
  char *msg = (char *) payload;
  msg[length] = '\0';

  Serial.println("Message Received!: ");
  Serial.println(msg);
  Serial.println("On Topic: ");
  Serial.println(topic);
  Serial.println("At time:");
  Serial.println(hourStamp + ":" + minStamp);
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
  else if((String) topic == (myClientId+"/GMToffset")) {
    GMToffset = *payload;
    timeClient.setTimeOffset(GMToffset * 3600); //convert GMT hour offset to Epoch Time offset (seconds)
  }
  else if((String) topic == (otherClientId+"/msg")) {
    currentMsg = String(msg);
  } //if received message from online, update the display
}

void setup() {
  neopixels.Begin();
  neopixels.Show(); // initialize all pixels to "off"

  pinMode(touchPin, INPUT);

  Serial.begin(115200);
  Serial.setTimeout(500);// Set time out
  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  reconnect();

  //subscribe to other lamp's topics
  client.subscribe((otherClientId + "/on").c_str());
  client.subscribe((myClientId + "/color").c_str());
  client.subscribe((otherClientId + "/msg").c_str());
  client.subscribe((myClientId + "/time").c_str());

  Serial.println("Connecting to NTP Server");
  timeClient.begin(); // init NTP
  timeClient.setTimeOffset(GMToffset*3600);
  updateTimeAndDate();
  getWeather();

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(0x0000);
  tft.setTextSize(1);
  tft.setTextColor(0xFFE0, 0x0000);

  //when first turned on, turn on lamp to blueGray default
  setColor(blueGray);
  lastUpdatedTime = millis();
  lastUpdatedWeather = millis();
}

void loop() {
  //update the time every 30 seconds
  if(millis() - lastUpdatedTime > 30*1000)
  {
    updateTimeAndDate();
    lastUpdatedTime = millis();
    client.publish((myClientId+"/on").c_str(), "Y");
  }
  //update the weather every 30 minutes
  if(millis() - lastUpdatedWeather > 30*60*1000) 
  {
    getWeather();
    lastUpdatedWeather = millis();
  }
  drawTimeandWeather();

  //if the touch sensor is activated, alternate 
  if (digitalRead(touchPin)){
    if(touch == false) {
      setColor(blueGray);
      client.publish((myClientId+"/on").c_str(), "Y");
    }
    else if(touch == true) {
      turnOff();
      client.publish((myClientId+"/on").c_str(), "N");
    }
  } 

  client.loop();
  reconnect();
}
