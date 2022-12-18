/**
ESP32 Garage Door Controller by StudioShemp 
A highly configurable garage door controller allowing control and updates via MQTT, Blynk, and voice control via Google Home  
Optionally, you can remove either the Blynk or MQTT components following the comments in the code
There's also an option to replace the Ultrasonic sensor and use a reed switch to read the open/closed door state 
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <BlynkSimpleEsp32.h>   		 							/** Only required if using Blynk */ 
//#include <Simpletimer.h>											// Uncomment if using MQTT Only  //switch from Blynk's implementation of Simpletimer (included within BlynkSimple and contains anti-flooding code for the Blynk Service) to Simpletimer.h library
#include <iostream>   												// provides std::cout for char conversion in MQTT message  /** Only required if using MQTT */ 
#include <ArduinoJson.h>											/** Only required if using MQTT */ 
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

const char* host = "garagedoor";
using std::cout;

#define BLYNK_PRINT Serial 											/** Only required if using Blynk */ 

#define BLYNK_TEMPLATE_ID "XXXXXXXX"   								/** Only required if using Blynk */ //get this when you create your Blynk device on https//blynk.console
#define BLYNK_DEVICE_NAME "Garage Door Controller V2"  				/** Only required if using Blynk */ //get this when you create your Blynk device on https//blynk.console 
#define BLYNK_AUTH_TOKEN "XXXXXXXX"  								/** Only required if using Blynk */ //get this when you create your Blynk device on https//blynk.console


#define pushPin V7 								    				/** Only required if using Blynk */ //Virtual Pin for Blynk button 
#define distancePin V5 										 		/** Only required if using Blynk */ //Virtual Pin for recording distance
#define warnPin V0 													/** Only required if using Blynk */ //Virtual Pin for muting open door alert	

#define relayPin 32 												//Pin to trigger relay //comment out if you want to use a reed switch in place of the ultrasonic sensor. 
#define trigPin 14 													//Pin to trigger ultrasonic sensor //comment out if you want to use a reed switch in place of the ultrasonic sensor. 
#define echoPin 25 													//Pin to read ultrasonic sensor echo //comment out if you want to use a reed switch in place of the ultrasonic sensor. 
#define reedPin 34 												    //Option - Pin to read state of a reed switch for close/open. Declared but unused unless using a reed switch in place of the ultrasonic sensor and the checkDoorstate function is uncommented in setup (and the checkDistance should be commented out). 

char auth[] = BLYNK_AUTH_TOKEN;										/** Only required if using Blynk */ 
char ssid[] = "XXXXXXXX";											//replace with your wifi SSID
char pass[] = "XXXXXXXX";											//replace with your wifi password

const char* mqttUser = "XXXXXXXX";									/** Only required if using MQTT */ //replace with your Mosquitto username
const char* mqttPassword = "XXXXXXXX";						  		/** Only required if using MQTT */ //replace with your Mosquitto password

char mqtt_server[] = "XXXXXXXX";					  	    	    /** Only required if using MQTT */  //replace with your own MQTT Server IP Address e.g. char mqtt_server[] = "192.168.1.254";
WiFiClient GarageDoor;							                    /** Only required if using MQTT */ 
PubSubClient client(GarageDoor);									/** Only required if using MQTT */ 

long duration;
int distance;
int lastdistance;
int openThreshold = 40; 											// if sensor is reading below 40cm, the door is in an open position
int doorstate;													    //Option - Declared but unused unless using a reed switch in place of the ultrasonic sensor and the checkDoorstate function is uncommented in setup (and the checkDistance should be commented out).
int lastdoorstate;												    //Option - Declared but unused unless using a reed switch in place of the ultrasonic sensor and the checkDoorstate function is uncommented in setup (and the checkDistance should be commented out).

int warnvalue = 0;													/** Only required if using Blynk */
int warnCount = 0;													/** Only required if using Blynk */
int warnThreshold = 300; 											/** Only required if using Blynk */ //300 seconds before first "Garage Door is Open" warning (5 mins)
int warnAgain = 120;												/** Only required if using Blynk */ //shorter duration before sending a subsequent warning - reduces WarnThreshold by this value so subsequent warnings will occur every 3 minutes (warnThreshold - warnAgain) seconds
long lastPublish = 0;

WidgetLCD lcd(V3);													/** Only required if using Blynk */ 
BlynkTimer timer;													/** Only required if using Blynk */

WebServer server(80);

