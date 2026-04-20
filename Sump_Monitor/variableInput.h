//
//   "Sump_Sender.ino" and  
//   variableInput.h library
//   William M. Lucid   10/06/2019   @ 15:02 EDT    
// 

// Replace with your network details  
const char * host  = "Sump-Monitor";

#define SSID       "R2D2"
#define PASS       "Sky7388500"

//Settings pertain to NTP
const int udpPort = 1337;
//NTP Time Servers
const char * NTP0 = "us.pool.ntp.org";
const char * NTP1 = "time.nist.gov";

// ESP32 is the server -- this is its own IP
#define SERVER_IP "192.168.12.22"    // ESP32 itself

//Find your public ipAddress:  https://whatismyipaddress.com/

String LISTEN_PORT = "80"; //Part of href link for "GET" requests

String linkAddress = "sump-monitor.tailb986d2.ts.net";

String ip1String = "192.168.12.141";  //Host ip address  

int PORT = 80;  //Web Server port

// ===================================================
// Email sender credentials
// ===================================================
#define SENDER_EMAIL   "ab9nq.william@gmail.com"       // sending Gmail account  <---- use a seperate Gmail account; not your main Gmail account set filter!
#define SENDER_PASS    "pgns djue qzps seat"            // Gmail app password

// ===================================================
// Alert destinations
// ===================================================
#define ALERT_EMAIL    "ab9nq.william@gmail.com"         // Larry's dedicated sump Gmail
#define ALERT_SMS      "3173405675@tmomail.net"        // Larry's T-Mobile SMS gateway
#define ALERT_EMAIL2   "ab9nq.william@gmail.com"       // your monitoring copy

#define ALERT_TAG  "SUMP"

//Graphing requires "FREE" "ThingSpeak.com" account..  
//Enter "ThingSpeak.com" data here....
//Example data; enter yout account data..
unsigned long int myChannelNumber = 123456; 
const char * myWriteAPIKey = "EE2345";

//webInterface --send Data to Domain, hosted web site
const char * sendData = "your domaindestination and filename for data from webInterface function";

//FTP Credentials
const char * ftpUser = "sump";
const char * ftpPassword = "sumpone";
 
//Restricted Access
const char* Restricted = "/Restricted";  //Can be any filename.  
//Will be used for "GET" request path to pull up client ip list.

///////////////////////////////////////////////////////////
//   "pulicIP/LISTEN_PORT/reset" wiill restart the server
///////////////////////////////////////////////////////////

///////////////// OTA Support //////////////////////////

const char* http_username = "admin";
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
#define APP_KEY    "f965bf3e-6d28-41e4-93f3-06772a503550"
#define APP_SECRET "12de50c7-24cf-47e1-a0ca-63e22e6090f4-fc49eeb2-fc36-4fd5-bd36-7aed7bb000ad"


#define HIGHWATER  "69e010efad44f4047dff36d3"
#define ALLCLEAR   "69e0ea0bad44f4047dffbae4"  
#define FLOODING   "69e011aec1c259380d9824c7"  
