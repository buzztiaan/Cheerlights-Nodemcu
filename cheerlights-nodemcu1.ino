#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
//needed for library

#include "Freepin_WS2801.h"

ESP8266WiFiMulti WiFiMulti;

const char* ssid = "PIEMELS";
const char* password = "PIEMELS";

uint8_t dataPin  = 12;    // Yellow wire
uint8_t clockPin = 14;    // Green wire

uint8_t buttonPin = 5;
int buttonState;

uint8_t button2Pin = 4;
int button2State;

bool dimness = false;
bool darkness = false;
bool pressed = false;

Freepin_WS2801 strip = Freepin_WS2801(50, dataPin, clockPin);

#define DEBUG 1

int fadetoR;
int fadetoG;
int fadetoB;

float curR = 0;
float curG = 0;
float curB = 0;

float deltaR;
float deltaG;
float deltaB;

int fadesteps = 100;

unsigned long previousMillis = 0;
const long interval = 7500;


// configure thingspeak endpoint
const char host[] = "api.thingspeak.com";
const String url = "/channels/1417/field/2/last.txt";

// Create a 24 bit color value from R,G,B
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}

String getValue(String data, char separator, int index)
{
 int found = 0;
  int strIndex[] = {
0, -1  };
  int maxIndex = data.length()-1;
  for(int i=0; i<=maxIndex && found<=index; i++){
  if(data.charAt(i)==separator || i==maxIndex){
  found++;
  strIndex[0] = strIndex[1]+1;
  strIndex[1] = (i == maxIndex) ? i+1 : i;
  }
 }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}


void makeitacolor(uint32_t color) {
  for (int i=0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void setup() {
  Serial.begin(115200);

  pinMode(2, OUTPUT); // built-in led on the ESP12 is connected to this
  digitalWrite(2, LOW); // we're gonna use it for statusblinks

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);

  WiFiMulti.addAP(ssid, password);

  if (DEBUG) Serial.println("connecting...yeey :)");

  digitalWrite(2, HIGH);
  
  strip.begin();
  makeitacolor(Color(0,0,0));
  strip.show();

}

void loop() {
  if((WiFiMulti.run() == WL_CONNECTED)) {
    if (DEBUG) Serial.println("connected...yeey :)");
    HTTPClient http;  

    http.begin(host, 80, url);

    Serial.println("[HTTP] GET...");
    // start connection and send HTTP header
    int httpCode = http.GET();
    if(httpCode) {
      // HTTP header has been send and Server response header has been handled
      //Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      // file found at server
      if(httpCode == 200) {
        unsigned long color;
        String line2 = http.getString();
        Serial.println(line2);
        String line = getValue(line2, '\n', 1);

        Serial.println(line[0]);
        Serial.println(line.length());

        if ((line[0] == '#') && (line.length() == 8)) {
          color = strtoul(line.c_str()+1, NULL, 16);
          if (DEBUG) Serial.println(color, HEX);
                       
          fadetoR = (color & 0xFF0000) >> 16;
          fadetoG = (color & 0x00FF00) >>  8;
          fadetoB = (color & 0x0000FF);
        }

        if (darkness) {
          fadetoR = 0;
          fadetoG = 0;
          fadetoB = 0;
        }
  
      if(0){
        while (((fadetoR + fadetoG + fadetoB) < 356) && !darkness) {

          fadetoR = fadetoR + 10;
          fadetoG = fadetoG + 10;
          fadetoB = fadetoB + 10;
  
          if (fadetoR > 255) fadetoR = 255;
          if (fadetoG > 255) fadetoG = 255;
          if (fadetoB > 255) fadetoB = 255;
        }
      }

        if (dimness) {
          fadetoR = map(fadetoR, 0, 255, 0, 127);
          fadetoG = map(fadetoG, 0, 255, 0, 127);
          fadetoB = map(fadetoB, 0, 255, 0, 127);
        }

        digitalWrite(2, HIGH);
        
        deltaR = (curR-fadetoR)/fadesteps;
        deltaG = (curG-fadetoG)/fadesteps;
        deltaB = (curB-fadetoB)/fadesteps;

        for (int steps=1; steps<=fadesteps; steps++) {
          curR -= deltaR;
          curG -= deltaG;
          curB -= deltaB;

          makeitacolor(Color(curR,curG,curB));
    
          delay(15);
        }
  
        curR = fadetoR;
        curG = fadetoG;
        curB = fadetoB;

        makeitacolor(Color(curR,curG,curB));
    
        while (millis() - previousMillis < interval) {
        // check button, update screen?
          buttonState = digitalRead(buttonPin);
          button2State = digitalRead(button2Pin);
        //  if (DEBUG) Serial.println(buttonState);
        if ((buttonState == LOW) && !pressed) {
          pressed = true;
          darkness = !darkness;
          if (DEBUG) Serial.println("Darkness toggled");
            digitalWrite(2, LOW);
          }

          if ((button2State == LOW) && !pressed) {
            pressed = true;
            dimness = !dimness;
            if (DEBUG) Serial.println("Dimness toggled");
              digitalWrite(2, LOW);
          }
      
            delay(15);
        }
        pressed = false;
        buttonState = 0;
        button2State = 0;
      }
    }
  }
}
