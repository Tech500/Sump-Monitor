/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////
//
// Version 8.0 Added Alexa announcement with stop command for alert 04/18/2026 @ 20:04 EDT
//
// Alert cooldown, counter, All Clear, boot fix 04/09/2026
//
// Adding HTTP over TLS (HTTPS) 09/05/2020 @ 13:48 EDT
//
// ESP8266 --Internet Sump Pit Monitor, Datalogger and Dynamic Web Server 05/16/2020 @ 12:45 EDT
//
// NTP time routines optimized for speed by schufti --of ESP8266.com
//
// Project uses ESP8266 Development board: NodeMCU
//
// readFiles and readFile functions by martinayotte of ESP8266 Community Forum
//
// Previous projects: by tech500 on https://github.com/tech500
//
// Project is Open-Source; requires ESP8266 based development board.
//
// ESP-32-S3 Development Board ESP-32-S3 Module with ESP-1-N16R8   Memry size required!!!
//
// Note: Uses ESP32-S3 by Espressif Community version 3.3.7 for "Arduino IDE Nightbuild 2.3.8."
//
/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////

/*
Program uses ultrasonic sensing to measure distance of water from top of sump pit.

Features:

1. Ultrasonic distance to water measuring,
2. Log file of measurements at 15 minute intervals.
3. Web interface display last update of water distance to top of sump pit.
4. Graph of distance to top and time of data point.
5. FTP for file maintenance; should it be needed.
6. Automatic deletion of log files. Can be daily or weekly
7. OTA Over-the-air firmware updates.
8. Sends email and SMS alert at predetermined height of water to top of sump pit.
9. Alert cooldown with phase 1 (5 min) and phase 2 (10 min) intervals.
10. Alert counter included in subject line.
11. All Clear notification when water returns to normal.
*/

// ********************************************************************************
// ********************************************************************************
//
// See individual library downloads for each library license.
//
// Following code was developed with the Adafruit CC300 library, "HTTPServer" example.
// and by merging library examples, adding logic for Sketch flow.
//
// *********************************************************************************
// *********************************************************************************

#include <WebServer.h>

#define WEBSERVER_H

#define ELEGANTOTA_USE_ASYNC_WEBSERVER 1

#include "Arduino.h"
#include <EMailSender.h>  //https://github.com/xreef/EMailSender
#include <WiFi.h>         //Part of ESP8266 Board Manager install __> Used by WiFi to connect to network
#include <WiFiUdp.h>
#include <WebSocketsClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>  //https://github.com/me-no-dev/ESPAsyncWebServer
#include <WebServer.h>          // needed by tzapu WiFiManager
#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager
#include <ElegantOTA.h>
#include <WiFiClientSecure.h>
#include <SD.h>
#include <FS.h>
#include <LittleFS.h>
#include <FTPServer.h>
#define BAUDRATE 74880
#include <HTTPClient.h>  //Part of ESP32 Board Manager install --> Used for Domain web interface
#include <sys/time.h>    // struct timeval --> Needed to sync time
#include <time.h>        // time() ctime() --> Needed to sync time
#include <Ticker.h>      //Part of ESP32 Board Manager install -----> Used for watchdog ISR
#include <Wire.h>        //Part of the Arduino IDE software download --> Used for I2C Protocol
#include <SinricPro.h>
#include "SinricProContactsensor.h"
#include <SinricProWebsocket.h>
#include "variableInput.h"  //Packaged with project download. Provides editing options; without having to search 2000+ lines of code.

WiFiManager wm;  // global WiFiManager instance -- do NOT move into a function

// Reset button pin -- wire a momentary pushbutton between this pin and GND.
// Hold for 3 seconds at boot to wipe saved credentials and reopen the
// setup portal. GPIO 0 is shown here as an example; change to any free
// pin that does not conflict with trigPin, echoPin, SDA, SCL, or the
// online LED pin.
#define WIFI_RESET_PIN 0

uint8_t connection_state = 0;
uint16_t reconnect_interval = 10000;

bool sinricReady = false;  // add to globals

