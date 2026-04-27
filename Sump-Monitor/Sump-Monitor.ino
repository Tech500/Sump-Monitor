/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//
// Version 9.0.1  04/27/2026
// 
// Fixed sync issue; no longer changes on /sump page refreshes.
// Using processor1 in read-only processing.
//
// Version 9.6.0  04/26/2026
//
// Split ultrasonic() into two functions:
//   - readSensor()      -- every loop pass, live distance for web display and pump tracking
//   - updateCondition() -- top of processor1() and 5-minute cycle, syncs condition/flags
// Eliminates missed condition calls from /sump route contention
// updateCondition() called at top of processor1() -- consistent condition at render time
// processor1() restored to condition/flags -- always in sync at render time
// ALERTKEYWORD restored to processor1()
// readSensor() removed from 5-minute cycle -- already runs every loop pass
// /sump route -- accessLog() and end() commented out, reduced Serial chatter
// Auto-refresh changed to 300 seconds -- syncs with 5-minute condition cycle
// Static IP addressing moved to variableInput.h for easier editing
// Pit depth thresholds moved to variableInput.h -- open source users set their own values
//
// Version 9.5  04/26/2026
//
// Fixes from 9.4:
//   - Double LittleFS.begin() removed -- single call with format-on-fail
//   - dryThreshold variable dropped -- hardcoded 38.0 inches
//   - Watchdog fed during WiFiManager portal -- always reset
//   - prevCondition made global -- correct cross-loop condition tracking
//   - pumpRunning flag added -- gates pump start/stop to single fire
//
// Improvements over 9.4:
//   - ultrasonic() moved to top of loop() -- every pass, best pump runtime tracking resolution
//   - 1-minute didFire cycle removed -- no longer needed
//   - pumpLog() minutes+seconds format
//   - No SSID counter -- auto credential reset after 10 consecutive failures, logged to WIFI.TXT
//   - Serial prints on condition change only -- no clutter
//   - Pump tracking disabled pending real sensor installation
//   - random() range corrected to 0-50.0 inches to cover full pit depth
//   - Pump start/stop logic simplified -- PUMPOFF vs not PUMPOFF
//
// Note: real pit depth observed 46.2 inches dry -- dryThreshold to be
//       finalized after real sensor observation period complete
//
// Version 9.4  04/23/2026
//
// Pump runtime log converted to minutes and seconds display
//
// Version 9.3  04/23/2026
//
// Fixed pump runtime tracking -- correct condition transitions:
// HIGHWATER/FLOODING = pump start, PUMPOFF = pump stop, ALLCLEAR = pump still running
//
// Version 9.2  04/23/2026
//
// Separate 1-minute cycle (condition/pump runtime) from 5-minute cycle (log/Alexa/display)
// Pump runtime tracking -- logs each run duration and daily total to /PUMP.TXT
//
// Version 9.1  04/23/2026
//
// Alert banner: no latching -- live state only. Color key block replaces button UI.
//
// Version 9.0  04/20/2026 @ 05:58 EDT
//
// Added InFlux database and Grafana graphing  (Raspberry Pi --Docker-compose.yml)
//
// Added Alexa announcement with stop command for alert 04/18/2026 @ 20:04 EDT
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
// ESP-32-S3 Development Board ESP-32-S3 Module with ESP-1-N16R8   Memory size required!!!
//
// Note: Uses ESP32-S3 by Espressif Community version 3.3.7 for "Arduino IDE Nightbuild 2.3.8."
//
/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////

#include <WebServer.h>

#define WEBSERVER_H

#define ELEGANTOTA_USE_ASYNC_WEBSERVER 1

#include "Arduino.h"
#include <EMailSender.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebSocketsClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <ElegantOTA.h>
#include <WiFiClientSecure.h>
#include <SD.h>
#include <FS.h>
#include <LittleFS.h>
#include <FTPServer.h>
#define BAUDRATE 74880
#include <HTTPClient.h>
#include <sys/time.h>
#include <time.h>
#include <Ticker.h>
#include <Wire.h>
#include <SinricPro.h>
#include "SinricProContactsensor.h"
#include <SinricProWebsocket.h>
#include "variableInput.h"

WiFiManager wm;

#define WIFI_RESET_PIN 0

uint8_t connection_state = 0;
uint16_t reconnect_interval = 10000;

