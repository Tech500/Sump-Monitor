////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    Version 2.0        Alert cooldown, counter, All Clear, boot fix    04/09/2026
//
//    Version 1.0        Adding  HTTP over TLS (HTTPS)     09/05/2020 @ 13:48 EDT
//
//                       ESP8266 --Internet Sump Pit Monitor, Datalogger and Dynamic Web Server   05/16/2020 @ 12:45 EDT
//
//                       NTP time routines optimized for speed by schufti  --of ESP8266.com
//
//                       Project uses ESP8266 Developement board: NodeMCU
//
//                       readFiles and readFile functions by martinayotte of ESP8266 Community Forum
//
//                       Previous projects:  by tech500" on https://github.com/tech500
//
//                       Project is Open-Source; requires ESP8266 based developement board.
//
//
//
//                      Note:  Uses ESP32-S3 by Espressif Community version 3.3.7 for "Arduino IDE Nightbuild 2.3.8."
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
	Program uses ultrasonicsonic sensing to measure distance of water from top of sump pit.

	Features:

	1.  Ultrasonicsonic distance to water measuring,
	2.  Log file of measurements at 15 minute intervals.
	3.  Web interface display last update of water distance to top of sump pit.
	4.  Graph of distance to top and time of data point.
	5.  FTP for file maintence; should it be needed.
	6.  Automatic deletion of log files.  Can be daily of weekly
	7.  OTA Over-the-air firmware updates.
	8.  Sends email and SMS alert at predetermined height of water to top of sump pit.
  9.  Alert cooldown with phase 1 (5 min) and phase 2 (10 min) intervals.
  10. Alert counter included in subject line.
  11. All Clear notification when water returns to normal.
*/

// ********************************************************************************
// ********************************************************************************
//
//   See invidual library downloads for each library license.
//
//   Following code was developed with the Adafruit CC300 library, "HTTPServer" example.
//   and by merging library examples, adding logic for Sketch flow.
//
// *********************************************************************************
// *********************************************************************************
#include "Arduino.h"
#include <EMailSender.h>  //https://github.com/xreef/EMailSender
#include <WiFi.h>         //Part of ESP8266 Board Manager install __> Used by WiFi to connect to network
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>  //https://github.com/me-no-dev/ESPAsyncWebServer
#include <WiFiClientSecure.h>
#include <SD.h>
#include <FS.h>
#include <LittleFS.h>
#include <FtpServer.h>   //https://github.com/nailbuster/esp8266FTPServer  -->Needed for ftp transfers
#include <HTTPClient.h>  //Part of ESP8266 Board Manager install --> Used for Domain web interface
#include <sys/time.h>    // struct timeval --> Needed to sync time
#include <time.h>        // time() ctime() --> Needed to sync time
#include <Ticker.h>      //Part of version 1.0.3 ESP32 Board Manager install  -----> Used for watchdog ISR
#include <Wire.h>        //Part of the Arduino IDE software download --> Used for I2C Protocol
//#include <LiquidCrystal_I2C.h>   //https://github.com/esp8266/Basic/tree/master/libraries/LiquidCrystal --> Used for LCD Display
#include "variableInput.h"  //Packaged with project download.  Provides editing options; without having to search 2000+ lines of code.

uint8_t connection_state = 0;
uint16_t reconnect_interval = 10000;

EMailSender emailSend("gmail-address@gmail.com", "jxvd vfoc amdb fabf");  //gmail email address and gmail application password

//How to create application password  https://www.lifewire.com/get-a-password-to-access-gmail-by-pop-imap-2-1171882

uint8_t WiFiConnect(const char *nSSID = nullptr, const char *nPassword = nullptr) {
  static uint16_t attempt = 0;
  Serial.print("Connecting to ");
  if (nSSID) {
    WiFi.begin(nSSID, nPassword);
    Serial.println(nSSID);
  }

  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 50) {
    delay(200);
    Serial.print(".");
  }
  ++attempt;
  Serial.println("");
  if (i == 51) {
    Serial.print("Connection: TIMEOUT on attempt: ");
    Serial.println(attempt);
    if (attempt % 2 == 0)
      Serial.println("Check if access point available or SSID and Password\r\n");
    return false;
  }
  Serial.println("Connection: ESTABLISHED");
  Serial.print("Got IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Port:  ");
  Serial.print(LISTEN_PORT);
  Serial.println("\n");
  return true;
}

