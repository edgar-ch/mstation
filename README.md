mstation
========

Arduino-based weather station

New version (v0.2.x)
--------------------

**Currently in begining of development. Very BAD idea to use it**  
Will support:  
* Measurement module:  
  * temperature measurement
  * humidity measurement measurement
  * pressure and ambient light measurement
  * sending data to base station
  * sleeping mode for low power consumption
  * wireless communication using nRF24L01+
  * RTC
  * choosing measurement time
* Base station module
  * sending data to PC
  * display current measured parameters on LCD display
  * gathering data from multiple measurement modules
* PC and server modules
  * sending data to FTP and WEB server in JSON format
  * setup setting for measurement modules
  * access to the measured data

Where that is
-------------

**MStation*/** - Arduino sketch  
**schematic/** - scheme of connection of sensors and display (use [Fritzing](http://fritzing.org/home/ "Fritzing") to view)  
**web_serv/** - code for manipulations with data on web server  
**mstation_serv.py** - collect data from Arduino using serial port and sending it to web (Web, FTP, OpenWeatherMap)  
**config/mstation.conf** - config to autostart for *mstation_serv.py* (using upstart)  
**config/mstation_py.conf** - version of config for Raspberry Pi (set system time using NTP before starting)  
**config/mstation_logrotate** - configuration file for logrotate service. 



For Python scripts
------------------

What should you do for use python scripts:

* Install **python 2 last release** (not needed if you operational system is Linux based)

* Install **pip** package:
  * Download [Get-pip.py](https://bootstrap.pypa.io/get-pip.py)
  * *python get-pip.py*

* Install **virtualenv**:
  * *pip install virtualenv*

* Make the virtual environment outside you working directory and activate it:
  * *cd ../*
  * *virtualenv env*
  * *. env/bin/activate*

* Install all dependencies:
  * *cd mstation*
  * *pip install -r requirements.txt*