bool sinricReady = false;

void setupSinricPro() {
  SinricProContactsensor &myHighWater = SinricPro[HIGHWATER];
  SinricProContactsensor &myFlooding  = SinricPro[FLOODING];
  SinricProContactsensor &myAllClear  = SinricPro[ALLCLEAR];

  SinricPro.onConnected([]()    { Serial.printf("Connected to SinricPro\r\n"); });
  SinricPro.onDisconnected([]() { Serial.printf("Disconnected from SinricPro\r\n"); });

  SinricPro.begin(APP_KEY, APP_SECRET);
}

EMailSender emailSend(SENDER_EMAIL, SENDER_PASS);

#define nosensor 0.00

WiFiClientSecure client;

#define online 37

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

#import "index1.h"
#import "index2.h"
#import "index3.h"
#import "index4.h"

IPAddress ipREMOTE;

Ticker secondTick;

volatile int watchdogCounter;
int totalwatchdogCounter;

void ISRwatchdog() {
  watchdogCounter++;
}

WiFiUDP UDP;
unsigned int localUDPport = 123;
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
int started;

#define BUFSIZE 64

AsyncWebServer serverAsync(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

#define LISTEN_PORT 80

const int ledPin = 14;

int trigPin = 14;
int echoPin = 12;
int pingTravelTime;
float pingTravelDistance;
float distance;
float distanceToTarget;
float pitActivity;
int dt = 50;

float snapshotDistance = 0.0;
String snapshotCondition = "UNKNOWN";
bool snapshotAlertFlag = false;
bool snapshotFloodFlag = false;
String snapshotTime = "";

int reconnect;

// ===================================================
// No SSID counter -- auto reset after 10 failures
// ===================================================
int noSSIDcount = 0;

// ===================================================
// Alert globals -- no latching, live state only
// ===================================================
bool didFire5 = false;   // 5-minute cycle flag

bool alertFlag = false;  // true when HIGHWATER
bool floodFlag = false;  // true when FLOODING
int  alertCount = 0;

unsigned long lastAlertTime = 0;

String condition        = "";
String prevCondition    = "";
String lastCondition    = "";
String currentCondition = "";

// Phase intervals kept for email throttling
const unsigned long PHASE1_INTERVAL = 300000UL;
const unsigned long PHASE2_INTERVAL = 600000UL;

// ===================================================
// Pump runtime globals
// ===================================================
bool pumpRunning = false;
unsigned long pumpStartMillis = 0;
unsigned long lastRunSeconds  = 0;
unsigned long totalRunSeconds = 0;
unsigned long dailyPumpSeconds = 0;
unsigned long dailyPumpMinutes = 0;
unsigned long dailyPumpHours   = 0;

// ===================================================
// Sinric Pro globals
// ===================================================
bool myPowerState    = true;
bool lastSinricState = false;
#define pass 2

void sendToGrafana(const char* eventType) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin("http://lucidpi.local:8001/event");
  http.addHeader("Content-Type", "application/json");

  String body = "{\"type\":\"" + String(eventType) +
                "\",\"location\":\"sump_pit\"" +
                ",\"distance\":" + String(distanceToTarget, 1) +
                ",\"pitActivity\":" + String(pitActivity, 1) +
                ",\"dtStamp\":\"" + dtStamp + "\"}";

  int responseCode = http.POST(body);
  Serial.printf("[Grafana] %s distance: %.1f pitActivity: %.1f response: %d\n",
                eventType, distanceToTarget, pitActivity, responseCode);
  http.end();
}

