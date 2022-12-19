
# ESP32-Garage-DoorController-V2-MQTT-Blynk
An ESP32 Garage Door Controller updated to publish door state and subscribe to door control messages via MQTT.

It also provides a mobile phone app with the updated Blynk.IOT platform, showing the door state, distance and allowing control via an app button. Alerts can pop up on your phone to notify you if  the garage door has been left open, and these alerts can be disabled by a switch in the app. 

Using IFTTT with Blynk, it also allows door operation via Google voice control.

It uses mDNS to appear as garagedoor.local on the network, and has OTA enabled for firmware updates (default username  and password are both 'update') so it doesn't need to be removed from the ceiling to update the firmware. 

## How does it work? 
The ESP32/ultrasonic sensor is mounted on the garage ceiling and measures distance. With the door closed it will measure the distance from thew ceiling to either the garage floor, or to a parked car (so a bonus measure to show someone is home). The sensor is positioned so if the door is open it crosses into the ultrasonic sensor path and the distance will be much smaller. The 'open' distance threshold is set below at 40cm  (in my case it measures around 12cm from the ceiling to the open door). 

Without a car parked the distance to the floor is 240cm, and with a car parked it measures ~80cm so simple to set up sensor templates in Home Assistant based on the 
distance topic. Finally, there's a Blynk alert for a garage door that's been left open for too long (set at 3 minutes). 
  
When triggered by either MQTT or Blynk (or Google to IFTTT to Blynk via webhook) it toggles pin 32 high for 0.5 seconds. This drives a 5v relay board which toggles the open/close switch pins on the garage door controller, as if you were pushing a wired button.  My Garage door controller has only one button for open/close. It would be relatively simple to alter the code to add seperate open and close relays on different pins. 

![Ceiling](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/CeilingMount.jpg) 
![MeasuringDoor](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/DoorOpen.jpg)

Ceiling mounted, and showing the position of the open door, where the ultrasonic sensors measure a shorter distance and report the 'open' state.

## Getting it going
The code is commented so that if you don't want Blynk or don't use MQTT, those sections are easy to remove. 

To get everything running you'll need to replace the following lines of code with your own settings:
```c++
#define BLYNK_TEMPLATE_ID "XXXXXXXX"   /** Only required if using Blynk */ //get this when you create your Blynk device on https//blynk.console
#define BLYNK_DEVICE_NAME "Garage Door Controller V2"   /** Only required if using Blynk */ //get this when you create your Blynk device on https//blynk.console 
#define BLYNK_AUTH_TOKEN "XXXXXXXX"    /** Only required if using Blynk */ //get this when you create your Blynk device on https//blynk.console

char ssid[] = "XXXXXXXX";	  //replace with your wifi SSID
char pass[] = "XXXXXXXX";	  //replace with your wifi password

const char* mqttUser = "XXXXXXXX";     /** Only required if using MQTT */ //replace with your Mosquitto username
const char* mqttPassword = "XXXXXXXX"; /** Only required if using MQTT */ //replace with your Mosquitto password

char mqtt_server[] = "XXXXXXXX";   /** Only required if using MQTT */  //replace with your own MQTT Server IP Address e.g. char mqtt_server[] = "192.168.1.254";
```