void setupSinricPro() {

  // Three contact sensor devices
  SinricProContactsensor &myHighWater = SinricPro[HIGHWATER];
  SinricProContactsensor &myFlooding = SinricPro[FLOODING];
  SinricProContactsensor &myAllClear = SinricPro[ALLCLEAR];

  // Connection callbacks
  SinricPro.onConnected([]() {
    Serial.printf("Connected to SinricPro\r\n");
  });
  SinricPro.onDisconnected([]() {
    Serial.printf("Disconnected from SinricPro\r\n");
  });

  SinricPro.begin(APP_KEY, APP_SECRET);
}

EMailSender emailSend(SENDER_EMAIL, SENDER_PASS);  //gmail email address and gmail application password <---edit variableInput.h

#define nosensor 0.00

//How to create application password
//https://www.lifewire.com

WiFiClientSecure client;

#define online 37  //pin for online LED indicator

#define SPIFFS LittleFS

FTPServer ftpSrv(LittleFS);

char *filename;
String fn;
String uncfn;
String urlPath;

char str[] = { 0 };

String fileRead;

char MyBuffer[17];

String PATH;

#import "index1.h"  //Weather HTML; do not remove
#import "index2.h"  //SdBrowse HTML; do not remove
#import "index3.h"  //HTML; do not remove
#import "index4.h"  //HTML; do not remove

IPAddress ipREMOTE;

Ticker secondTick;

volatile int watchdogCounter;
int totalwatchdogCounter;

void ISRwatchdog() {
  watchdogCounter++;
}

WiFiUDP UDP;
unsigned int localUDPport = 123;  // local port to listen for UDP packets
unsigned char incomingPacket[255];
unsigned char replyPacket[] __attribute__((section(".dram0.data"))) = "Hi there! Got the message :-)";

#define NTP0 "us.pool.ntp.org"
#define NTP1 "time.nist.gov"

#define TZ "EST+5EDT,M3.2.0/2,M11.1.0/2"
RTC_DATA_ATTR char savedTZ[] = "EST+5EDT,M3.2.0/2,M11.1.0/2";

int DOW, MONTH, DATE, YEAR, HOUR, MINUTE, SECOND;

char strftime_buf[64];

String dtStamp(strftime_buf);
String lastUpdate;

int lc = 0;
time_t tnow = 0;

int flag = 0;

float temp(NAN), hum(NAN), pres(NAN), millibars, fahrenheit, RHx, T, heat_index, dew, dew_point, atm;

float HeatIndex, DewPoint, temperature, humidity, TempUnit_Fahrenheit;

int count = 0;

int error = 0;

unsigned long delayTime;

int started;  //Used to tell if Server has started

#define BUFSIZE 64  //Size of read buffer for file download

////////////////////////// AsyncWebServer ////////////
AsyncWebServer serverAsync(80);      //PORT
AsyncWebSocket ws("/ws");            // access at ws://[esp ip]/ws
AsyncEventSource events("/events");  // event source (Server-Sent events)
//////////////////////////////////////////////////////

#define LISTEN_PORT 80

const int ledPin = 14;  //ESP8266, RobotDyn WiFi D1

int trigPin = 14;  //Orange wire
int echoPin = 12;  //White wire
int pingTravelTime;
float pingTravelDistance;
float distanceToTarget;
int dt = 50;

int reconnect;

// ===================================================
// Alert globals --Version 3.0
// ===================================================
bool didFire = false;
bool alertFlag = false;           // true when water is currently high/flooding
bool alertAcknowledged = false;   // true when user acknowledges via web
int alertCount = 0;               // number of alerts sent this event
unsigned long lastAlertTime = 0;  // millis() of last alert sent
bool wasAlerting = false;

bool floodFlag = false;  // true when sensor reads 0.5

String condition = "";
String lastCondition = "";
String currentCondition = "";

// Latching web banner -- stays set until user clears via web page
// even after water returns to normal. Gives morning-after visibility.
bool alertLatched = false;      // true once any alert fires
bool alertWaterNormal = false;  // true when water returned normal but not yet cleared

// Phase 1: first 3 alerts every 5 minutes
const unsigned long PHASE1_INTERVAL = 300000UL;

// Phase 2: after 3 alerts, every 10 minutes
const unsigned long PHASE2_INTERVAL = 600000UL;