void setup(void) {

  Serial.begin(115200);
  while (!Serial) {};

  randomSeed(esp_random());

  Serial.println("Starting Sump Monitor");

  Wifi_Start();

  secondTick.attach(1, ISRwatchdog);

  pinMode(online, OUTPUT);
  pinMode(4, INPUT_PULLUP);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  Wire.setClock(20000000);
  Wire.begin(SDA, SCL);

  configTime(0, 0, NTP0, NTP1);
  setenv("TZ", TZ, 3);
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

  // Single LittleFS mount -- format on failure
  bool fsok = LittleFS.begin(true);
  Serial.printf_P(PSTR("FS init: %s\n"), fsok ? PSTR("ok") : PSTR("fail!"));

  if (!LittleFS.exists("/SYSTEM")) {
    LittleFS.mkdir("/SYSTEM");
    Serial.println("SYSTEM folder created.");
  }

  ftpSrv.begin(F(ftpUser), F(ftpPassword));

  serverAsync.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    PATH = "/FAVICON";
    if (!flag == 1) request->send(SPIFFS, "/favicon.png", "image/png");
  });

  serverAsync.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    PATH = "/";
    accessLog();
    ipREMOTE = request->client()->remoteIP();
    if (!flag == 1) request->send_P(200, PSTR("text/html"), HTML1, processor1);
    end();
  });

  serverAsync.on("/sump", HTTP_GET, [](AsyncWebServerRequest *request) {
    PATH = "/sump";
    //accessLog();  // reduced Serial chatter
    ipREMOTE = request->client()->remoteIP();
    if (!flag == 1) request->send_P(200, PSTR("text/html"), HTML1, processor1);
    //end();  // reduced Serial chatter
  });

  serverAsync.on("/SdBrowse", HTTP_GET, [](AsyncWebServerRequest *request) {
    PATH = "/SdBrowse";
    accessLog();
    if (!flag == 1) request->send_P(200, PSTR("text/html"), HTML2, processor2);
    end();
  });

  serverAsync.on("/graph", HTTP_GET, [](AsyncWebServerRequest *request) {
    PATH = "/graph";
    accessLog();
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML3, processor3);
    response->addHeader("Server", "ESP Async Web Server");
    if (!flag == 1) request->send(response);
    end();
  });

  serverAsync.on("/Show", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!flag == 1) request->send_P(200, PSTR("text/html"), HTML4, processor4);
  });

  serverAsync.on("/get-file", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, fn, "text/txt");
    PATH = fn;
    accessLog();
    end();
  });

  serverAsync.on("/redirect/internal", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/Show");
  });

  serverAsync.onNotFound(notFound);

  serverAsync.begin();

  started = 1;

  getDateTime();
  updateFiveMinuteCycle();
  delay(500);

  ElegantOTA.begin(&serverAsync);
  Serial.println("ElegantOTA ready.");
  Serial.println("*** Ready ***\n\n");
}

#define BUFFER 100

void loop() {

  delay(1);

  // Always feed watchdog -- portal or not
  watchdogCounter = 0;

  wm.process();
  ElegantOTA.loop();
  SinricPro.handle();

  int packetSize = UDP.parsePacket();
  if (packetSize) {
    int len = UDP.read(incomingPacket, 255);
    if (len > 0) incomingPacket[len] = 0;
    UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());
    UDP.write(replyPacket, sizeof(replyPacket));
    UDP.endPacket();
  }

  // ── WiFi disconnect handling ──
  if (WiFi.status() == WL_NO_SSID_AVAIL) {
    noSSIDcount++;
    Serial.println("No SSID -- count: " + String(noSSIDcount));
    if (noSSIDcount >= 10) {
      Serial.println("Persistent no SSID -- resetting credentials.");
      getDateTime();
      File log = LittleFS.open("/SYSTEM/WIFI.TXT", "a");
      if (log) {
        log.print("Credentials reset -- no SSID count reached: ");
        log.println(dtStamp);
        log.close();
      }
      wm.resetSettings();
      ESP.restart();
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    noSSIDcount = 0;
  }

  if (WiFi.status() != WL_CONNECTED) {
    getDateTime();
    File logFile = SPIFFS.open("/WIFI.TXT", "a");
    if (logFile) {
      logFile.print("WiFi Disconnected: ");
      logFile.println(dtStamp);
      logFile.close();
      WiFi.waitForConnectResult();
      reconnect = 1;
    }
  }

  if ((WiFi.status() == WL_CONNECTED) && reconnect == 1) {
    File logFile = SPIFFS.open("/SYSTEM/WIFI.TXT", "a");
    if (logFile) {
      getDateTime();
      logFile.print("WiFi Reconnected: ");
      logFile.println(dtStamp);
      logFile.close();
      reconnect = 0;
    }
  }

  if (started == 1) {
    getDateTime();
    File log = LittleFS.open("/SYSTEM/SERVER.TXT", "a");
    if (log) {
      log.print("Started Server: ");
      log.println(dtStamp);
      log.close();
    }
  }

  started = 0;

  if (watchdogCounter > 45) {
    totalwatchdogCounter++;
    Serial.println("watchdog Triggered");
    File log = LittleFS.open("/WATCHDOG.TXT", "a");
    if (log) {
      getDateTime();
      log.print("Watchdog Restart: ");
      log.print(dtStamp);
      log.println("");
      log.close();
    }
    WiFi.disconnect();
    ESP.restart();
  }

  for(int x= 1; x<5000;x++){  
    ftpSrv.handleFTP();
  }

  getDateTime();

  // ===================================================
  // 5-minute scheduler
  // ===================================================
  if ((MINUTE % 5 == 0 && SECOND == 0) && !didFire5) {
    didFire5 = true;
    updateFiveMinuteCycle();
  }

  if (MINUTE % 5 != 0) {
    didFire5 = false;
  }  

  if ((HOUR == 23) && (MINUTE == 57) && (SECOND == 0)) {
    newDay();
  }
}