WiFiClientSecure client;

void Awaits() {
  uint32_t ts = millis();
  while (!connection_state) {
    delay(50);
    if (millis() > (ts + reconnect_interval) && !connection_state) {
      connection_state = WiFiConnect();
      ts = millis();
    }
  }
}

#define online 37  //pin for online LED indicator

#define SPIFFS LittleFS

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

#define NTP1 "us.pool.ntp.org"
#define NTP0 "time.nist.gov"

#define TZ "EST+5EDT,M3.2.0/2,M11.1.0/2"

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

FtpServer ftpSrv;  //set #define FTP_DEBUG in ESP8266FtpServer.h to see ftp verbose on serial

const int ledPin = 14;  //ESP8266, RobotDyn WiFi D1

int trigPin = 14;  //Orange wire
int echoPin = 12;  //White wire
int pingTravelTime;
float pingTravelDistance;
float distanceToTarget;
int dt = 50;

int reconnect;

// ===================================================
// Alert cooldown globals  --Version 2.0
// ===================================================
bool alertFlag = false;           // true when water is high
bool alertAcknowledged = false;   // true when user acknowledges
int alertCount = 0;               // number of alerts sent
unsigned long lastAlertTime = 0;  // millis() of last alert

// Phase 1: first 3 alerts every 5 minutes
const unsigned long PHASE1_INTERVAL = 300000UL;  // 5 minutes

// Phase 2: after 3 alerts every 10 minutes
const unsigned long PHASE2_INTERVAL = 600000UL;  // 10 minutes
// ===================================================

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

  Wire.setClock(2000000);
  Wire.begin(SDA, SCL);

  configTime(0, 0, NTP0, NTP1);
  setenv("TZ", "EST+5EDT,M3.2.0/2,M11.1.0/2", 3);  // this sets TZ to Indianapolis, Indiana
  tzset();

  delay(500);

  started = 1;  //Server started

  delay(500);

  LittleFS.begin(false);

  Serial.println("LittleFS opened!");
  Serial.println("");
  ftpSrv.begin("admin", "sumpone");  //username, password for ftp.

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

  serverAsync.onNotFound(notFound);

  serverAsync.begin();

  Serial.println("*** Ready ***");

  // NOTE: removed  requested = 1  here to prevent boot alert  --Version 2.0
}

// How big our line buffer should be. 100 is plenty!
#define BUFFER 100