// ===================================================
// Sinric Pro globals
// ===================================================
bool myPowerState = true;
bool lastSinricState = false;
#define pass 2

void setup(void) {

  Serial.begin(115200);
  while (!Serial) {};

  randomSeed(esp_random());

  Serial.println("Starting Sump Monitor");

  Wifi_Start();

  secondTick.attach(1, ISRwatchdog);  //watchdog ISR triggers every 1 second

  pinMode(online, OUTPUT);  //Set pinMode to OUTPUT for online LED

  pinMode(4, INPUT_PULLUP);  //Set input (SDA) pull-up resistor on GPIO_0

  pinMode(trigPin, OUTPUT);

  pinMode(echoPin, INPUT);

  Wire.setClock(20000000);
  Wire.begin(SDA, SCL);

  configTime(0, 0, NTP0, NTP1);
  setenv("TZ", TZ, 3);  // Indianapolis, Indiana
  tzset();
  syncNTP();

  delay(500);

  setupSinricPro();

  Serial.println("Waiting for SinricPro...");
  while (!SinricPro.isConnected()) {
    SinricPro.handle();
    delay(100);
  }

  Serial.println("SinricPro connected, ready.");

  LittleFS.begin(true);

  Serial.println("LittleFS opened!");
  Serial.println("");

  bool fsok = LittleFS.begin();
  Serial.printf_P(PSTR("FS init: %s\n"), fsok ? PSTR("ok") : PSTR("fail!"));

  // setup the ftp server with username and password
  ftpSrv.begin(F(ftpUser), F(ftpPassword));

  //LittleFS.format();

  serverAsync.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    PATH = "/FAVICON";
    if (!flag == 1) {
      request->send(SPIFFS, "/favicon.png", "image/png");
    }
  });

  serverAsync.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    PATH = "/";
    accessLog();
    ipREMOTE = request->client()->remoteIP();
    serverAsync.begin();
    if (!flag == 1) {
      request->send_P(200, PSTR("text/html"), HTML1, processor1);  //index1.h
    }
    end();
  });

  serverAsync.on("/sump", HTTP_GET, [](AsyncWebServerRequest *request) {
    PATH = "/sump";
    accessLog();
    ipREMOTE = request->client()->remoteIP();
    if (!flag == 1) {
      request->send_P(200, PSTR("text/html"), HTML1, processor1);
    }
    end();
  });

  serverAsync.on("/SdBrowse", HTTP_GET, [](AsyncWebServerRequest *request) {
    PATH = "/SdBrowse";
    accessLog();
    if (!flag == 1) {
      request->send_P(200, PSTR("text/html"), HTML2, processor2);  //index2.h
    }
    end();
  });

  serverAsync.on("/graph", HTTP_GET, [](AsyncWebServerRequest *request) {
    PATH = "/graph";
    accessLog();
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML3, processor3);
    response->addHeader("Server", "ESP Async Web Server");
    if (!flag == 1) {
      request->send(response);
    }
    end();
  });

  serverAsync.on("/Show", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!flag == 1) {
      request->send_P(200, PSTR("text/html"), HTML4, processor4);
    }
  });

  serverAsync.on("/get-file", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, fn, "text/txt");
    PATH = fn;
    accessLog();
    end();
  });

  serverAsync.on("/redirect/internal", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/Show");  //recevies HTML request to redirect
  });

  // Acknowledge -- pause alerts
  serverAsync.on("/acknowledge", HTTP_POST,
                 [](AsyncWebServerRequest *request) {
                   alertAcknowledged = true;
                   Serial.println("Alert acknowledged by user!");
                   request->redirect("/sump");
                 });

  // Resume -- restart alerts
  serverAsync.on("/resume", HTTP_POST,
                 [](AsyncWebServerRequest *request) {
                   alertAcknowledged = false;
                   lastAlertTime = 0;  // fire immediately on next cycle
                   Serial.println("Alerts resumed by user!");
                   request->redirect("/sump");
                 });

  // Clear latching web banner -- user confirms they saw the alert
  serverAsync.on("/clear-latch", HTTP_POST,
                 [](AsyncWebServerRequest *request) {
                   alertLatched = false;
                   alertWaterNormal = false;
                   Serial.println("Alert banner cleared by user.");
                   request->redirect("/sump");
                 });

  serverAsync.onNotFound(notFound);

  serverAsync.begin();

  started = 1;  //Server started

  getDateTime();
  lastUpdate = dtStamp;  // populate immediately at boot
  condition = ALLCLEAR;

  delay(500);

  ElegantOTA.begin(&serverAsync);
  Serial.println("ElegantOTA ready -- browse to http://<ip>/update to flash.");

  Serial.println("*** Ready ***\n\n");
}