// ===================================================
// updateFiveMinuteCycle() -- runs once every 5 minutes
// ===================================================
void updateFiveMinuteCycle() {

  lastUpdate = dtStamp;

  readSensor();          // ONLY here
  updateCondition();
  updatePumpRuntime();
  distanceLog();
  masterAlexa(condition);
  //handleAlexaReporting();

  // Snapshot for UI
  snapshotDistance   = distanceToTarget;
  snapshotCondition  = condition;
  snapshotAlertFlag  = alertFlag;
  snapshotFloodFlag  = floodFlag;
  snapshotTime       = dtStamp;
}

// ===================================================
// processor1 -- live state, color key block
// ===================================================
String processor1(const String &var) {

  if (var == F("TOP")) {
    char dist[10];
    dtostrf(snapshotDistance, 9, 1, dist);
    return String(dist);
  }

  if (var == F("DATE"))       return snapshotTime;
  if (var == F("LASTUPDATE")) return snapshotTime;
  if (var == F("CLIENTIP"))   return ipREMOTE.toString();

  // ── Color for "Alert Status" label ──
  if (var == F("ALERTCOLOR")) {
    if (snapshotFloodFlag)              return "#b91c1c";
    if (snapshotAlertFlag)              return "#FFD700";
    if (snapshotCondition == "PUMPOFF") return "#8b949e";
    return "#3fb950";
  }

  // ── Inline background color for key block ──
  if (var == F("ALERTBG")) {
    if (snapshotFloodFlag)              return "#b91c1c";
    if (snapshotAlertFlag)              return "#FFD700";
    if (snapshotCondition == "PUMPOFF") return "#8b949e";
    return "#3fb950";
  }

  // ── Inline text color for key block ──
  if (var == F("ALERTFG")) {
    if (snapshotAlertFlag)  return "#1a1a1a";   // dark text on yellow
    return "#ffffff";                           // white text on all others
  }

  // ── CSS class for the key block ──
  if (var == F("ALERTKEYCLASS")) {
    if (snapshotFloodFlag)              return "key-flood";
    if (snapshotAlertFlag)              return "key-high";
    if (snapshotCondition == "PUMPOFF") return "key-pumpoff";
    return "key-normal";
  }

  // ── Single word inside the key block ──
  if (var == F("ALERTKEYWORD")) {
    if (snapshotFloodFlag)              return "FLOODING";
    if (snapshotAlertFlag)              return "HIGH WATER";
    if (snapshotCondition == "PUMPOFF") return "PUMP OFF";
    return "NORMAL";
  }

  if (var == F("SUMPPUMP")) {
    if (snapshotCondition == "PUMPOFF")
      return "<span class='status-idle'>&#x1F4A7; Pump Idle &mdash; Pit Dry</span>";
    return "<span class='status-normal'>&#x1F504; Pump Active</span>";
  }

  return String();
}

String processor2(const String &var) {
  String str;

  if (!LittleFS.begin()) return String();

  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) return String();

  String lastFileName = "";

  File file = root.openNextFile();
  while (file) {
    String file_name = file.name();
    if (file_name.startsWith("DISTANCE") || file_name.startsWith("LOG") ||
        file_name.startsWith("ALERT")    || file_name.startsWith("PUMP")) {
      lastFileName = file_name;
      str += "<a href=\"/";
      str += file_name;
      str += "\">";
      str += file_name;
      str += "</a> ";
      str += file.size();
      str += "<br>\r\n";
    }
    file = root.openNextFile();
  }
  root.close();

  if (var == F("URLLINK"))  return str;
  if (var == F("LINK"))     return linkAddress;
  if (var == F("FILENAME")) return lastFileName;

  return String();
}

