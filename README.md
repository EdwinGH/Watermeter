# Watermeter

I decided to measure the water use at home, and based my own implementation on the following sources:
* https://www.huizebruin.nl/home-assistant/esphome/watermeter-uitlezen-in-home-assistant-met-esphome/?v=796834e7a283
* https://gathering.tweakers.net/forum/list_messages/1642520/1
* https://www.mysensors.org/build/pulse_water
* https://github.com/buldogwtf/watermeter_esp8236_TCRT5000_mqtt_node-red/blob/master/watermeter.ino

I am using a NodeMCU ESP8266 board with an inductive proximity sensor, and the sensor is connected to the Vin, GND, and D4 (GPIO2). Note that there are different sizes and voltages and NPN/PNP proximity sensors, so make sure to buy the correct one (I use 5V NPN).

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

The NodeMCU does not do to sleep (yet?), as pulses can come all the time. Maybe in a next release I can connect the sensor to the wake-up pin, but need to check later...