void loop() {

  int flag;

  delay(1);

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

    //Open a "WIFI.TXT" for appended writing.   Client access ip address logged.
    File logFile = SPIFFS.open("/WIFI.TXT", "a");

    if (!logFile) {
      Serial.println("File: '/WIFI.TXT' failed to open");
    } else {

      logFile.print("WiFi Disconnected:  ");
      logFile.println(dtStamp);
      logFile.close();

      WiFi.waitForConnectResult();

      reconnect = 1;
    }
  }

  if ((WiFi.status() == WL_CONNECTED) && reconnect == 1) {

    watchdogCounter = 0;  //Resets the watchdogCount

    //Open a "WIFI.TXT" for appended writing.   Client access ip address logged.
    File logFile = SPIFFS.open("/WIFI.TXT", "a");

    if (!logFile) {
      Serial.println("File: '/WIFI.TXT' failed to open");
    } else {

      getDateTime();

      logFile.print("WiFi Reconnected:   ");
      logFile.println(dtStamp);
      logFile.close();

      reconnect = 0;
    }
  }

  if (started == 1) {

    getDateTime();

    // Open "/SERVER.TXT" for appended writing
    File log = LittleFS.open("/SERVER.TXT", "a");

    if (!log) {
      Serial.println("file:  '/SERVER.TXT' open failed");
    }

    log.print("Started Server:  ");
    log.println(dtStamp) + "  ";
    log.close();
  }

  started = 0;  //only time started = 1 is when Server starts in setup

  if (watchdogCounter > 45) {

    totalwatchdogCounter++;

    Serial.println("watchdog Triggered");

    Serial.print("Watchdog Event has occurred. Total number: ");
    Serial.println(watchdogCounter / 80);

    // Open a "log.txt" for appended writing
    File log = LittleFS.open("/WATCHDOG.TXT", "a");

    if (!log) {
      Serial.println("file 'WATCHDOG.TXT' open failed");
    }

    getDateTime();

    log.print("Watchdog Restart:  ");
    log.print("  ");
    log.print(dtStamp);
    log.println("");
    log.close();

    Serial.println("Watchdog Restart  " + dtStamp);

    WiFi.disconnect();

    ESP.restart();
  }

  watchdogCounter = 0;  //Resets the watchdogCount

  ////////////////////////////////////////////////////// FTP ///////////////////////////////////
  for (int x = 1; x < 5000; x++) {
    ftpSrv.handleFTP();
  }
  ///////////////////////////////////////////////////////////////////////////////////////////////

  getDateTime();

  //Executes every 30 seconds routine
  if (SECOND % 30 == 0) {

    flag = 1;

    Serial.println("");
    Serial.println("Thirty second routine");
    Serial.println(dtStamp);
    lastUpdate = dtStamp;  //store dtstamp for use on dynamic web page
    logtoSD();             // Log FIRST -- write distance to LOG file, close cleanly
    delay(100);            // Small breathe between file operations
    ultrasonic();          // Sensor read SECOND -- alert/ALERT.TXT write safe after log closed

    delay(1000);
  }

  flag = 0;

  //Collect "LOG.TXT" Data for one day; do it early (before 00:00:00) so day of week still equals 6.
  if ((HOUR == 23) && (MINUTE == 57) && (SECOND == 0)) {
    newDay();
  }
}

String buildAlertButtonHTML() {
  // Normal water level
  if (!alertFlag) {
    return "<p class='status-normal'>&#x2705; Water Level Normal</p>";
  }

  // High water — not acknowledged
  if (alertFlag && !alertAcknowledged) {
    return String(
      "<form action='/acknowledge' method='POST'>"
      "<button class='btn-alert'>&#x26A0;&#xFE0F; Acknowledge Alert</button>"
      "</form>");
  }

  // High water — acknowledged
  if (alertFlag && alertAcknowledged) {
    return String(
      "<p class='status-ack'>&#x26A0;&#xFE0F; Alert Acknowledged — Water Still High</p>"
      "<br>"
      "<form action='/resume' method='POST'>"
      "<button class='btn-resume'>&#x1F514; Resume Alerts</button>"
      "</form>");
  }

  return "";
}

String processor1(const String &var) {
  Serial.println("processor called: " + var);

  if (var == F("TOP")) {
    char dist[10];
    float safeValue = distanceToTarget;  // ensures valid float
    dtostrf(safeValue, 9, 1, dist);
    return String(dist);
  }

  if (var == F("DATE")) return dtStamp;
  if (var == F("CLIENTIP")) return ipREMOTE.toString();
  if (var == F("ALERTBUTTON")) return buildAlertButtonHTML();

  return String();
}

String processor2(const String &var) {
  // index2.h

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

      str += "<a href=\"";
      str += file_name;
      str += "\">";
      str += file_name;
      str += "</a>";
      str += "    ";
      str += file.size();
      str += "<br>\r\n";
    }

    file = root.openNextFile();
  }

  root.close();

  client.print(str);

  if (var == F("URLLINK"))
    return str;

  if (var == F("LINK"))
    return linkAddress;

  if (var == F("FILENAME"))
    return lastFileName;

  return String();
}

