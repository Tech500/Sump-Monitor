//
//   "Sump_Sender.ino" and  
//   variableInput.h library
//   William Lucid   04/25/2026   @ 16:37 EDT    
// 

// Replace with your network details  
const char * host  = "Sump-Monitor";

// Replace with your network details
const char * ssid = "yourSSID";
const char * password = "yourPassword";

//Settings pertain to NTP
const int udpPort = 1337;
//NTP Time Servers
const char * udpAddress1 = "us.pool.ntp.org";
const char * udpAddress2 = "time.nist.gov";

//publicIP accessiable over Internet with Port Forwarding; know the risks!!!
//WAN IP Address.  Or use LAN IP Address --same as server ip; no Internet access. 
#define publicIP "your Tailscale domain machine/"  //Part of href link for "GET" requests

//Find your public ipAddress:  https://whatismyipaddress.com/

String LISTEN_PORT = "80"; //Part of href link for "GET" requests

String linkAddress = "your Tailscale domain machine:80";  //publicIP and PORT for URL link

String ip1String = "hostipaddress";  //Host ip address  

int PORT = 80;  //Web Server port

//Graphing requires "FREE" "ThingSpeak.com" account..  
//Enter "ThingSpeak.com" data here....
//Example data; enter yout account data..
unsigned long int myChannelNumber = 123456; //placeholder --enter your myChannelNumber
const char * myWriteAPIKey = "EE2345";

//Server settings --all internal LAN addresses  --Enter your network address reaervation from router ip, gateway, dns
#define ip {192,168,12,22}
#define subnet {255,255,255,0}
#define gateway {192,168,12,1}
#define dns {192,168,12,1}

//webInterface --send Data to Domain, hosted web site
const char * sendData = "your domaindestination and filename for data from webInterface function";

//FTP Credentials
//const char * ftpUser = "username";  //create username uncomment
//const char * ftpPassword = "password";  //create password uncomment
 
//Restricted Access
const char* Restricted = "/Restricted";  //Can be any filename.  
//Will be used for "GET" request path to pull up client ip list.

///////////////////////////////////////////////////////////
//   "pulicIP/LISTEN_PORT/reset" wiill restart the server
///////////////////////////////////////////////////////////

///////////////// OTA Support //////////////////////////

const char* http_username = "yours";
const char* http_password = "yours";

// xx.xx.xx.xx:yyyy/login will log in; this will allow updating firmware using:
// xx.xx.xx.xx:yyyy/update
//
// xx.xx.xx.xx being publicIP and yyyy being PORT.
//
///////////////////////////////////////////////////////

// ===================================================
// Sinric Pro credentials
// ===================================================
#define APP_KEY    "yours"
#define APP_SECRET "yours"
#define HIGH_WATER_ID  "yours"
#define FLOODING_ID    "yours"
#define ALL_CLEAR_ID   "yours"