String processor3(const String &var) {
  if (var == F("LINK")) return linkAddress;
  return String();
}

String processor4(const String &var) {
  if (var == F("FN"))   return fn;
  if (var == F("LINK")) return linkAddress;
  return String();
}

void accessLog() {
  digitalWrite(online, HIGH);
  getDateTime();

  String ip1String = STATIC_IP;
  String ip2String = ipREMOTE.toString().c_str();

  Serial.println("\nClient connected: " + dtStamp);
  Serial.println("Client IP: " + ip2String);
  Serial.println("Path: " + PATH);
  Serial.println(F("Processing request"));

  File logFile = SPIFFS.open(Restricted, "a");
  if (!logFile) {
    Serial.println("File 'ACCESS.TXT' failed to open");
    return;
  }

  if ((ip1String == ip2String) || (ip2String == "0.0.0.0") || (ip2String == "(IP unset)")) {
    logFile.close();
  } else {
    logFile.print("Accessed: ");
    logFile.print(dtStamp);
    logFile.print(" -- Client IP: ");
    logFile.print(ip2String);
    logFile.print(" -- Path: ");
    logFile.println(PATH);
    logFile.close();
  }
}

void deleteAllFiles() {
    File root = LittleFS.open("/");
    File file = root.openNextFile();

    while (file) {
        String name = file.name();

        // Skip the SYSTEM folder entirely
        if (!name.startsWith("/SYSTEM")) {
            LittleFS.remove(name);
            Serial.println("Deleted: " + name);
        }

        file = root.openNextFile();
    }
}

void end() {
  delay(1000);
  client.flush();
  client.stop();
  digitalWrite(online, LOW);
  getDateTime();
  Serial.println("Client closed: " + dtStamp);
  delay(1);
}

void fileStore() {
  File log = LittleFS.open("/LOG.TXT", "a");
  if (!log) { Serial.println("file open failed"); return; }
  String logname = "/LOG";
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
  DOW    = ti->tm_wday;
  YEAR   = ti->tm_year + 1900;
  MONTH  = ti->tm_mon + 1;
  DATE   = ti->tm_mday;
  HOUR   = ti->tm_hour;
  MINUTE = ti->tm_min;
  SECOND = ti->tm_sec;
  strftime(strftime_buf, sizeof(strftime_buf), "%a , %m/%d/%Y , %H:%M:%S %Z", localtime(&tnow));
  dtStamp = strftime_buf;
  return dtStamp;
}

void handleAlexaReporting() {
  if (condition == lastCondition) {
    Serial.println("Condition change");
    return;
  }
  masterAlexa(condition);
  lastCondition = condition;
}

void links() {
  client.println("<br>");
  client.println("<h2><a href='http://' + SERVER_IP + ':' + PORT + '/SdBrowse >File Browser</a></h2><br>'");
  client.print("Client IP: ");
  client.print(client.remoteIP().toString().c_str());
  client.println("</body></html>");
}

void distanceLog() {
  getDateTime();

  int tempy;
  String Date, Month;

  tempy = DATE;
  Date  = (tempy < 10) ? ("0" + String(tempy)) : String(tempy);
  tempy = MONTH;
  Month = (tempy < 10) ? ("0" + String(tempy)) : String(tempy);

  String logname = "/DISTANCE" + Month + Date + ".TXT";
  File log = LittleFS.open(logname.c_str(), "a");
  if (!log) { Serial.println(logname + " open failed"); return; }
  delay(500);
  log.print(distanceToTarget);
  log.print(", inches,  ");
  log.println(dtStamp);
  log.close();
}

