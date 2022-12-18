
# ESP32-Garage-DoorController-V2-MQTT-Blynk
An ESP32 Garage Door Controller updated to publish door state and subscribe to door control messages via MQTT.

It also provides a mobile phone app with the updated Blynk.IOT platform, showing the door state, distance and allowing control via an app button. Alerts can pop up on your phone to notify you if  the garage door has been left open, and these alerts can be disabled by a switch in the app. 

Using IFTTT with Blynk, it also allows door operation via Google voice control.

It uses mDNS to appear as garagedoor.local on the network, and has OTA enabled for firmware updates (default username  and password are both 'update') so it doesn't need to be removed from the ceiling to update the firmware. 

<H2>How does it work? </H2><BR>
The ESP32/ultrasonic sensor is mounted on the garage ceiling and measures distance. With the door closed it will measure the distance from thew ceiling to either the garage floor, or to a parked car (so a bonus measure to show someone is home). The sensor is positioned so if the door is open it crosses into the ultrasonic sensor path and the distance will be much smaller. The 'open' distance threshold is set below at 40cm  (in my case it measures around 12cm from the ceiling to the open door). 

Without a car parked the distance to the floor is 240cm, and with a car parked it measures ~80cm so simple to set up sensor templates in Home Assistant based on the 
distance topic. Finally, there's a Blynk alert for a garage door that's been left open for too long (set at 3 minutes). 
  
When triggered by either MQTT or Blynk (or Google to IFTTT to Blynk via webhook) it toggles pin 32 high for 0.5 seconds. This drives a 5v relay board which toggles the open/close switch pins on the garage door controller, as if you were pushing a wired button.  My Garage door controller has only one button for open/close. It would be relatively simple to alter the code to add seperate open and close relays on different pins. 
  
<H2>Parts </H2><BR>
<li>ESP32 (I used a devkitC but almost any dev board will do. I've chosen pins that are all on one side of the ESP32 DevkitC just to save space in the box (breaking out  wiring on one side only)
<li>A 5v DC relay board
<li>An ultrasonic sensor board
<li>Optionally - in place of the ultrasonic sensor board, you can use a reed switch to sense the state of the door. 

<H2>Wiring </H2><BR>
<img src="https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Schematic.jpg?sanitize=true&raw=true" width="33%" "height=33%" />



<H2>Optional </H2><BR>
I've added code to use a reed switch to check the door state. I'm not too keen on this approach as it means running wires to the bottom of the door and mounting the magnet on a door and the reed switch on the frame. Just seems messy. 

But if you don't want to use the ultrasonic method, or if you have a roller door and not a panel/tilt door, this can be enabled and the ultrasonic sensor disabled in code (just change which function is called).
Using a pull-down resistor the signal will be LOW if the switch is open, so - a normally open reed switch (closed when the magnet is nearby) will be HIGH when the door is closed and LOW when the door is opened and the magnet moves away. 

<H2>MQTT </H2><BR>
MQTT messages are published in JSON for changes in state (in the format e.g. {"cover":"open","distance":15}  or if using the reed switch option (see "Optional" below) just the cover message e.g. {"cover":"open"} ) on the "garagedoor/state" topic.
The MQTT topic "garagedoor/operate" listens for a "toggle" message 

<H2>Home Assistant </H2><BR>
To read the values in Home Assistant via MQTT 

<B>Garage Door inclusions in configuration.yaml</B><BR>
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

  button:
    - command_topic: "garagedoor/operate"
      availability_topic: "zigbee2mqtt/bridge/state"
      name: "Garage Door"
      unique_id: garage_door_operate_btn
      payload_press: "toggle"
      retain: false
      entity_category: "config"
```
<h3>Automations</h3><br>
Automations should be configured through the Home Assistant GUI:  <br>
<img src="https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Automations.jpg?sanitize=true&raw=true" width="33%" "height=33%" />

<b>Sample in automations.yaml</b><br>
  
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

<h3>Lovelace Card</h3><br>
<img src="https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/lovelace.jpg?sanitize=true&raw=true" width="33%" "height=33%" />
  
<B>Sample ui-lovelace.yaml</b><br>
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

<H2>Blynk </H2><BR>
I use Blynk (with IFTTT)  for the convenience of door control via my mobile phone, and for Google Home integration via IFTTT. Blynk is a free service for personal use as long as you have only a few devices to control. 

Blynk essentially provides virtual pins and devices that you can use to control and read from youe ESP32 remotely. 

I won't go in to the whole Blynk how-to, but you need to create an account on the web, log in and enable developer mode, then create a template with datastreams and a dashboard.  It's simpler than it sounds.
  
<h3>Datastreams Tab</h3><br>
<img src="https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Datastreams.jpg?sanitize=true&raw=true" width="33%" "height=33%" />

<H3>Events Tab</h3><br>  
Under the template "Events" tab, you'll need to add the event which sends the door open warnings:<br>
<img src="https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Event.jpg?sanitize=true&raw=true" width="33%" "height=33%" />  

  And on the second tab, 'Events' tab, "Notifications"  
  <h3> </h3><br>
  
<img src="https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Event Notifications.jpg?sanitize=true&raw=true" width="33%" "height=33%" />  
  
You can then download the blynk.iot mobile app and set up a "Mobile Dashboard" and add elements to the screen to create a Mobile App.<br>
<img src="https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Datastreams - Mobile Template.png?sanitize=true&raw=true" width="33%" "height=33%" /> 
<li>Advanced LCD - Attached to Datastream v3 <a href="https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Datastream%20Settings%20-%20Mobile%20-%20Display%20V3.png" 1></a>
<li>Pushbutton - Attached to Datastream V7 <a href="https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Datastream%20Settings%20-%20Mobile%20-%20PressButton%20V7.png" 2></a>
<li>Integer Display - Attached to Datastream V5 <a href="https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Datastream Settings - Mobile - Integer V5.png" 3></a>
<li>Switch - Attached to Datastream V0 <a href="https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Datastream Settings - Mobile - Switch V7.png" 4></a>


<BR><H3> </H3><br>

<img src="https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Web App.jpg?sanitize=true&raw=true" width="33%" "height=33%" />

You then use the template to create a "device" in Blynk, and this device page provides the "Firmware Configuration" - the template id and auth token you will use at the top of your sketch to connect your ESP32 to Blynk services - e.g.  

```C++
#define BLYNK_TEMPLATE_ID "TMPL**********"
#define BLYNK_DEVICE_NAME "Garage Door Controller V2"
#define BLYNK_AUTH_TOKEN "1234567abcdeFGhiJKlMnOp"
```

  
You will then have a web app to control your garage door. 
<BR><H3> </H3><br>

<img src="https://github.com/StudioShemp/ESP32-Garage-DoorController-V2-MQTT-Blynk/blob/main/images/Web App.jpg?sanitize=true&raw=true" width="33%" "height=33%" />
  
<BR><h2>Google Home</h2> <BR>
It's easy to set up voice activation using Google Home by setting up  IFTTT/Google Home integration. This lets you say "hey google, activate garage door" to call a webhook to your Blynk service. You will need your BLYNK_AUTH_TOKEN to set this up. 
