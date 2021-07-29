#include <LEDMatrixDriver.hpp>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266Ping.h>
#include <Ticker.h>

#include "credentials.h"
#include "settings.h"
#include "prototypes.h"
#include "OTA.h"
#include "sprites.h"

LEDMatrixDriver lmd(LEDMATRIX_SEGMENTS, LEDMATRIX_CS_PIN);
ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);
Ticker ticker;

void setup() {
  Serial.begin(115200);
  delay(300);
  lmd.setEnabled(true);
  lmd.setIntensity(2);   // 0 = low, 10 = high
  randomSeed(analogRead(0));

  Serial.println();
  Serial.println("Start ticker...");
  Serial.println();
  ticker.attach(1, loopCount);

  Serial.println("");
  startWIFI();
  handleOTASetup();
  startWebserver();
  blackOut();
}

void loop() { 
  //(re)connect wifi if not connected
  if (WiFiMulti.run() != WL_CONNECTED) {
    delay(1);
    startWIFI();
    return;
  }

  ArduinoOTA.handle();
  handlePing();
  server.handleClient();
  handleAnimation();
}

/**
   Start or reconnect the wifi
   by switching into an infinite loop
   as long as the connection is broken
*/
void startWIFI() {
  int loopcnt = 0;
  Serial.println("---");
  WiFi.mode(WIFI_STA);
  Serial.println("Connecting WIFI.");
  Serial.println("(Re)Connecting to Wifi-Network with following credentials:");
  Serial.print("SSID: ");
  //Serial.println(WifiCredentials[0].c_str());
  Serial.print("Key: ");
  //Serial.println(WifiCredentials[1].c_str());
  WiFi.hostname(espName);
  WiFiMulti.addAP(WifiCredentials[0].c_str(), WifiCredentials[1].c_str());

  while (WiFiMulti.run() != WL_CONNECTED) {
    loopcnt++;
    if (loopcnt < 10) {
      Serial.print(".");
    }
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address:");
  Serial.println(WiFi.localIP().toString());
}

/**
   Start webserver for handling
   incoming requests
*/
void startWebserver() {
  Serial.println("Starting HTTP-Server...");
  Serial.println("-- Registering routes.");
  server.on("/happy", HTTP_GET, []() {
    String sec = server.arg("s");
    startHappy(sec.toInt());
  });
  server.on("/angry", HTTP_GET, []() {
    String sec = server.arg("s");
    startAngry(sec.toInt());
  });
  server.on("/party", HTTP_GET, []() {
    String text = server.arg("text");
    String sec = server.arg("s");
    startText(text, sec.toInt());
  });
  server.on("/twinkle", HTTP_GET, []() {
    String sec = server.arg("s");
    startTwinkle(sec.toInt());
  });
  server.on("/blackout", HTTP_GET, []() {
    endAnimation();
  });
  server.onNotFound(handleRequestNotFound);
  Serial.println("-- Launching server ...");
  server.begin();
  Serial.println("-- DONE.");
}

/**
   Handles pinging (send alive)
   to server
*/
void handlePing() {
  if (pingCnt > handlePingSecs) {
    Serial.println("Start pinging.");
    bool result = Ping.ping("www.google.com");

    if (!result) {
      Serial.println("Ping failed!");
    }

    Serial.println("[HTTP] end ping-pong.");
    Serial.println("---");
    pingCnt = 0;
  }
}

/**
   Tick, tick, tick
*/
void loopCount() {
  loopCnt++;
  pingCnt++;

  if (animationActive) {
    activeCnt++;
  }
}

/**
   Start flickering on request
*/
void startHappy(int secs) {
  server.send(200, "text/plain", "I am happy!");
  if (secs == 0) secs = animationSecsFallback;
  animationPeriodSecs = secs;
  animationActive = true;
  eyeMode = "happy";
  Serial.println("Start being happy!");
}

void startAngry(int secs) {
  server.send(200, "text/plain", "I am angry!");
  if (secs == 0) secs = animationSecsFallback;
  animationPeriodSecs = secs;
  animationActive = true;
  eyeMode = "angry";
  Serial.println("Start being angry!");
}

void startTwinkle(int secs) {
  server.send(200, "text/plain", "I start twinkling!");
  if (secs == 0) secs = animationSecsFallback;
  animationPeriodSecs = secs;
  animationActive = true;
  eyeMode = "twinkle";
  Serial.println("Start being twinkle!");
}

void startText(String text, int secs) {
  server.send(200, "text/plain", "Starting with displaying text.");
  if (secs == 0) secs = animationSecsFallback;
  animationPeriodSecs = secs;
  animationActive = true;
  text.toCharArray(marqueeText, sizeof(marqueeText));
  eyeMode = "text";
  x = LEDMATRIX_WIDTH;
  lmd.clear();
  lmd.display();
  Serial.println("Start displaying text.");
  Serial.println(text);
}

void endAnimation() {
  server.send(200, "text/plain", "Closing my eyes. :(");
  blackOut();
}

void blackOut() {
  animationActive = false;
  activeCnt = 0;
  lmd.clear();
  lmd.display();
  Serial.println("BLACKOUT animation!");
}

void handleAnimation() {
  if (animationActive) {
    if (millis() % SPEED == 0){
      if (eyeMode.equals("text")) {
        handleMarquee();
        } else {
        if (eyeMovement == 0) {
          if (eyeMode.equals("happy")) {
            triangle();
          } else if(eyeMode.equals("angry")) {
            xing();
          } else if(eyeMode.equals("twinkle")){
            rect();
          }
          if (random(0, 200) > 195) {
            eyeMovement = 1;
          }
        } else {
          switch (eyeMovement) {
            case 1:
            case 3:
              blink();
              break;
            default:
              close();
              break;
          }
          eyeMovement++;
          if (eyeMovement == 4) eyeMovement = 0;
        }
      }
     
     if (activeCnt >= animationPeriodSecs) {
        blackOut();
      }
    }
  }
}

void blink() {
  if (eyeMode.equals("twinkle")) {
    switch (random(1)){
      case 0:
        drawSprite((byte*)&c, 0, 0, 8, 8);
        drawSprite((byte*)&d, 8, 0, 8, 8);
        break;
      case 1:
        drawSprite((byte*)&c, 16, 0, 8, 8);
        drawSprite((byte*)&d, 24, 0, 8, 8);
        break;
    }
  } else {
    drawSprite((byte*)&c, 0, 0, 8, 8);
    drawSprite((byte*)&d, 8, 0, 8, 8);
    drawSprite((byte*)&c, 16, 0, 8, 8);
    drawSprite((byte*)&d, 24, 0, 8, 8);
  }
  lmd.display();
}

void close() {
  drawSprite((byte*)&e, 0, 0, 8, 8);
  drawSprite((byte*)&f, 8, 0, 8, 8);
  drawSprite((byte*)&e, 16, 0, 8, 8);
  drawSprite((byte*)&f, 24, 0, 8, 8);
  lmd.display();
}

void xing() {
  drawSprite((byte*)&i, 0, 0, 8, 8);
  drawSprite((byte*)&j, 8, 0, 8, 8);
  drawSprite((byte*)&i, 16, 0, 8, 8);
  drawSprite((byte*)&j, 24, 0, 8, 8);
  lmd.display();
}

void triangle() {
  drawSprite((byte*)&a, 0, 0, 8, 8);
  drawSprite((byte*)&b, 8, 0, 8, 8);
  drawSprite((byte*)&a, 16, 0, 8, 8);
  drawSprite((byte*)&b, 24, 0, 8, 8);
  lmd.display();
}

void drawString(char* text, int len, int x, int y) {
  for (int idx = 0; idx < len; idx ++) {
    int c = text[idx] - 32;
    if (x + idx * 8  > LEDMATRIX_WIDTH) return;
    if (8 + x + idx * 8 > 0 ) drawSprite(font[c], x + idx * 8, y, 8, 8);
  }
}

void rect() {
  drawSprite((byte*)&k, 0, 0, 8, 8);
  drawSprite((byte*)&l, 8, 0, 8, 8);
  drawSprite((byte*)&k, 16, 0, 8, 8);
  drawSprite((byte*)&l, 24, 0, 8, 8);
  lmd.display();
}

void drawSprite(byte* sprite, int x, int y, int width, int height) {
  byte mask = B10000000;
  for(int iy = 0; iy < height; iy++) {
    for(int ix = 0; ix < width; ix++) {
      lmd.setPixel(x + ix, y + iy, (bool)(sprite[iy] & mask));
      mask = mask >> 1;
    }
    mask = B10000000;
  }
}

void handleMarquee() {
  int len = strlen(marqueeText);
  drawString(marqueeText, len, x, 0);
  lmd.display();
  if( --x < len * -8 ) x = LEDMATRIX_WIDTH;
}

/**
   Unknown Route
   Send teapot.
*/
void handleRequestNotFound() {
  server.send(418, "text/plain", "I'm a paintballmask. Pew. Pew.");
}