// How big our line buffer should be. 100 is plenty!
#define BUFFER 100

void loop() {

  int flag;

  delay(1);

  // Feed watchdog only when WiFiManager portal is not active.
  if (!wm.getConfigPortalActive()) {
    watchdogCounter = 0;
  }

  wm.process();        // serves WiFiManager portal until credentials saved
  ElegantOTA.loop();   // handles OTA upload progress and reboot callbacks
  SinricPro.handle();  //Handles Alexa alerts

  int packetSize = UDP.parsePacket();

  if (packetSize) {
    Serial.printf("Received %d bytes from %s, port %d\n", packetSize, UDP.remoteIP().toString().c_str(), UDP.remotePort());
    int len = UDP.read(incomingPacket, 255);
    if (len > 0) {
      incomingPacket[len] = 0;
    }
    Serial.printf("UDP packet contents: %s\n", incomingPacket);

    // send back a reply, to the IP address and port we got the packet from
    UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());
    UDP.write(replyPacket, sizeof(replyPacket));
    UDP.endPacket();
  }

  if (WiFi.status() != WL_CONNECTED) {

    getDateTime();

    //Open a "WIFI.TXT" for appended writing.
    File logFile = SPIFFS.open("/WIFI.TXT", "a");

    if (!logFile) {
      Serial.println("File: '/WIFI.TXT' failed to open");
    } else {
      logFile.print("WiFi Disconnected: ");
      logFile.println(dtStamp);
      logFile.close();

      WiFi.waitForConnectResult();

      reconnect = 1;
    }
  }

  if ((WiFi.status() == WL_CONNECTED) && reconnect == 1) {

    watchdogCounter = 0;  //Resets the watchdogCount

    File logFile = SPIFFS.open("/WIFI.TXT", "a");

    if (!logFile) {
      Serial.println("File: '/WIFI.TXT' failed to open");
    } else {
      getDateTime();
      logFile.print("WiFi Reconnected: ");
      logFile.println(dtStamp);
      logFile.close();

      reconnect = 0;
    }
  }

  if (started == 1) {

    getDateTime();

    File log = LittleFS.open("/SERVER.TXT", "a");

    if (!log) {
      Serial.println("file: '/SERVER.TXT' open failed");
    }

    log.print("Started Server: ");
    log.println(dtStamp) + " ";
    log.close();
  }

  started = 0;

  if (watchdogCounter > 45) {

    totalwatchdogCounter++;

    Serial.println("watchdog Triggered");

    Serial.print("Watchdog Event has occurred. Total number: ");
    Serial.println(watchdogCounter / 80);

    File log = LittleFS.open("/WATCHDOG.TXT", "a");

    if (!log) {
      Serial.println("file 'WATCHDOG.TXT' open failed");
    }

    getDateTime();

    log.print("Watchdog Restart: ");
    log.print(" ");
    log.print(dtStamp);
    log.println("");
    log.close();

    Serial.println("Watchdog Restart " + dtStamp);

    WiFi.disconnect();

    ESP.restart();
  }

  ftpSrv.handleFTP();

  getDateTime();

  //Executes every 5 minute routine
  if ((MINUTE % 5 == 0 && SECOND == 0) && !didFire) {
    didFire = true;

    ultrasonic();
    
    handleAlexaReporting();
  }

  // Reset flag when no longer on a 5-minute mark
  if (MINUTE % 5 != 0) {
    didFire = false;
  }

  flag = 0;

  //Collect "LOG.TXT" Data for one day
  if ((HOUR == 23) && (MINUTE == 57) && (SECOND == 0)) {
    newDay();
  }
}