const char* loginIndex = 											//set up login page for OTA - 
 "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
            "<td colspan=2>"
                "<center><font size=4><b>Garagedoor Update Login</b></font></center>"
                "<br>"
            "</td>"
            "<br>"
            "<br>"
        "</tr>"
        "<td>Username:</td>"
        "<td><input type='text' size=25 name='userid'><br></td>"
        "</tr>"
        "<br>"
        "<br>"
        "<tr>"
            "<td>Password:</td>"
            "<td><input type='Password' size=25 name='pwd'><br></td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
            "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
        "</tr>"
    "</table>"
"</form>"
"<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='update' && form.pwd.value=='update')"		
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Bad update Password or Username')/*displays error message with a bit of a username and password hint*/" 
    "}"
    "}"
"</script>";
 
 
const char* serverIndex =  											//set up index page for OTA - 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')" 
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";



void setup() {
  pinMode(trigPin, OUTPUT); 										// Sets the esp32 trigPin as an Output if using ultrasonic sensor. // Comment out if using a reed switch on GPIO34 to sense open / close
  pinMode(echoPin, INPUT); 											// Sets the esp32 echoPin as an Input if using ultrasonic sensor.  // Comment out if using a reed switch on GPIO34 to sense open / close
  pinMode(relayPin, OUTPUT); 										// Sets esp32 pin Connected to the Relay control board as output
//  pinMode(reedPin, INPUT);											// Sets esp32 pin as input if replacing the ultrasonic sensor with a reed switch to sense garage door open/close. Uncomment using a reed switch.
  Serial.begin(115200);
  
//setup_wifi();  													//Uncomment if using MQTT only. This will call the setup_wifi() function to connect wifi instead of using the Blynk libraries via Blynk.begin

Blynk.begin(auth, ssid, pass, "blynk.cloud", 8080);					/** Only required if using Blynk */ //sets up both wifi and connection to Blynk

  Serial.println("Setting up MQTT");								/** Only required if using MQTT */ 
  client.setServer(mqtt_server, 1883);								/** Only required if using MQTT */ 
  client.setCallback(callback);										/** Only required if using MQTT */
  timer.setInterval(1000L, checkDistance);							// run checkDistance every 1 second if using ultrasonic sensor. comment out if using a reed switch.
  //timer.setInterval(1000L, checkDoorstate);							// run checkDoorstate every 1 second - uncomment if using a reed switch instead of an ultrasonic sensor 

  if (!MDNS.begin(host)) { 										  	// set up mDNS to respond as http://garagedoor.local 
    Serial.println("Error setting up MDNS responder!");
       } else {
        Serial.println("MDNS responder configured");
      }

  server.on("/", HTTP_GET, []() {								  	// set up OTA updates 
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });																//end of OTA setup
  server.begin();
  Serial.println("OTA Host set up"); 
  
  Serial.println("Leaving setup"); 
}

BLYNK_WRITE(pushPin) 											/** Only required if using Blynk */ // reads V7 from the Blynk App/IFTTT webhook and holds it high for half a second, then returns it to low   
  {																/** Only required if using Blynk */
    if (param.asInt() == 1) {									/** Only required if using Blynk */	
      digitalWrite(relayPin, HIGH); 							/** Only required if using Blynk */ // GPIO32  sets the digital relay pin high
      timer.setTimeout(500L, []() { 							/** Only required if using Blynk */ // commence non-blocking timer - half a second
        digitalWrite(relayPin, LOW); 							/** Only required if using Blynk */ //GPIO32   sets the digital relay pin low
        Blynk.virtualWrite(pushPin, 0); 						/** Only required if using Blynk */ // sets the virtual pin V7 low  
      }); 														/** Only required if using Blynk */ // END Timer Function
    }															/** Only required if using Blynk */
  }																/** Only required if using Blynk */

  BLYNK_WRITE(warnPin)											/** Only required if using Blynk */ //READ VPIN0	
{ 																/** Only required if using Blynk */ // assigning incoming value from virtual pin V0 to a variable
    if (param.asInt() == 1) {									/** Only required if using Blynk */	
      warnvalue = 1;					 						/** Only required if using Blynk */ 
	}else{							 							/** Only required if using Blynk */ 
       warnvalue = 0; 											/** Only required if using Blynk */ 
   }															/** Only required if using Blynk */ 
Serial.print("Open Door Warning set to:  ");
Serial.println(warnvalue);
} 																/** Only required if using Blynk */