void logtoSD() {
  getDateTime();

  int tempy;
  String Date, Month;

  tempy = DATE;
  Date  = (tempy < 10) ? ("0" + String(tempy)) : String(tempy);
  tempy = MONTH;
  Month = (tempy < 10) ? ("0" + String(tempy)) : String(tempy);

  String logname = "/LOG" + Month + Date + ".TXT";
  File log = LittleFS.open(logname.c_str(), "a");
  if (!log) { Serial.println("file 'LOG.TXT' open failed"); return; }
  delay(500);
  log.print(condition);
  log.print(" ,  ");
  log.print(distanceToTarget);
  log.print(" inches,  ");
  log.println(dtStamp);
  log.close();

  Serial.println("\n" + condition + ",  " + String(distanceToTarget, 1) +
                 "  inches,  Data written to " + logname + ",  " + dtStamp + "\n\n");
}

void pumpLog() {
  unsigned long runMin = lastRunSeconds  / 60;
  unsigned long runSec = lastRunSeconds  % 60;
  unsigned long totMin = totalRunSeconds / 60;
  unsigned long totSec = totalRunSeconds % 60;

  File log = LittleFS.open("/PUMP.TXT", "a");
  if (!log) {
    Serial.println("File '/PUMP.TXT' open failed");
    return;
  }
  log.print("Run: ");
  log.print(runMin);
  log.print("m ");
  log.print(runSec);
  log.print("s  Total today: ");
  log.print(totMin);
  log.print("m ");
  log.print(totSec);
  log.print("s  --  ");
  log.println(dtStamp);
  log.close();

  Serial.println("[Pump] Run: " + String(runMin) + "m " + String(runSec) +
                 "s  Total today: " + String(totMin) + "m " + String(totSec) +
                 "s  --  " + dtStamp);
}

void masterAlexa(String condition) {

  getDateTime();
  Serial.println(dtStamp);

  //readSensor();  // sync display with condition change

  if (condition == "PUMPOFF") return;

  SinricProContactsensor &myHighWater = SinricPro[HIGHWATER];
  SinricProContactsensor &myFlooding  = SinricPro[FLOODING];
  SinricProContactsensor &myAllClear  = SinricPro[ALLCLEAR];

  if (condition == "FLOODING") {
    myHighWater.sendContactEvent(false);
    myAllClear.sendContactEvent(false);
    Serial.println("masterAlexa: FLOODING Triggered");
    sendToGrafana("flooding");
  } else if (condition == "HIGHWATER") {
    myFlooding.sendContactEvent(false);
    myAllClear.sendContactEvent(false);
    Serial.println("masterAlexa: HIGH WATER Triggered");
    sendToGrafana("highwater");
  } else if (condition == "ALLCLEAR") {
    myHighWater.sendContactEvent(false);
    myFlooding.sendContactEvent(false);
    Serial.println("masterAlexa: ALL CLEAR Triggered");
    sendToGrafana("allclear");
  }

  Serial.println(condition + "  Condition");
  logtoSD();
}

void newDay() {
  getDateTime();

  if (DOW == 6) fileStore();

  File log = LittleFS.open("/LOG.TXT", "a");
  if (!log) Serial.println("file open failed");

  EMailSender::EMailMessage message;
  message.subject = String(ALERT_TAG) + " Sump Monitor -- Daily Heartbeat";
  message.message = "Sump Monitor is online and operational. " + dtStamp;
  EMailSender::Response resp = emailSend.send("ab9nq.william@gmail.com", message);
  Serial.println("Heartbeat: " + String(resp.status));

  // Reset alert and pump state at start of new day
  alertFlag       = false;
  floodFlag       = false;
  alertCount      = 0;
  totalRunSeconds = 0;
  lastRunSeconds  = 0;
  pumpStartMillis = 0;
  pumpRunning     = false;

  // Write yesterday's total runtime to MASTER.TXT
  writeMasterRuntime();

  // Weekly purge on Saturday
  if (DOW == 6) {
      deleteAllFiles();   // MASTER.TXT is safe
  }

  // Reset daily counters
  dailyPumpSeconds = 0;
  dailyPumpMinutes = 0;
  dailyPumpHours   = 0;

  snapshotTime = dtStamp;
}