String buildAlertButtonHTML() {

  // FLOODING -- red, no auto-clear
  if (floodFlag) {
    return String(
      "<p class='status-flood'>&#x1F6A8; FLOODING DETECTED -- IMMEDIATE ATTENTION REQUIRED</p>"
      "<br>"
      "<form action='/acknowledge' method='POST'>"
      "<button class='btn-alert'>&#x26A0;&#xFE0F; Acknowledge</button>"
      "</form>");
  }

  // HIGH WATER -- active, not acknowledged
  if (alertFlag && !alertAcknowledged) {
    return String(
      "<p class='status-alert'>&#x26A0;&#xFE0F; HIGH WATER ALERT</p>"
      "<form action='/acknowledge' method='POST'>"
      "<button class='btn-alert'>&#x26A0;&#xFE0F; Acknowledge Alert</button>"
      "</form>");
  }

  // HIGH WATER -- active, acknowledged
  if (alertFlag && alertAcknowledged) {
    return String(
      "<p class='status-ack'>&#x26A0;&#xFE0F; Alert Acknowledged -- Water Still High</p>"
      "<br>"
      "<form action='/resume' method='POST'>"
      "<button class='btn-resume'>&#x1F514; Resume Alerts</button>"
      "</form>");
  }

  // LATCHED -- water returned to normal but not yet cleared by user
  if (alertLatched && alertWaterNormal) {
    return String(
      "<p class='status-amber'>&#x26A0;&#xFE0F; ALERT -- Water has returned to Normal</p>"
      "<p class='status-amber'>All Clear sent. Total alerts: "
      + String(alertCount) + "</p>"
                             "<br>"
                             "<form action='/clear-latch' method='POST'>"
                             "<button class='btn-resume'>&#x2705; Clear Alert Banner</button>"
                             "</form>");
  }

  // NORMAL -- no active or latched alert
  return "<p class='status-normal'>&#x2705; Water Level Normal</p>";
}

String processor1(const String &var) {

  getDateTime();

  if (var == F("TOP")) {
    char dist[10];
    float safeValue = distanceToTarget;
    dtostrf(safeValue, 9, 1, dist);
    return String(dist);
  }

  if (var == F("DATE")) return dtStamp;
  if (var == F("LASTUPDATE")) return lastUpdate;
  if (var == F("CLIENTIP")) return ipREMOTE.toString();
  if (var == F("ALERTBUTTON")) return buildAlertButtonHTML();

  return String();
}

String processor2(const String &var) {
  String str;

  if (!LittleFS.begin()) {
    Serial.println("LittleFS failed to mount !");
    return String();
  }

  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) {
    Serial.println("LittleFS failed to open root directory!");
    return String();
  }

  String lastFileName = "";

  File file = root.openNextFile();
  while (file) {
    String file_name = file.name();

    if (file_name.startsWith("LOG") || file_name.startsWith("ALERT")) {
      lastFileName = file_name;

      str += "<a href=\"/";    // Add leading slash -- absolute path
      str += file_name;
      str += "\">";
      str += file_name;
      str += "</a>";
      str += " ";
      str += file.size();
      str += "<br>\r\n";
    }

    file = root.openNextFile();
  }

  root.close();

  if (var == F("URLLINK"))
    return str;

  if (var == F("LINK"))
    return linkAddress;

  if (var == F("FILENAME"))
    return lastFileName;

  return String();
}

String processor3(const String &var) {
  if (var == F("LINK"))
    return linkAddress;
  return String();
}

String processor4(const String &var) {

  //index4.h

  if (var == F("FN"))
    return fn;

  if (var == F("LINK"))
    return linkAddress;

  return String();
}