String processor3(const String &var) {

  //index3.h

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
  Serial.println("Client connected:  " + dtStamp);
  Serial.print("Client IP:  ");
  Serial.println(ip2String);
  Serial.print("Path:  ");
  Serial.println(PATH);
  Serial.println(F("Processing request"));

  //Open a "access.txt" for appended writing.   Client access ip address logged.
  File logFile = SPIFFS.open(Restricted, "a");

  if (!logFile) {
    Serial.println("File 'ACCESS.TXT'failed to open");
  }

  if ((ip1String == ip2String) || (ip2String == "0.0.0.0") || (ip2String == "(IP unset)")) {
    logFile.close();
  } else {

    Serial.println("Log client ip address");

    logFile.print("Accessed:  ");
    logFile.print(dtStamp);
    logFile.print(" -- Client IP:  ");
    logFile.print(ip2String);
    logFile.print(" -- ");
    logFile.print("Path:  ");
    logFile.print(PATH);
    logFile.println("");
    logFile.close();
  }
}

void end() {

  delay(1000);

  //Client flush buffers
  client.flush();
  // Close the connection when done.
  client.stop();

  digitalWrite(online, LOW);  //turn-off online LED indicator

  getDateTime();

  Serial.println("Client closed:  " + dtStamp);

  delay(1);
}