void setup_wifi() { // Connect to a WiFi network			   // this function will not be called if using Blynk (ie function call "setup_wifi();" commented out above. )  If using MQTT only, comment out the Blynk.begin and uncomment the setup.wifi function calls
  delay(10);
  
  Serial.print ("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  }
  Serial.print ("Connected on IP Address :");
  Serial.println(WiFi.localIP());
}


void callback(char * topic, byte * message, unsigned int length) { /** Only required if using MQTT */  // this function will not be called if using Blynk only and function call "client.setCallback(callback);"  is commented out above. 

  Serial.println(topic);											/** Only required if using MQTT */  
  String mqttmessage;												/** Only required if using MQTT */  

  for (int i = 0; i < length; i++) {								/** Only required if using MQTT */  
    Serial.print((char) message[i]);								/** Only required if using MQTT */  
    mqttmessage += (char) message[i];								/** Only required if using MQTT */  
  }																	/** Only required if using MQTT */  
  Serial.println(mqttmessage);										/** Only required if using MQTT */  

  
  if (String(topic) == "garagedoor/operate") {						/** Only required if using MQTT */  // When a toggle message is received on the topic garagedoor/operate, toggle the relay for half a second. 
    Serial.println("garagedoor/operate topic received");
    if (mqttmessage == "toggle") {
      digitalWrite(relayPin, HIGH); 								/** Only required if using MQTT */  //GPIO32  sets the digital relay pin high
      timer.setTimeout(500L, []() { 								/** Only required if using MQTT */  //commence timer - half a second
          digitalWrite(relayPin, LOW); 								/** Only required if using MQTT */  // GPIO32   sets the digital relay pin low 
        });  														/** Only required if using MQTT */  // END Timer Function
      }																/** Only required if using MQTT */  
    }																/** Only required if using MQTT */  
}																	/** Only required if using MQTT */  
    

void checkDistance() {											// function to check the distance to the door and door state and publish updates to Blynk and MQTT

 if (!client.connected()) {		 								/** Only required if using MQTT */ //Attempt MQTT reconnection if it drops  
	timer.setTimeout(3000L, []() { 								/** Only required if using MQTT */
    reconnect();												/** Only required if using MQTT */
        });														/** Only required if using MQTT */
  }																/** Only required if using MQTT */

  long now = millis();
  String cover;	
 
  digitalWrite(trigPin, LOW);								    //Read Ultrasonic sensor
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);									// Sets the trigPin on HIGH state for 10 micro seconds - sends ultrasonic signal
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);  							// Reads the echoPin, returns the sound wave travel time in microseconds
  distance = duration * 0.034 / 2;								// Calculating the distance


Blynk.virtualWrite(distancePin, distance); 						/** Only required if using Blynk */
  if (distance < openThreshold) {
    cover = "open";
  lcd.print(3, 0, " Door Open "); 								/** Only required if using Blynk */
    warnCount++;

  } else {
    cover = "closed";
    lcd.print(3, 0, "Door Closed"); 							/** Only required if using Blynk */
    warnCount = 0;
  }

  if (warnCount > warnThreshold) {
	  if (warnvalue == 0) { 									/** Only required if using Blynk */
    Blynk.logEvent("door_warning", String("Garage door is open!")); /** Only required if using Blynk */ // Send an event to the App. Could add another mqtt publish topic here, e.g 'client.publish("garagedoor/warn", "TRUE");' but I only need notification in the Blynk App. 
	Serial.println("Door open warning sent  ");			  		/** Only required if using Blynk */
    } 															/** Only required if using Blynk */
	warnCount = (warnCount - warnAgain); 						/** Only required if using Blynk */ //Could add another mqtt publish topic here, e.g 'client.publish("garagedoor/warn", "FALSE");' but I only need notification in the Blynk App.
	}															/** Only required if using Blynk */
if (abs(lastdistance - distance) > 3 || (now - lastPublish) > 600000){		/** Only required if using MQTT */ // ignore small sensor variations from sensor - don't need slight changes sent to MQTT - but send at least every 10 minutes
    lastdistance = distance;
	std::string strdistance = std::to_string(distance); 		/** Only required if using MQTT */ //convert distance to string using cout so it can be published
 
    StaticJsonDocument<256> JSONdoc;							/** Only required if using MQTT */ //Alocate RAM for JSON document. MQTT will restrict packets to 256 bytes
 
    JSONdoc["cover"] = cover.c_str();							/** Only required if using MQTT */ //add values to the JSON doc
    JSONdoc["distance"] = strdistance.c_str();					/** Only required if using MQTT */

	  Serial.println(distance);
	  Serial.println(cover);

    char buffer[256];											/** Only required if using MQTT */ //create a temporary buffer for the serialised data
    serializeJson(JSONdoc, buffer);								/** Only required if using MQTT */ //generate the JSON string -e.g. {"cover":"open","distance":11} and send it to buffer

    client.publish("garagedoor/state", buffer);				    /** Only required if using MQTT */  
    lastPublish = now;											/** Only required if using MQTT */
  }															    /** Only required if using MQTT */

}																

