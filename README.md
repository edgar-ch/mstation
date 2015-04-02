mstation
========

Arduino-based weather station

Now support (v0.1.x)
--------------------

* temperature measurement
* humidity measurment
* sending data to PC and web server
* sending data to FTP server in CSV format
* display current measured parameters on LCD display

Where that is
-------------

**MStation/** - Arduino sketch  
**schematic/** - scheme of connection of sensors and display (use [Fritzing](http://fritzing.org/home/ "Fritzing") to view)  
**web_serv/** - code for manipulations with data on web server  
**mstation_serv.py** - collect data from Arduino using serial port and sending it to web (Web, FTP, OpenWeatherMap)  
**mstation.conf** - config to autostart for *mstation_serv.py* (using upstart)  
**mstation_py.conf** - version of config for Raspberry Pi (set system time using NTP before starting)  

Future
------

Now I'm seriously rework the project, plans to give up the connection by wire connection, use the radios, the ability to run on battery power, etc.  
All changes in the folder *new_version/*  