void fileStore()  //If 6th day of week, rename "log.txt" to ("log" + month + day + ".txt") and create new, empty "log.txt"
{

  // Open file for appended writing
  File log = LittleFS.open("/LOG.TXT", "a");

  if (!log) {
    Serial.println("file open failed");
  }

  // rename the file "LOG.TXT"
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

void links() {

  client.println("<br>");
  client.println("<h2><a href='http://' + publicIP + ':' + PORT + '/SdBrowse >File Browser</a></h2><br>'");
  //Show IP Adress on client screen
  client.print("Client IP: ");
  client.print(client.remoteIP().toString().c_str());
  client.println("</body>");
  client.println("</html>");
}

void logtoSD()  //Output to LittleFS every thirty seconds
{

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

  log.print(distanceToTarget);
  log.print(" inches ");
  log.print(" , ");
  log.print(dtStamp);
  log.println();

  Serial.println("");
  Serial.print(distanceToTarget, 1);
  Serial.println(" inches; Data written to  " + logname + "  " + dtStamp);

  log.close();
}

void newDay()  //Collect Data for twenty-four hours; then start a new day
{

  if ((DOW) == 6) {
    fileStore();
  }

  File log = LittleFS.open("/LOG.TXT", "a");

  if (!log) {
    Serial.println("file open failed");
  }

  connection_state = WiFiConnect(ssid, password);
  if (!connection_state)
    Awaits();

  EMailSender::EMailMessage message;
  message.subject = "Sump Monitor -- Daily Heartbeat";
  message.message = "Sump Monitor is online and operational.  " + dtStamp;

  EMailSender::Response resp = emailSend.send("ab9nq.william@gmail.com", message);

  Serial.println("Heartbeat sending status: ");
  Serial.println(resp.status);
  Serial.println(resp.code);
  Serial.println(resp.desc);

  // Reset alert state at start of new day
  alertFlag = false;
  alertAcknowledged = false;
  alertCount = 0;
}

///////////////////////////////////////////////////////////////
//readFile  --AsyncWebServer version
////////////////////////////////////////////////////////////////
String notFound(AsyncWebServerRequest *request) {

  digitalWrite(online, HIGH);  //turn-on online LED indicator

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

  digitalWrite(online, LOW);  //turn-off online LED indicator

  return fn;
}

// ===================================================
// sendAlert  --sends email and SMS alert  Version 2.0
// ===================================================
void sendAlert() {

  connection_state = WiFiConnect(ssid, password);
  if (!connection_state)
    Awaits();

  alertCount++;

  EMailSender::EMailMessage message;
  message.subject = "Warning High Water!!! Alert #" + String(alertCount);
  message.message = "Sump Pump /// Alert high water level! "
                    "/// Alert number: "
                    + String(alertCount) + " /// " + dtStamp;

  EMailSender::Response resp = emailSend.send("10 digit-cell-number5@tmomail.net", message);
  emailSend.send("gmail-address@gmail.com", message);

  Serial.println("Alert #" + String(alertCount) + " sent: " + dtStamp);
  Serial.println(resp.status);
  Serial.println(resp.code);
  Serial.println(resp.desc);

  // Log alert event to ALERT.TXT
  delay(100);  // Small breathe before file write
  File alertLog = LittleFS.open("/ALERT.TXT", "a");
  if (!alertLog) {
    Serial.println("File '/ALERT.TXT' open failed");
  } else {
    alertLog.print("Alert #");
    alertLog.print(alertCount);
    alertLog.print(" -- High Water!  Distance: ");
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

// ===================================================
// sendAllClear  --water returned to normal  Version 2.0
// ===================================================
void sendAllClear() {

  connection_state = WiFiConnect(ssid, password);
  if (!connection_state)
    Awaits();

  EMailSender::EMailMessage message;
  message.subject = "Sump Pit -- All Clear";
  message.message = "Water level has returned to normal. "
                    "Total alerts sent: "
                    + String(alertCount) + " /// " + dtStamp;

  EMailSender::Response resp = emailSend.send("3173405675@tmomail.net", message);
  emailSend.send("ab9nq.william@gmail.com", message);

  Serial.println("All Clear sent. Total alerts: " + String(alertCount));

  // Log All Clear event to ALERT.TXT
  File alertLog = LittleFS.open("/ALERT.TXT", "a");
  if (!alertLog) {
    Serial.println("File '/ALERT.TXT' open failed");
  } else {
    alertLog.print("All Clear -- Water Normal.  Distance: ");
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

// ===================================================
// ultrasonic  --get distance in inches  Version 2.0
// ===================================================
void ultrasonic() {

  delay(10);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(10);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  pingTravelTime = pulseIn(echoPin, HIGH);
  delay(25);
  pingTravelDistance = (pingTravelTime * 765. * 5280. * 12) / (3600. * 1000000);
  //distanceToTarget = (pingTravelDistance / 2);  // Normal use
  //distanceToTarget = 4.0;  // Testing only -- comment out for deployment!
  // Fake distance generator for video demo
  distanceToTarget = random(0, 101) / 10.0;   // 0.0 to 10.0 inches
  Serial.print("Distance to Target is: ");
  Serial.print(distanceToTarget, 1);
  Serial.println(" in.");

  digitalWrite(echoPin, HIGH);

  delay(50);

  // ===================================================
  // Alert logic with cooldown  --Version 2.0
  // ===================================================
  if (distanceToTarget < 3.0)  // Water high threshold
  {

    unsigned long now = millis();

    // First alert -- fire immediately
    if (!alertFlag) {
      alertFlag = true;
      alertAcknowledged = false;  // reset acknowledge
      alertCount = 0;
      lastAlertTime = now;
      getDateTime();
      sendAlert();
    }
    // Only continue alerting if NOT acknowledged
    else if (!alertAcknowledged) {
      // Phase 1 -- alerts 2 & 3 every 5 minutes
      if (alertCount < 3 && (now - lastAlertTime >= PHASE1_INTERVAL)) {
        lastAlertTime = now;
        getDateTime();
        sendAlert();
      }
      // Phase 2 -- every 10 minutes after alert 3
      else if (alertCount >= 3 && (now - lastAlertTime >= PHASE2_INTERVAL)) {
        lastAlertTime = now;
        getDateTime();
        sendAlert();
      }
    }

  } else {

    // Water normal -- send All Clear if we were alerting
    if (alertFlag) {
      getDateTime();
      sendAllClear();
    }

    alertFlag = false;          // reset flag
    alertAcknowledged = false;  // reset acknowledge
    alertCount = 0;             // reset counter
  }
  // ===================================================
}

void Wifi_Start() {

  WiFi.mode(WIFI_STA);

  Serial.print("Connecting to ");
  Serial.println(ssid);

  IPAddress ip;
  IPAddress gateway;
  IPAddress subnet;
  IPAddress dns;

  WiFi.begin(ssid, password);

  Serial.println();
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  WiFi.config(ip, gateway, subnet, dns);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  WiFi.waitForConnectResult();

  Serial.printf("Connection result: %d\n", WiFi.waitForConnectResult());
}