void syncNTP() {
  Serial.print("Waiting for NTP sync");
  int attempts = 0;
  while (time(nullptr) < 1000000000UL && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println(time(nullptr) > 1000000000UL ? " NTP synced!" : " NTP sync failed.");
}

String notFound(AsyncWebServerRequest *request) {
  digitalWrite(online, HIGH);

  if (!request->url().endsWith(F(".TXT"))) {
    request->send(404);
  } else {
    int fnsstart = request->url().lastIndexOf('/');
    fn      = request->url().substring(fnsstart);
    uncfn   = fn.substring(1);
    urlPath = linkAddress + "/" + uncfn;
  }

  request->redirect("/Show");
  digitalWrite(online, LOW);
  return fn;
}

void sendFlood() {
  alertCount++;
  EMailSender::EMailMessage message;
  message.subject = String(ALERT_TAG) + " FLOODING -- IMMEDIATE ATTENTION REQUIRED -- Alert #" + String(alertCount);
  message.message = "FLOODING DETECTED -- Sensor may be submerged! /// Alert number: "
                    + String(alertCount) + " /// " + dtStamp;
  EMailSender::Response resp = emailSend.send(ALERT_SMS, message);
  emailSend.send(ALERT_EMAIL, message);
  emailSend.send(ALERT_EMAIL2, message);
  getDateTime();
  Serial.println("FLOOD Alert #" + String(alertCount) + " sent: " + dtStamp);

  File alertLog = LittleFS.open("/ALERT.TXT", "a");
  if (alertLog) {
    alertLog.print("FLOODING Alert #"); alertLog.print(alertCount);
    alertLog.print(" -- Distance: ");   alertLog.print(distanceToTarget, 1);
    alertLog.print(" inches -- ");      alertLog.print(dtStamp);
    alertLog.print(" -- Email: ");      alertLog.println(resp.status);
    alertLog.close();
  }
}

void sendAlert() {
  alertCount++;
  EMailSender::EMailMessage message;
  message.subject = String(ALERT_TAG) + " Warning High Water!!! Alert #" + String(alertCount);
  message.message = "Sump Pump /// Alert high water level! /// Alert number: "
                    + String(alertCount) + " /// " + dtStamp;
  EMailSender::Response resp = emailSend.send(ALERT_SMS, message);
  emailSend.send(ALERT_EMAIL, message);
  Serial.println("Alert #" + String(alertCount) + " sent: " + dtStamp);

  File alertLog = LittleFS.open("/ALERT.TXT", "a");
  if (alertLog) {
    alertLog.print("Alert #");                    alertLog.print(alertCount);
    alertLog.print(" -- High Water! Distance: "); alertLog.print(distanceToTarget, 1);
    alertLog.print(" inches -- ");                alertLog.print(dtStamp);
    alertLog.print(" -- Email: ");                alertLog.println(resp.status);
    alertLog.close();
  }
}

void sendAllClear() {
  EMailSender::EMailMessage message;
  message.subject = String(ALERT_TAG) + " Sump Pit -- All Clear";
  message.message = "Water level has returned to normal. Total alerts sent: "
                    + String(alertCount) + " /// " + dtStamp;
  EMailSender::Response resp = emailSend.send(ALERT_SMS, message);
  emailSend.send(ALERT_EMAIL, message);
  Serial.println("All Clear sent. Total alerts: " + String(alertCount));

  File alertLog = LittleFS.open("/ALERT.TXT", "a");
  if (alertLog) {
    alertLog.print("All Clear -- Water Normal. Distance: "); alertLog.print(distanceToTarget, 1);
    alertLog.print(" inches -- ");        alertLog.print(dtStamp);
    alertLog.print(" -- Total alerts: "); alertLog.print(alertCount);
    alertLog.print(" -- Email: ");        alertLog.println(resp.status);
    alertLog.close();
  }
}

// ===================================================
// readSensor -- distance only, every loop pass
// Swap random() for real sensor code when deployed
// ===================================================
pp// ===================================================
// readSensor() -- JSN-SR04T ultrasonic, inches
//                 OR random() simulator (demo mode)
// Mode controlled by ULTRASONIC_MODE in variableInput.h
// Called ONLY from updateFiveMinuteCycle()
// ===================================================
void readSensor() {

#if ULTRASONIC_MODE

  // ── Live sensor mode ──────────────────────────
  const int  SAMPLES  = 5;
  const long TIMEOUT  = 30000UL;   // ~510cm max range, uS
  float total         = 0.0;
  int   validReadings = 0;

  for (int i = 0; i < SAMPLES; i++) {

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    // JSN-SR04T requires 20uS trigger pulse
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(20);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH, TIMEOUT);

    if (duration > 0) {
      float inches = (duration / 73.746) / 2.0;
      total += inches;
      validReadings++;
    }

    delay(dt);   // dt = 50, defined in sketch
  }

  if (validReadings > 0) {
    distanceToTarget = total / validReadings;
  } else {
    distanceToTarget = nosensor;   // 0.00 -- fail loud = FLOODING
    Serial.println("[Sensor] WARNING -- no valid echo returns");
  }

  Serial.printf("[Sensor] LIVE  %.1f inches  (%d/%d valid)\n",
                distanceToTarget, validReadings, SAMPLES);

#else

  // ── Demo / simulator mode ─────────────────────
  // random() range covers full pit depth 0.0 - 50.0 inches
  distanceToTarget = random(0, 501) / 10.0;
  Serial.printf("[Sensor] DEMO  %.1f inches  (simulated)\n",
                distanceToTarget);

#endif

  pitActivity = distanceToTarget;
}