void accessLog() {

  digitalWrite(online, HIGH);  //turn on online LED indicator

  getDateTime();

  String ip1String = "10.0.0.146";                 //Host ip address
  String ip2String = ipREMOTE.toString().c_str();  //client remote IP address

  Serial.println("");
  Serial.println("Client connected: " + dtStamp);
  Serial.print("Client IP: ");
  Serial.println(ip2String);
  Serial.print("Path: ");
  Serial.println(PATH);
  Serial.println(F("Processing request"));

  File logFile = SPIFFS.open(Restricted, "a");

  if (!logFile) {
    Serial.println("File 'ACCESS.TXT'failed to open");
  }

  if ((ip1String == ip2String) || (ip2String == "0.0.0.0") || (ip2String == "(IP unset)")) {
    logFile.close();
  } else {
    Serial.println("Log client ip address");
    logFile.print("Accessed: ");
    logFile.print(dtStamp);
    logFile.print(" -- Client IP: ");
    logFile.print(ip2String);
    logFile.print(" -- ");
    logFile.print("Path: ");
    logFile.print(PATH);
    logFile.println("");
    logFile.close();
  }
}

void end() {
  delay(1000);
  client.flush();
  client.stop();
  digitalWrite(online, LOW);  //turn-off online LED indicator
  getDateTime();
  Serial.println("Client closed: " + dtStamp);
  delay(1);
}

void fileStore() {
  File log = LittleFS.open("/LOG.TXT", "a");

  if (!log) {
    Serial.println("file open failed");
  }

  String logname;
  logname = "/LOG";
  logname += MONTH;
  logname += DATE;
  logname += ".TXT";
  LittleFS.rename("/LOG.TXT", logname.c_str());
  log.close();
}

String getDateTime() {
  struct tm *ti;

  setenv("TZ", savedTZ, 1);
  tzset();
  tnow = time(nullptr) + 1;
  ti = localtime(&tnow);
  DOW = ti->tm_wday;
  YEAR = ti->tm_year + 1900;
  MONTH = ti->tm_mon + 1;
  DATE = ti->tm_mday;
  HOUR = ti->tm_hour;
  MINUTE = ti->tm_min;
  SECOND = ti->tm_sec;

  strftime(strftime_buf, sizeof(strftime_buf), "%a , %m/%d/%Y , %H:%M:%S %Z", localtime(&tnow));
  dtStamp = strftime_buf;
  return (dtStamp);
}

void handleAlexaReporting() {

  // Exit if the state hasn't changed since the last 5-minute check
  if (condition == lastCondition) {
    Serial.println("No Condition change");
    return;
  }

  // Only report to Alexa when there is a state transition
  masterAlexa(condition);

  lastCondition = condition;
}


void links() {
  client.println("<br>");
  client.println("<h2><a href='http://' + publicIP + ':' + PORT + '/SdBrowse >File Browser</a></h2><br>'");
  client.print("Client IP: ");
  client.print(client.remoteIP().toString().c_str());
  client.println("</body>");
  client.println("</html>");
}

void logtoSD() {
  getDateTime();

  int tempy;
  String Date;
  String Month;

  tempy = (DATE);
  if (tempy < 10) {
    Date = ("0" + (String)tempy);
  } else {
    Date = (String)tempy;
  }

  tempy = (MONTH);
  if (tempy < 10) {
    Month = ("0" + (String)tempy);
  } else {
    Month = (String)tempy;
  }

  String logname;
  logname = "/LOG";
  logname += Month;
  logname += Date;
  logname += ".TXT";

  File log = LittleFS.open(logname.c_str(), "a");

  if (!log) {
    Serial.println("file 'LOG.TXT' open failed");
  }

  delay(500);

  log.print(condition);
  log.print(" ,  ");
  log.print(distanceToTarget);
  log.print(" inches,  ");
  log.print(dtStamp);
  log.println();

  Serial.println("");
  Serial.print(condition);
  Serial.print(",  ");
  Serial.print(distanceToTarget, 1);
  Serial.println("  inches,  Data written to " + logname + ",  " + dtStamp);

  log.close();
}

