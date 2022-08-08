# Smart Rain Barrel

The best way for me to participate in Justine’s hobbies is to over-engineer them and her garden is no different. To reduce our water use I came up with a smart water barrel that:

* Can turn the pump for the barrel water on and off
* Has four float switches to detect the barrel water level
* Uses two valves to select barrel water or city water
* Monitors two soil moisture sensors to determine if watering is needed
* Connects to my Home Assistant automation server for control and logging
* Uses the weather forecast to postpone watering if rain is imminent

##Software
A esp8266 running nodemcu firmware runs an arduino program to act as the interface between Home Assistant and the rain barrel hardware. MQTT, a simple messaging service for IOT devices, is used as the messaging protocol.  The rain barrel understands command messages and it sends status messages on a regular basis. 

<img src="https://mechied.com/wp-content/uploads/2022/07/explainer-graphic.png" width="800">

It’s very easy to do things like send the water level once a second and then have Home Assistant recognize that as sensor data and associate it with a device. Here’s what the rain barrel dashboard looks like in Home Assistant.

<img src="https://mechied.com/wp-content/uploads/2022/05/image1.png" width="300">
<img src="https://mechied.com/wp-content/uploads/2022/05/image.png" width="300">

One page is the controls where you can actuate the valves and turn the pump on and off.  There’s also two commands you can send to the rain barrel to enable faster logging of the soil moisture (because I’m using an extra mechanical relay to mux the two soil moisture sensors to one analog input, I only check the soil moisture a few times an hour because otherwise the relay would have hundreds of thousands of cycles in only a few years of operation).  The one soil moisture sensor is unplugged at the moment so the readings are screwy.  For that matter, I’m still trying to figure out if the capacitive soil moisture sensor actually produces any useful data.  The water level data seem reasonably well behaved after lots of averaging.

The other page is a weather dashboard centered around the weather forecast for the next few days including a grid of dials that show the amount of rain and probability of the next four days.

## Hardware 

The components are all installed in a waterproof project box. I 3D printed brackets to hold the miscellaneous circuit boards. I used various waterproof cable glands and cheap waterproof connectors to connect the pumps, valves, and sensors to the box. The wiring is messy in this photo because I didn’t want to lock it down until all the programming is done. Here is [a wiring diagram](https://mechied.com/37d0a3ae-e11d-42e2-9f07-c8ca1182d6ed) 

<img src="https://mechied.com/wp-content/uploads/2022/05/sensor-array-768x1024.jpg" width="300">

<img src="https://mechied.com/wp-content/uploads/2022/05/rain-barrel-control-768x1024.jpg" width="300">

<img src="https://mechied.com/wp-content/uploads/2022/05/full-barrel-scaled.jpg" width="300">

<img src="https://mechied.com/wp-content/uploads/2022/05/valves-rain-barrl-scaled.jpg" width="300">