// ===================================================
// updateCondition -- condition/flags from current distanceToTarget
// Called from: processor1(), 5-minute cycle
// Thresholds set in variableInput.h
// ===================================================
// ===================================================
// updateCondition() -- pure state machine
// Called ONLY at boot + 5-minute cycle
// ===================================================
String updateCondition() {
  prevCondition = condition;

  // DRY PIT (pump off)
  if (distanceToTarget >= 38.0) {
    condition = "PUMPOFF";
    alertFlag = false;
    floodFlag = false;
  }

  // ALLCLEAR (3.0" to <38.0")
  else if (distanceToTarget >= 3.0) {
    condition = "ALLCLEAR";
    alertFlag = false;
    floodFlag = false;
  }

  // HIGHWATER (0.5" to <3.0")
  else if (distanceToTarget > 0.5) {
    condition = "HIGHWATER";
    alertFlag = true;
    floodFlag = false;
  }

  // FLOODING (≤ 0.5")
  else {
    condition = "FLOODING";
    alertFlag = true;
    floodFlag = true;
  }

  return condition;
}

// ===================================================
// updatePumpRuntime() -- called AFTER updateCondition()
// ===================================================
void updatePumpRuntime() {

  // Pump starts when leaving PUMPOFF
  if (condition != "PUMPOFF" &&
      prevCondition == "PUMPOFF" &&
      !pumpRunning) {

    pumpRunning     = true;
    pumpStartMillis = millis();
    Serial.println("[Pump] Start detected: " + dtStamp);
  }

  // Pump stops when returning to PUMPOFF
  if (condition == "PUMPOFF" && pumpRunning) {

    pumpRunning      = false;
    lastRunSeconds   = (millis() - pumpStartMillis) / 1000;
    totalRunSeconds += lastRunSeconds;
    pumpStartMillis  = 0;

    pumpLog();
  }
}

void writeMasterRuntime() {
    File f = LittleFS.open("/MASTER.TXT", "a");
    if (!f) return;

    char line[80];
    sprintf(line,
        "Total runtime: %02dh %02dm %02ds  --  %s\n",
        dailyPumpHours,
        dailyPumpMinutes,
        dailyPumpSeconds,
        dtStamp.c_str()
    );

    f.print(line);
    f.close();
}

void Wifi_Start() {
  WiFi.mode(WIFI_STA);

  IPAddress _ip, _gateway, _subnet, _dns;
  _ip.fromString(STATIC_IP);
  _gateway.fromString(STATIC_GW);
  _subnet.fromString(STATIC_SUBNET);
  _dns.fromString(STATIC_DNS);
  wm.setSTAStaticIPConfig(_ip, _gateway, _subnet, _dns);

  pinMode(WIFI_RESET_PIN, INPUT_PULLUP);
  delay(100);
  if (digitalRead(WIFI_RESET_PIN) == LOW) {
    unsigned long holdStart = millis();
    Serial.println("Reset button held -- hold 3 seconds to confirm wipe...");
    while (digitalRead(WIFI_RESET_PIN) == LOW) {
      if (millis() - holdStart >= 3000) {
        Serial.println("Wiping WiFi credentials -- restarting into portal.");
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
  } else {
    Serial.println("WiFi connected via saved credentials.");
    Serial.println("IP: " + WiFi.localIP().toString());
    Serial.println("Port: " + String(LISTEN_PORT));
  }
}
