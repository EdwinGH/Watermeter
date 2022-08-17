# Measuring home water usage with sensor on watermeter

I decided to measure the water use at home, and based my own implementation on the following sources:
* https://www.huizebruin.nl/home-assistant/esphome/watermeter-uitlezen-in-home-assistant-met-esphome/?v=796834e7a283
* https://gathering.tweakers.net/forum/list_messages/1642520/1
* https://www.mysensors.org/build/pulse_water
* https://github.com/buldogwtf/watermeter_esp8236_TCRT5000_mqtt_node-red/blob/master/watermeter.ino

I am using a NodeMCU ESP8266 board with an inductive proximity sensor, and the sensor is connected to the Vin, GND, and D4 (GPIO2). The board is powered with a micro USB connection. I put the board in a watertight case, and used a long micro USB cabel to put the power supply away from the watermeter, as the (underfloor) area is quite humid in my case.

(picture missing)

This is how the sensor is 'attached' to the Elster / Honeywell V200 watermeter (which is owned by the water utility company, so don't make any modifications to it!):

![Setup](https://github.com/EdwinGH/Watermeter/blob/main/Watermeter%20setup.jpg)

This watermeter has a rotating element with iron, which rotates once per liter. 
Note that there are different sizes and voltages and NPN/PNP proximity sensors, so make sure to buy the correct one (I use 5V NPN).

I uploaded the Watermeter.ino file to the board; it does the following setup:
* Open serial connection to write monitoring / debugging messages
* Connect to WiFi
* Connect to MySQL database and download the latest logged pulse count
* Set an IRQ callback on falling pulse of induction / prox sensor
In the loop it does the following:
* See if SEND_FREQUENCY time has passed (default 30 seconds), and upon passing
* * Update the flow
* * If there are pulses received
* * * Re-calculate the used water volume based on pulses
* * * Upload the new pulse count, water volume and flow to the MySQL database

The used MySQL table can be created with:
```
CREATE TABLE `water` (
 `datetime` datetime NOT NULL,
 `pulseCount` int NOT NULL,
 `volume` decimal(10,3) NOT NULL,
 `flow` decimal(4,1) NOT NULL,
 PRIMARY KEY (`datetime`),
 UNIQUE KEY `datetime` (`datetime`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COMMENT='Water meter readings'
```

The NodeMCU does not do to sleep (yet?), as pulses can come all the time. Maybe in a next release I can connect the sensor to the wake-up pin, but need to check later...
