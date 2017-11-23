# NexStarAdapter

A USB interface for [Celestron](https://www.celestron.com/collections/astronomy)
telescopes using the NexStar AUX protocol.

![](images/nexstar_adapter1.jpg)


## Description

This is a USB adapter for Celestron NexStar AUX protocol implemented in Arduino.
It acts like a hand controller, receiving the commands from the PC (HC procotol)
and communicating with the mount (AUX protocol).

![](images/block_diagram.png)

Documentation:

 * [NexStar HC Protocol](http://www.nexstarsite.com/download/manuals/NexStarCommunicationProtocolV1.2.zip)
 * [NexStar AUX Protocol](http://www.paquettefamily.ca/nexstar/NexStar_AUX_Commands_10.pdf)


## Compiling

The code can be compiled using the standard windows Arudino IDE. Jaun has an excellent site on doing this in Linux however my fork is based on Windows only. I am planning to use Mini PC for observatory control. 

![](images/circuit.png)


## Enclosure

The CAD directory contains an STL model of the enclosure I use, which was
created with openSCAD. It requires an Arduino Nano and a small breadboard.
