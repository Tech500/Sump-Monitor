//
//   "Sump_Sender.ino" and  
//   variableInput.h library
//   William M. Lucid   09/25/2026 @ 11:34 EDT    
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
#define SERVER_IP   "192.168.12.122"  //Raspberru Pi ipAddress
#define SERVER_PORT  8001

//Find your public ipAddress:  https://whatismyipaddress.com/

String LISTEN_PORT = "80"; //Part of href link for "GET" requests

String linkAddress = "192.168.12.138";

String ip1String = "192.168.12.138";  //Host ip address  

int PORT = 80;  //Web Server port

// ===================================================
// Email sender credentials
// ===================================================
#define SENDER_EMAIL   "emai@gmail.com"       // sending Gmail account  <---- use a seperate Gmail account; not your main Gmail account set filter!
#define SENDER_PASS    "xxxxxxxxxxxx"            // Gmail app password

// ===================================================
// Alert destinations
// ===================================================
#define ALERT_EMAIL    "email@gmail.com"         // Larry's dedicated sump Gmail
#define ALERT_SMS      "xxxxxxxxxxxxxxxx"        // Larry's T-Mobile SMS gateway
#define ALERT_EMAIL2   "email@gmail.com"       // your monitoring copy

#define ALERT_TAG  "SUMP"

// ===================================================
// Grafana Settings
// ===================================================
// Enter the local IP address of your Grafana server (Raspberry Pi)
// Example: "192.168.1.100" -- check your router for the Pi's assigned IP
const char * grafanaIP = "192.168.12.122";  
const int grafanaPort = 3000;
// Your Grafana dashboard UID -- found in the dashboard URL:
// http://grafanaIP:3000/d/XXXXXXX/dashboard-name
const char * grafanaDashboardUID = "xxxxxxxxx";
// ===================================================

//Graphing requires "FREE" "ThingSpeak.com" account..  
//Enter "ThingSpeak.com" data here....
//Example data; enter yout account data..
unsigned long int myChannelNumber = 123456; 
const char * myWriteAPIKey = "EE2345";

//webInterface --send Data to Domain, hosted web site
const char * sendData = "your domaindestination and filename for data from webInterface function";

//FTP Credentials
const char * ftpUser = "xxxxxx";
const char * ftpPassword = "xxxxx";
 
//Restricted Access
const char* Restricted = "/xxxxxxx";  //Can be any filename.  
//Will be used for "GET" request path to pull up client ip list.

///////////////////////////////////////////////////////////
//   "pulicIP/LISTEN_PORT/reset" wiill restart the server
///////////////////////////////////////////////////////////

///////////////// OTA Support //////////////////////////

const char* http_username = "xxxxxx";
const char* http_password = "xxxxxx";

// xx.xx.xx.xx:yyyy/login will log in; this will allow updating firmware using:
// xx.xx.xx.xx:yyyy/update
//
// xx.xx.xx.xx being publicIP and yyyy being PORT.
//
///////////////////////////////////////////////////////

// ===================================================
// Sinric Pro credentials
// ===================================================
#define APP_KEY    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#define APP_SECRET "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"


#define HIGHWATER  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#define ALLCLEAR   "xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"  
#define FLOODING   "xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"  