## Parts 
- ESP32 (I used a devkitC but almost any dev board will do. I've chosen pins that are all on one side of the ESP32 DevkitC just to save space in the box (breaking out  wiring on one side only)
- A 5v DC relay board
- An ultrasonic sensor board
- A small case. I used a small Jaycar or Altronics project box.  
- Optionally - in place of the ultrasonic sensor board, you can use a reed switch to sense the state of the door. 



## Wiring 

![Schematic](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Schematic.jpg)

## Assembled
![Schematic](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Innards.jpg)

## Optional 

I've added code to use a reed switch to check the door state. I'm not too keen on this approach as it means running wires to the bottom of the door and mounting the magnet on a door and the reed switch on the frame. Just seems messy. 

But if you don't want to use the ultrasonic method, or if you have a roller door and not a panel/tilt door, this can be enabled and the ultrasonic sensor disabled in code (just change which function is called).
Using a pull-down resistor the signal will be LOW if the switch is open, so - a normally open reed switch (closed when the magnet is nearby) will be HIGH when the door is closed and LOW when the door is opened and the magnet moves away. 

## MQTT 

MQTT messages are published in JSON for changes in state (in the format e.g. {"cover":"open","distance":15}  or if using the reed switch option (see "Optional" below) just the cover message e.g. {"cover":"open"} ) on the "garagedoor/state" topic.
The MQTT topic "garagedoor/operate" listens for a "toggle" message 

## Home Assistant 

To read the values in Home Assistant via MQTT make the following changes to the MQTT settings in configuration.yaml

## Garage Door sensor templates in the MQTT section of configuration.yaml

```YAML
mqtt:
  sensor:
    - state_topic: "garagedoor/state"
      availability_topic: "zigbee2mqtt/bridge/state"
      icon: "mdi:garage-open"
      name: "Garage Door Distance"
      unique_id: garage_door_distance
      unit_of_measurement: "cm"
      value_template: '{{ value_json.distance }}'

    - state_topic: "garagedoor/state"
      availability_topic: "zigbee2mqtt/bridge/state"
      icon: "mdi:garage-open"
      name: "Garage Door Cover"
      unique_id: garage_door_cover
      value_template: '{{ value_json.cover }}'
```

## Garage Door button template in the MQTT section of configuration.yaml
```yaml
  button:
    - command_topic: "garagedoor/operate"
      availability_topic: "zigbee2mqtt/bridge/state"
      name: "Garage Door"
      unique_id: garage_door_operate_btn
      payload_press: "toggle"
      retain: false
      entity_category: "config"
```
## Automations

Automations should be configured through the Home Assistant GUI: 

![Automations](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Automations.jpg)

#### This should yield something like this in automations.yaml

  
```YAML
- id: '1670604494618'
  alias: Operate Garage Door
  description: ''
  trigger: []
  condition: []
  action:
  - service: button.press
    data: {}
    target:
      entity_id: button.garage_door
  mode: single
```

## Lovelace Card

![Lovelace](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/lovelace.jpg)
  
#### Sample ui-lovelace.yaml

```YAML
views:
    cards:
      - type: entities
        title: "Garage Door"
        entities:
        - entity: button.garage_door
        - entity: sensor.garage_door_distance
        - entity: sensor.garage_door_state
```




## Blynk 

I use Blynk (with IFTTT)  for the convenience of door control via my mobile phone, and for Google Home integration via IFTTT. Blynk is a free service for personal use as long as you have only a few devices to control. 

Blynk essentially provides virtual pins and devices that you can use to control and read from youe ESP32 remotely. 

I won't go in to the whole Blynk how-to, but you need to create an account on the web, log in and enable developer mode, then create a template with datastreams and a dashboard.  It's simpler than it sounds.
  
### Datastreams Tab
![Datastreams](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Datastreams.jpg)

#### Mobile view of Datastreams
![Mobile DataStreams](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Datastreams%20-%20Mobile%20Template.png)


#### Advanced LCD Datastream setup 
+Advanced LCD - Attached to Datastream v3 <br>
![image](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Datastream%20Settings%20-%20Mobile%20-%20Display%20V3.png)
  <br>


#### Pushbutton Datastream setup
+Pushbutton - Attached to Datastream V7 <br>
 ![image](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Datastream%20Settings%20-%20Mobile%20-%20PressButton%20V7.png)


#### Integer Display Datastream setup
+Integer Display - Attached to Datastream V5<br>
![image](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Datastream%20Settings%20-%20Mobile%20-%20Integer%20V5.png)


#### Switch Datastream setup
+Switch - Attached to Datastream V0 <br>
![image](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Datastream%20Settings%20-%20Mobile%20-%20Switch%20V0.png)

### Events Tab

Under the template "Events" tab, you'll need to add the event which sends the door open warnings:<br>
![Events](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Event.jpg)

  And on the second tab, 'Events' tab, "Notifications"  
  
![Events2](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Event%20Notifications.jpg)
  
You can then download the blynk.iot mobile app and set up a "Mobile Dashboard" and add elements to the screen to create a Mobile App.<br>


### Mobile App Layout
![image](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Mobile_template.png)
  <br> 
  This image shows the elements/widgets you would include on the Blynk mobile app. 
 + LCD (Advanced LCD) - Grey box at top of screen (configured to use Virtual pin V3 from your datastreams)
 + Pushbutton - momentary, the button you will use to open and close the garage door  (configured to use Virtual pin V7 from your datastreams)
 + Integer display - shows the distance readout from the ultrasonic sensor
 + Button configured as an on/off switch. This is the button you would press if you wanted to mute "door open" alerts

#### Advanced LCD widget setup 
+Advanced LCD - V3 <br>
![image](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/LCD%20Settings%20-%20Mobile%20-%20V3.png)
  <br>


#### Pushbutton widget setup
+Pushbutton - V7 <br>
 ![image](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Pushbutton%20Settings%20-%20Mobile%20-V7.png)


#### Integer Display witdget setup
+Integer Display - V5<br>
![image](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Value%20Display%20Settings%20-%20Mobile%20-%20V5.png)


#### Switch widget setup
+Switch - V0 <br>
![image](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Switch%20Settings%20-%20Mobile%20-%20V0.png)


#### Note
There is also a "web" version of this layout that is set up in the developer settings in the Blynk developer web site. ie - It sets up a web page with similar buttons / displays as the mobile app, so you could control your device via a web page if that's your thing. Blynk advise that this should also be done but I'm not certain it's necessary.   



### Setting it all up
Once you have your datastreams set up, and your app layout set up and connected to the datastreams, it's time to use the template to create a "device" in the Blynk devloper site. This is a critical step as it provides the "Firmware Configuration" - the template id and auth token you will use at the top of your sketch to connect your ESP32 to Blynk services and use the app. 

e.g.
```C++
#define BLYNK_TEMPLATE_ID "TMPL**********"
#define BLYNK_DEVICE_NAME "Garage Door Controller V2"
#define BLYNK_AUTH_TOKEN "1234567abcdeFGhiJKlMnOp"
```

Once you've added this in to the sketch, along with your Wifi and MQTT server details you're ready to push the code to the ESP32.   
You should then see the device go "active" in the Blynk device web page, and the app should control your garage door and display the state. 

![Web App](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Web%20App.png) 
  


## Google Home
<BR>
It's easy to set up voice activation using Google Home by setting up  IFTTT/Google Home integration. This lets you say "hey google, activate garage door" to call a webhook to your Blynk service. You will need your BLYNK_AUTH_TOKEN, and an MQTT account linked to Google Home. 

### Setting it up

You will need to log in to MQTT and set up a new applet using the Google Home V2 service. The images below should help, but if you're in trouble, there's an excellent article at iotcircuithub.com here 
[IFTTTBLYNK](https://iotcircuithub.com/ifttt-blynk-url-for-google-assistant/)

### Applet
![IFTTT1](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/IFTTT-Applet.jpg)

### IF

![IFTTT2](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/IFTTT-If.jpg)

### Then

![IFTTT3](https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/IFTTT-Then.jpg)


