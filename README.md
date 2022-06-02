# Bike Tracker with ESP32 (Kolotrek)
## Overview
This is a simple device that allows a bike to be located with the help of GPS. It uses a SIM card for communication by sending and receiving SMS messages. Another nice feature is the "Parking mode", in which an accelerometer checks for movement and warns the owner by calling him if the bike is manipulated.

## Commands
- help – shows all possible commands
- parking mode – on/off – turns on or off the parking mode
- status – sends battery level and last gps location

## Code logic
- Stores the last GPS signal every 5 minutes.
- When it's at 10% or 5%, a status message gets sent. 
- In parking mode, when the accelerometer detects that the bike has moved, it immediately alerts the owner by calling him.
- If the gps signal goes down, it keeps the last gps location. If the signal drops out for more than 5 minutes, a text message with the last gps location is sent. The - - device will try to connect to cell tower gps, if it succeeds it will send a status saying it is currently using cellular data until it catches the gps signal again.
- If the cell signal drops out and something happened in that time (bike moved, lost gps etc..) a status as soon gets sent as soon as it gets a signal.