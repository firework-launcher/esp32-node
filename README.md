# ESP32 Node

## Description
These use a custom board with an ESP32S3 on it. The board has an arm circuit, mosfets for controlling 16 channels along with their PWM. The board also has a header for connecting a display board which has WS2811s, and has an I2C bus that controls three digit displays, and an arm led. The arm circuit has a physical button that needs to be turned on to arm it, as well as the software arm that controls a relay. Both arms need to be on in order to light any fireworks.

## Auto Discovery
This code has an auto discovery feature that uses UDP broadcasts from the controller.

## Protocol
The protocol that the controller uses to talk to these nodes is explained [here](protocol.md).

## OTA
The nodes have an OTA feature, which can be triggered by sending code 5 to it. This turns everything off in the node, and turns on a webserver, which can be used to configure the node's wifi configuration, as well as reprogramming the node remotely. The node automatically enters OTA if there is no wifi configuration, or the current configuration fails. The ota_scripts directory has some scripts for interacting with the nodes in OTA.

## Display board
The WS2811s indicate whether a firework is connected or not, by showing red/green for every one of the 16 LEDs when it is not armed. When it is armed, all LEDs turn off, and turn on with varying brightness depending on the PWM when a firework is launched. The digit displays normally show the last number in the IP address. When the node goes into OTA for if the wifi configuration is broken, it hosts a wifi hotspot, with the SSID looking something like this: Node 74. The 74 would be a random number created by the node. It will also show this random number on the digit displays.