void checkDoorstate() {											// optional function to check/report open close state via a reed switch instead of ultrasonic sensor. not called unless uncommented in setup() (should also make sure checkDistance is commented out)

 if (!client.connected()) {		 								/** Only required if using MQTT */ //Attempt MQTT reconnection if it drops  
	timer.setTimeout(3000L, []() { 								/** Only required if using MQTT */
    reconnect();												/** Only required if using MQTT */
        });														/** Only required if using MQTT */
  }																/** Only required if using MQTT */

  lastdoorstate = doorstate;									// capture the last known state of the garage door
  long now = millis();
  String cover;	

  doorstate = digitalRead(reedPin);								// read the state of the reed switch (assumes a normally open reed switch (closed when near magnet) which is opened when the door is opened
  if (doorstate == LOW) {			
    cover = "open";
  lcd.print(3, 0, " Door Open "); 								/** Only required if using Blynk */
    warnCount++;

  } else {
    cover = "closed";
    lcd.print(3, 0, "Door Closed"); 							/** Only required if using Blynk */
    warnCount = 0;
  }

  if (warnCount > warnThreshold) {
	  if (warnvalue == 0) { 									/** Only required if using Blynk */
    Blynk.logEvent("door_warning", String("Garage door is open!")); 	/** Only required if using Blynk */ // Send an event to the App. Could add another mqtt publish topic here, e.g 'client.publish("garagedoor/warn", "TRUE");' but I only need notification in the Blynk App. 
	Serial.println("Door open warning sent  ");			  		/** Only required if using Blynk */
    } 															/** Only required if using Blynk */
	warnCount = (warnCount - warnAgain); 						/** Only required if using Blynk */ //Could add another mqtt publish topic here, e.g 'client.publish("garagedoor/warn", "FALSE");' but I only need notification in the Blynk App.
	}															/** Only required if using Blynk */
if ((doorstate != lastdoorstate) || (now - lastPublish) > 600000){		/** Only required if using MQTT */ // publish all state changes and also send at least every 10 minutes
 
    StaticJsonDocument<256> JSONdoc;							/** Only required if using MQTT */ //Alocate RAM for JSON document. MQTT will restrict packets to 256 bytes
 
    JSONdoc["cover"] = cover.c_str();							/** Only required if using MQTT */ //add values to the JSON doc

	  Serial.println(cover);

    char buffer[256];											/** Only required if using MQTT */ //create a temporary buffer for the serialised data
    serializeJson(JSONdoc, buffer);								/** Only required if using MQTT */ //generate the JSON string -e.g. {"cover":"open"} and send it to buffer

    client.publish("garagedoor/state", buffer);				    /** Only required if using MQTT */  
    lastPublish = now;											/** Only required if using MQTT */
  }															    /** Only required if using MQTT */

}																


void reconnect() {												/** Only required if using MQTT */  // Reconnect to MQTT   //Comment out entire function if using Blynk only and/or remove the function call at the top of checkDistance().

  while (!client.connected()) {									/** Only required if using MQTT */  // Attempt to connect, Loop until we're reconnected
    Serial.println("Attempting MQTT connection...");			/** Only required if using MQTT */  
    
    if (client.connect("GarageDoor",mqttUser, mqttPassword)) {	/** Only required if using MQTT */  
      Serial.println("connected");								/** Only required if using MQTT */  

      client.subscribe("garagedoor/operate");      				/** Only required if using MQTT */  // Subscribe to 'operate' topic
    } else {													/** Only required if using MQTT */  
      Serial.print("failed, rc=");								/** Only required if using MQTT */  
      Serial.println(client.state());							/** Only required if using MQTT */  
    }															/** Only required if using MQTT */  
  }																/** Only required if using MQTT */  

}																/** Only required if using MQTT */  

void loop() {
  server.handleClient();										// Listen for HTTP requests from clients for OTA Update
  client.loop();												/** Only required if using MQTT */
  Blynk.run();													/** Only required if using Blynk */
  timer.run();													//Uses Blynk timer if Blynk enabled, simpletimer.h if only MQTT is enabled (include Simpletimer.h library uncommented and include BlynkSimpleEsp32 commented out).  
}
