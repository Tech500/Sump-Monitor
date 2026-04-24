/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////
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
// ESP-32-S3 Development Board ESP-32-S3 Module with ESP-1-N16R8   Memry size required!!!
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
float distanceToTarget;
float pitActivity;
int dt = 50;

float dryThreshold = 38.0;

int reconnect;

// ===================================================
// Alert globals -- no latching, live state only
// ===================================================
bool didFire  = false;   // 1-minute cycle flag
bool didFire5 = false;   // 5-minute cycle flag

bool alertFlag = false;  // true when HIGHWATER
bool floodFlag = false;  // true when FLOODING
int  alertCount = 0;

unsigned long lastAlertTime = 0;

String condition     = "";
String lastCondition = "";
String currentCondition = "";

// Phase intervals kept for email throttling
const unsigned long PHASE1_INTERVAL = 300000UL;
const unsigned long PHASE2_INTERVAL = 600000UL;

// ===================================================
// Pump runtime globals
// ===================================================
unsigned long pumpStartMillis = 0;
unsigned long lastRunSeconds  = 0;
unsigned long totalRunSeconds = 0;

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

  LittleFS.begin(true);
  Serial.println("LittleFS opened!");

  bool fsok = LittleFS.begin();
  Serial.printf_P(PSTR("FS init: %s\n"), fsok ? PSTR("ok") : PSTR("fail!"));

  ftpSrv.begin(F(ftpUser), F(ftpPassword));

  serverAsync.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    PATH = "/FAVICON";
    if (!flag == 1) request->send(SPIFFS, "/favicon.png", "image/png");
  });

  serverAsync.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    PATH = "/";
    accessLog();
    ipREMOTE = request->client()->remoteIP();
    serverAsync.begin();
    if (!flag == 1) request->send_P(200, PSTR("text/html"), HTML1, processor1);
    end();
  });

  serverAsync.on("/sump", HTTP_GET, [](AsyncWebServerRequest *request) {
    PATH = "/sump";
    accessLog();
    ipREMOTE = request->client()->remoteIP();
    if (!flag == 1) request->send_P(200, PSTR("text/html"), HTML1, processor1);
    end();
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
  lastUpdate = dtStamp;
  ultrasonic();   // initial reading at boot
  delay(500);

  ElegantOTA.begin(&serverAsync);
  Serial.println("ElegantOTA ready.");
  Serial.println("*** Ready ***\n\n");
}

#define BUFFER 100

void loop() {

  int flag;

  delay(1);

  if (!wm.getConfigPortalActive()) {
    watchdogCounter = 0;
  }

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
    watchdogCounter = 0;
    File logFile = SPIFFS.open("/WIFI.TXT", "a");
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
    File log = LittleFS.open("/SERVER.TXT", "a");
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

  ftpSrv.handleFTP();

  getDateTime();

  // ── 1-minute cycle: condition + pump runtime tracking ──
  if (SECOND == 0 && !didFire) {
    didFire = true;
    ultrasonic();   // updates condition, alertFlag, floodFlag, pump runtime
  }

  if (SECOND != 0) {
    didFire = false;
  }

  // ── 5-minute cycle: distance log, display update, Alexa reporting ──
  if ((MINUTE % 5 == 0 && SECOND == 0) && !didFire5) {
    didFire5 = true;
    lastUpdate = dtStamp;
    distanceLog();
    handleAlexaReporting();
  }

  if (MINUTE % 5 != 0) {
    didFire5 = false;
  }

  flag = 0;

  if ((HOUR == 23) && (MINUTE == 57) && (SECOND == 0)) {
    newDay();
  }
}

// ===================================================
// processor1 -- live state, color key block
// ===================================================
String processor1(const String &var) {

  getDateTime();

  if (var == F("TOP")) {
    char dist[10];
    dtostrf(distanceToTarget, 9, 1, dist);
    return String(dist);
  }

  if (var == F("DATE"))       return dtStamp;
  if (var == F("LASTUPDATE")) return lastUpdate;
  if (var == F("CLIENTIP"))   return ipREMOTE.toString();

  // ── Color for "Alert Status" label ──
  if (var == F("ALERTCOLOR")) {
    if (floodFlag)              return "#b91c1c";
    if (alertFlag)              return "#FFD700";
    if (condition == "PUMPOFF") return "#8b949e";
    return "#3fb950";
  }

  // ── Inline background color for key block ──
  if (var == F("ALERTBG")) {
    if (floodFlag)              return "#b91c1c";
    if (alertFlag)              return "#FFD700";
    if (condition == "PUMPOFF") return "#8b949e";
    return "#3fb950";
  }

  // ── Inline text color for key block ──
  if (var == F("ALERTFG")) {
    if (alertFlag)  return "#1a1a1a";   // dark text on yellow
    return "#ffffff";                   // white text on all others
  }

  // ── CSS class for the key block ──
  if (var == F("ALERTKEYCLASS")) {
    if (floodFlag)              return "key-flood";
    if (alertFlag)              return "key-high";
    if (condition == "PUMPOFF") return "key-pumpoff";
    return "key-normal";
  }

  // ── Single word inside the key block ──
  if (var == F("ALERTKEYWORD")) {
    if (floodFlag)              return "FLOODING";
    if (alertFlag)              return "HIGH WATER";
    if (condition == "PUMPOFF") return "PUMP OFF";
    return "NORMAL";
  }

  if (var == F("SUMPPUMP")) {
    if (condition == "PUMPOFF")
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

  String ip1String = "192.168.12.22";
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
    Serial.println("No Condition change");
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
                 "  inches,  Data written to " + logname + ",  " + dtStamp);
}

void pumpLog() {
  // Convert seconds to minutes and seconds
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
  Serial.println(dtStamp);

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

  Serial.println(condition + "  Condition changed");
  logtoSD();
}

void newDay() {
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

String ultrasonic() {

  distanceToTarget = random(0, 101) / 10.0;
  //distanceToTarget = 39.0;  // for testing dry/pump-off

  pitActivity = distanceToTarget;

  delay(100);

  Serial.printf("\n\nDistanceToTarget:  %.1f in.\n", distanceToTarget);

  // Save previous condition before updating
  String prevCondition = condition;

  if (distanceToTarget >= dryThreshold) {
    condition  = "PUMPOFF";
    alertFlag  = false;
    floodFlag  = false;
  } else if (distanceToTarget <= 0.5) {
    condition  = "FLOODING";
    floodFlag  = true;
    alertFlag  = false;
  } else if (distanceToTarget <= 3.0) {
    condition  = "HIGHWATER";
    alertFlag  = true;
    floodFlag  = false;
  } else {
    condition  = "ALLCLEAR";
    alertFlag  = false;
    floodFlag  = false;
  }

  // ── Pump runtime tracking ──
  // Pump starts when water rises to HIGH WATER or FLOODING
  // Pump stops when pit returns to dry (PUMPOFF)
  // ALLCLEAR = water dropping but pit not yet dry -- pump still running

  // Pump just started -- water rose to alert level
  if ((condition == "HIGHWATER" || condition == "FLOODING") &&
      (prevCondition == "ALLCLEAR" || prevCondition == "PUMPOFF")) {
    pumpStartMillis = millis();
    Serial.println("[Pump] Start detected: " + dtStamp);
  }

  // Pump just stopped -- pit returned to dry
  if (condition == "PUMPOFF" &&
      (prevCondition == "HIGHWATER" || prevCondition == "FLOODING" || prevCondition == "ALLCLEAR")) {
    if (pumpStartMillis > 0) {
      lastRunSeconds   = (millis() - pumpStartMillis) / 1000;
      totalRunSeconds += lastRunSeconds;
      pumpStartMillis  = 0;
      pumpLog();
    }
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