void masterAlexa(String condition) {

  getDateTime();
  Serial.println(dtStamp);

  SinricProContactsensor &myHighWater = SinricPro[HIGHWATER];
  SinricProContactsensor &myFlooding = SinricPro[FLOODING];
  SinricProContactsensor &myAllClear = SinricPro[ALLCLEAR];

  if (condition == "FLOODING") {
    //myFlooding.sendContactEvent(true);
    myHighWater.sendContactEvent(false);
    myAllClear.sendContactEvent(false);
    Serial.println("masterAlexa: FLOODING Triggered");
    //sendFlood();
    Serial.println(condition + "  Condition changed");
  } else if (condition == "HIGHWATER") {
    //myHighWater.sendContactEvent(true);
    myFlooding.sendContactEvent(false);
    myAllClear.sendContactEvent(false);
    Serial.println("masterAlexa: HIGH WATER Triggered");
    //sendAlert();
    Serial.println(condition + "  Condition changed");
  } else if (condition == "ALLCLEAR"){
    //myAllClear.sendContactEvent(true);
    myHighWater.sendContactEvent(false);
    myFlooding.sendContactEvent(false);
    Serial.println("masterAlexa: ALL CLEAR Triggered");
    //sendAllClear();
    Serial.println(condition + "  Condition changed");
  }
  logtoSD();
}

void newDay() {

  if ((DOW) == 6) {
    fileStore();
  }

  File log = LittleFS.open("/LOG.TXT", "a");

  if (!log) {
    Serial.println("file open failed");
  }

  EMailSender::EMailMessage message;
  message.subject = String(ALERT_TAG) + " Sump Monitor -- Daily Heartbeat";
  message.message = "Sump Monitor is online and operational. " + dtStamp;

  EMailSender::Response resp = emailSend.send("ab9nq.william@gmail.com", message);

  Serial.println("Heartbeat sending status: ");
  Serial.println(resp.status);
  Serial.println(resp.code);
  Serial.println(resp.desc);

  // Reset alert state at start of new day
  alertFlag = false;
  alertAcknowledged = false;
  alertCount = 0;
  alertLatched = false;
  alertWaterNormal = false;
  floodFlag = false;
}

