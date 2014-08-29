#!/usr/bin/env python
# -*- coding: utf-8 -*-

import serial
from serial import SerialException
import datetime
import re
import ftplib
import socket
import os, sys
import time
import signal
import logging
from logging.handlers import WatchedFileHandler
import threading
import requests
from requests.exceptions import *

LOG_PATH = '/var/log/mstation.log'
LOG_FORMAT = '[%(asctime)s] %(levelname)s: %(funcName)s: %(message)s'
SERIAL_DEV = '/dev/ttyACM0'
FTP_SERV = 'ftp_serv'
FTP_NAME = 'ftp_name'
FTP_PASS = 'ftp_password'
DATA_PATH = '/usr/local/MStation/data/'
CONNECT_TRYOUT = 10
UPLOAD_URL = 'upload_handler'
# OpenWeatherMap
URL_OPENW = 'http://openweathermap.org/data/post'
LATITUDE = 56.3294
LONGITUDE = 37.7532
ALTITUDE = 215
USER_NAME = 'user_name'
PASSW = 'password'
STATION_NAME = 'station_name'
# OpenWeatherMap

write_lock = threading.Lock()
upload_lock = threading.Lock()

failed_upload = []

log = logging.getLogger('main_log')
log.setLevel(logging.INFO)
log_handler = WatchedFileHandler(LOG_PATH)
log_fmt = logging.Formatter(fmt=LOG_FORMAT)
log_handler.setFormatter(log_fmt)
log.addHandler(log_handler)

def main():
	signal.signal(signal.SIGINT, ctrl_c_handler)
	f_name = datetime.datetime.now().strftime('%Y-%m-%d.cvs')
	f_path = DATA_PATH + f_name
	f_table = open(f_path, 'a', 0)

	ser = serial_connect(SERIAL_DEV, 115200)

	init_patt = re.compile(r'MStation')
	curr_date = datetime.date.today()

	while True:
		try:
			curr = ser.readline()
		except SerialException as e:
			log.error('Serial error: ' + str(e))
			time.sleep(120)
			ser = serial_connect(SERIAL_DEV, 115200)
			continue

		init_match = re.search(init_patt, curr)

		if init_match:
			log.info(curr)
			continue
		if curr_date.day <> datetime.date.today().day:
			f_table.close()
			f_name = datetime.datetime.now().strftime('%Y-%m-%d.cvs')
			f_path = DATA_PATH + f_name
			f_table = open(f_path, 'a', 0)
			curr_date = datetime.date.today()

		date_str = datetime.datetime.now().strftime('%Y/%m/%d %H:%M')
		final_str = date_str + ',' + curr

		ftp_thread = threading.Thread(target=upload_to_ftp, args=(FTP_SERV, FTP_NAME, FTP_PASS, f_name))
		upl_thread = threading.Thread(target=upload_to_site, args=(UPLOAD_URL, final_str))
		openw_thread = threading.Thread(target=upload_to_openweathermap, args=[final_str])
		
		write_lock.acquire()
		f_table.write(final_str)
		write_lock.release()
		#upload_to_ftp(FTP_SERV, FTP_NAME, FTP_PASS, f_name)
		ftp_thread.start()
		upl_thread.start()
		openw_thread.start()
	
	sys.exit(0)

def serial_connect(dev, speed):
	try_count = 1

	while try_count <= CONNECT_TRYOUT:
		try:
			ser = serial.Serial(dev, speed)
		except SerialException as e:
			log.error('Serial Connect Error: ' + str(e))
			print 'Connection Error'
			time.sleep(120)
		else:
			log.info('Connected after ' + str(try_count) + ' tryouts')
			return ser
		finally:
			try_count += 1
	
	log.critical('FAIL to connect. Exit.')
	sys.exit(-1)
	

def upload_to_ftp(host, name, passw, f_name):
	path = DATA_PATH + f_name
	upl_file = open(path, 'rb')
	name_new = f_name + '.new'
	try:
		ftp_serv = ftplib.FTP(host, name, passw, '', 30)
		ftp_serv.cwd('public_html')
		ftp_serv.voidcmd('TYPE I')
		ftp_serv.voidcmd('PASV')
		write_lock.acquire()
		ftp_serv.storbinary('STOR '+name_new, upl_file)
		upl_size = ftp_serv.size(name_new)
		if upl_size:
			f_size = os.path.getsize(path)
			if upl_size == f_size:
				ftp_serv.rename(name_new, f_name)
			else:
				raise uplFail('sizes not match')
		else:
			raise uplFail('unknown error')
		ftp_serv.close()
	except (socket.error, socket.gaierror) as e:
		log.error('FTP Socket Error: ' + str(e))
	except ftplib.error_temp as e:
		log.error('FTP Temporary Error: ' + str(e))
	except ftplib.error_perm as e:
		log.error('FTP Permanent Error: ' + str(e))
	except ftplib.error_reply as e:
		log.error('FTP Unexpected Error: ' + str(e))
	except uplFail as e:
		log.error('FTP Upload Failed: ' + str(e))
	else:
		log.info('FTP Upload Success')
	finally:
		if write_lock.locked():
			write_lock.release()
	
	upl_file.close()

class uplFail(Exception):
	def __init__(self, value):
		self.value = value
	def __str__(self):
		return repr(self.value)
		
def upload_to_site(url, data_str):
	if not upload_to_url(url, data_str):
		failed_upload.append(data_str)
	else:
		if upload_lock.acquire(False):
			while len(failed_upload) <> 0:
				data = failed_upload[0]
				if upload_to_url(url, data):
					failed_upload.pop(0)
			upload_lock.release()
		
def upload_to_url(url, data_str):
	data_send = data_str.split(',')
	data_req = {'date': data_send[0], 'temp1': data_send[1], 'temp2': data_send[2], 'humid': data_send[3]}
	
	try:
		req = requests.post(url, data=data_req, timeout=60)
	except (ConnectionError, HTTPError, URLRequired, Timeout) as e:
		log.error(str(e))
		return False
	else:
		if (req.status_code == requests.codes.ok):
			log.info('Upload success.')
			return True
		else:
			log.error('Upload failed with code ' + str(req.status_code))
			return False
			
def upload_to_openweathermap(data_str):
	data_send = data_str.split(',')
	data_post = {'temp': data_send[1], 'humidity': data_send[3],
				'lat': LATITUDE, 'long': LONGITUDE, 'alt': ALTITUDE, 
				'name': STATION_NAME}
	try:
		req = requests.post(URL_OPENW, auth=(USER_NAME, PASSW), 
							data=data_post, timeout=60)
	except (ConnectionError, HTTPError, URLRequired, Timeout) as e:
		log.error(str(e))
	else:
		if (req.status_code == requests.codes.ok):
			log.info('Upload to OpenWeather success.')
		else:
			log.error('Upload to OpenWeather failed with code ' + str(req.status_code))

def ctrl_c_handler(signum, frame):
	print 'Exit.'
	log.info('Exit.')
	logging.shutdown()
	sys.exit(0)

if __name__ == '__main__':
	main()

