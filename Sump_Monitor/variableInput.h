//
//   "Sump_Sender.ino" and  
//   variableInput.h library
//   William M. Lucid   05/04/2026 @ 21:30 EDT    
// 

// Replace with your network details  
const char * host  = "Sump-Monitor";

//Project uses static ip addressing with router ip address reservation
#define STATIC_IP      "192.168.12.138"  
#define STATIC_GW      "192.168.12.1"
#define STATIC_SUBNET  "255.255.255.0"
#define STATIC_DNS     "192.168.12.1"

// Pit depth thresholds -- measure your pit before setting these
#define FLOODING_THRESHOLD   0.5    // inches -- sensor near submerged
#define HIGHWATER_THRESHOLD  3.0    // inches -- pump should be running
#define DRY_THRESHOLD       38.0    // inches -- pit dry, pump off

// ===================================================
// Sensor Mode -- set before uploading
//   true  = live JSN-SR04T ultrasonic sensor
//   false = random() simulator (demo / bench testing)
// ===================================================
#define ULTRASONIC_MODE false

//Settings pertain to NTP
const int udpPort = 1337;
//NTP Time Servers
const char * NTP0 = "us.pool.ntp.org";
const char * NTP1 = "time.nist.gov";

// ── Server Configuration ──────────────────────────
#define SERVER_IP   "192.168.12.122"  //Raspberry Pi ipAddress
#define SERVER_PORT  8001

//Find your public ipAddress:  https://whatismyipaddress.com/

String LISTEN_PORT = "80"; //Part of href link for "GET" requests

String linkAddress = "192.168.12.138";

String ip1String = "192.168.12.138";  //Host --ESP32 ip address  

int PORT = 80;  //Web Server port

// ===================================================
// Email sender credentials
// ===================================================
#define SENDER_EMAIL   "ab9nq.william@gmail.com"       // sending Gmail account  <---- use a seperate Gmail account; not your main Gmail account set filter!
#define SENDER_PASS    "your Google Mail app password"            // Gmail app password

// ===================================================
// Alert destinations
// ===================================================
#define ALERT_EMAIL    "your Gmail account"         // Larry's dedicated sump Gmail
#define ALERT_SMS      "your sms gateway"        // Larry's T-Mobile SMS gateway
#define ALERT_EMAIL2   "other Gmail contact"       // your monitoring copy

#define ALERT_TAG  "SUMP"

// ===================================================
// Grafana Settings
// ===================================================
// Enter the local IP address of your Grafana server (Raspberry Pi)
// Example: "192.168.1.100" -- check your router for the Pi's assigned IP
const char * grafanaIP = SERVER_IP;  
const int grafanaPort = 3000;
// Your Grafana dashboard UID -- found in the dashboard URL:
// http://grafanaIP:3000/d/XXXXXXX/dashboard-name
const char * grafanaDashboardUID = "adqlkqm";
// ===================================================

//webInterface --send Data to Domain, hosted web site
const char * sendData = "your domaindestination and filename for data from webInterface function";

//FTP Credentials
const char * ftpUser = "admin";
const char * ftpPassword = "admin";
 
//Restricted Access
const char* Restricted = "/Restricted";  //Can be any filename.  
//Will be used for "GET" request path to pull up client ip list.

///////////////////////////////////////////////////////////
//   "pulicIP/LISTEN_PORT/reset" wiill restart the server
///////////////////////////////////////////////////////////

///////////////// OTA Support //////////////////////////

const char* http_username = "admin123";
const char* http_password = "admin";

// xx.xx.xx.xx:yyyy/login will log in; this will allow updating firmware using:
// xx.xx.xx.xx:yyyy/update
//
// xx.xx.xx.xx being publicIP and yyyy being PORT.
//
///////////////////////////////////////////////////////

// ===================================================
// Sinric Pro credentials
// ===================================================
#define APP_KEY    "5535b663-414c-45a5-9dd5-4eb14c56df57"
#define APP_SECRET "9eeb6ff2-171c-45d4-8cb9-c48e09563026-ec3675a4-590d-418b-a6a0-81c748ba4f13"


#define HIGHWATER  "69e010efad44f4047dff36d3"
#define ALLCLEAR   "69e0ea0bad44f4047dffbae4"  
#define FLOODING   "69e011aec1c259380d9824c7"  