void syncNTP() {
  Serial.print("Waiting for NTP sync");
  int attempts = 0;
  while (time(nullptr) < 1000000000UL && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (time(nullptr) > 1000000000UL) {
    Serial.println(" NTP synced!");
  } else {
    Serial.println(" NTP sync failed -- UTC until next sync.");
  }
}

String notFound(AsyncWebServerRequest *request) {

  digitalWrite(online, HIGH);

  if (!request->url().endsWith(F(".TXT"))) {
    request->send(404);
  } else {
    if (request->url().endsWith(F(".TXT"))) {
      int fnsstart = request->url().lastIndexOf('/');
      fn = request->url().substring(fnsstart);
      uncfn = fn.substring(1);
      urlPath = linkAddress + "/" + uncfn;
    }
  }

  request->redirect("/Show");
  digitalWrite(online, LOW);
  return fn;
}

void sendFlood() {

  alertCount++;

  EMailSender::EMailMessage message;
  message.subject = String(ALERT_TAG) + " FLOODING -- IMMEDIATE ATTENTION REQUIRED -- Alert #" + String(++alertCount);
  message.message = "FLOODING DETECTED -- Sensor may be submerged! "
                    "/// Alert number: "
                    + String(alertCount) + " /// " + dtStamp;

  EMailSender::Response resp = emailSend.send(ALERT_SMS, message);
  emailSend.send(ALERT_EMAIL, message);
  emailSend.send(ALERT_EMAIL2, message);

  getDateTime();

  Serial.println("FLOOD Alert #" + String(alertCount) + " sent: " + dtStamp);

  File alertLog = LittleFS.open("/ALERT.TXT", "a");
  if (alertLog) {
    alertLog.print("FLOODING Alert #");
    alertLog.print(alertCount);
    alertLog.print(" -- Distance: ");
    alertLog.print(distanceToTarget, 1);
    alertLog.print(" inches -- ");
    alertLog.print(dtStamp);
    alertLog.print(" -- Email status: ");
    alertLog.println(resp.status);
    alertLog.close();
  }
}

void sendAlert() {  //HIGHWATER

  alertCount++;

  EMailSender::EMailMessage message;
  message.subject = String(ALERT_TAG) + " Warning High Water!!! Alert #" + String(alertCount);
  message.message = "Sump Pump /// Alert high water level! "
                    "/// Alert number: "
                    + String(alertCount) + " /// " + dtStamp;

  EMailSender::Response resp = emailSend.send(ALERT_SMS, message);
  emailSend.send(ALERT_EMAIL, message);

  Serial.println("Alert #" + String(alertCount) + " sent: " + dtStamp);

  delay(100);
  File alertLog = LittleFS.open("/ALERT.TXT", "a");
  if (!alertLog) {
    Serial.println("File '/ALERT.TXT' open failed");
  } else {
    alertLog.print("Alert #");
    alertLog.print(alertCount);
    alertLog.print(" -- High Water! Distance: ");
    alertLog.print(distanceToTarget, 1);
    alertLog.print(" inches -- ");
    alertLog.print(dtStamp);
    alertLog.print(" -- Email status: ");
    alertLog.print(resp.status);
    alertLog.println();
    alertLog.close();
    Serial.println("Alert event logged to /ALERT.TXT");
  }
}

void sendAllClear() {  //ALLCLEAR

  EMailSender::EMailMessage message;
  message.subject = String(ALERT_TAG) + " Sump Pit -- All Clear";
  message.message = "Water level has returned to normal. "
                    "Total alerts sent: "
                    + String(alertCount) + " /// " + dtStamp;

  EMailSender::Response resp = emailSend.send(ALERT_SMS, message);
  emailSend.send(ALERT_EMAIL, message);

  Serial.println("All Clear sent. Total alerts: " + String(alertCount));

  File alertLog = LittleFS.open("/ALERT.TXT", "a");
  if (!alertLog) {
    Serial.println("File '/ALERT.TXT' open failed");
  } else {
    alertLog.print("All Clear -- Water Normal. Distance: ");
    alertLog.print(distanceToTarget, 1);
    alertLog.print(" inches -- ");
    alertLog.print(dtStamp);
    alertLog.print(" -- Total alerts: ");
    alertLog.print(alertCount);
    alertLog.print(" -- Email status: ");
    alertLog.print(resp.status);
    alertLog.println();
    alertLog.close();
    Serial.println("All Clear event logged to /ALERT.TXT");
  }
}

String ultrasonic() {
  // Simulate sensor data using random generator for testing
  // Generates a float between 0.0 and 10.0
  distanceToTarget = random(0, 101) / 10.0;

  delay(100);

  Serial.print("\n\nDistanceToTarget:  ");
  Serial.print(distanceToTarget, 1);
  Serial.println(" in.");

  // Determine condition based on the simulated distance
  if (distanceToTarget <= 0.5) {
    condition = "FLOODING";
  } else if (distanceToTarget <= 3.0) {
    condition = "HIGHWATER";
  } else if (distanceToTarget > 3.0) {
    condition = "ALLCLEAR";
  }

  return condition;
}

void Wifi_Start() {

  WiFi.mode(WIFI_STA);

  IPAddress _ip, _gateway, _subnet, _dns;
  _ip.fromString("192.168.12.22");
  _gateway.fromString("192.168.12.1");
  _subnet.fromString("255.255.255.0");
  _dns.fromString("192.168.12.1");
  wm.setSTAStaticIPConfig(_ip, _gateway, _subnet, _dns);

  pinMode(WIFI_RESET_PIN, INPUT_PULLUP);
  delay(100);
  if (digitalRead(WIFI_RESET_PIN) == LOW) {
    unsigned long holdStart = millis();
    Serial.println("Reset button held -- hold 3 seconds to confirm wipe...");
    while (digitalRead(WIFI_RESET_PIN) == LOW) {
      if (millis() - holdStart >= 3000) {
        Serial.println("Wiping WiFi credentials -- restarting into portal.");
        //wm.resetSettings();
        ESP.restart();
      }
    }
    Serial.println("Released before 3 seconds -- continuing normal boot.");
  }

  wm.setConfigPortalBlocking(true);
  wm.setConfigPortalTimeout(180);
  wm.setAPClientCheck(true);

  if (!wm.autoConnect("SumpMonitor-Setup")) {
    Serial.println("No saved credentials -- portal is open.");
    Serial.println("Connect to AP 'SumpMonitor-Setup', browse to 192.168.4.1");
    Serial.println("Sketch continues running while portal is active.");
  } else {
    Serial.println("WiFi connected via saved credentials.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Port: ");
    Serial.println(LISTEN_PORT);
  }
}
