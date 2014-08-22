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
import threading

LOG_PATH = '/var/log/mstation.log'
LOG_FORMAT = '[%(asctime)s] %(levelname)s: %(funcName)s: %(message)s'
SERIAL_DEV = '/dev/ttyACM0'
FTP_SERV = 'ftp_serv'
FTP_NAME = 'ftp_name'
FTP_PASS = 'ftp_password'
DATA_PATH = '/usr/local/MStation/data/'
CONNECT_TRYOUT = 10

logging.basicConfig(format = LOG_FORMAT, level = logging.INFO, filename = LOG_PATH)

write_lock = threading.Lock()

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
		except IOError as e:
			logging.error('Serial error: ' + str(e))
			time.sleep(120)
			ser = serial_connect(SERIAL_DEV, 115200)
			continue
		except SerialException as e:
			logging.error('Serial error: ' + str(e))
			time.sleep(60)
			ser = serial_connect(SERIAL_DEV, 115200)
			continue
		
		init_match = re.search(init_patt, curr)
		
		if init_match:
			logging.info(curr)
			continue
		if curr_date.day <> datetime.date.today().day:
			f_table.close()
			f_name = datetime.datetime.now().strftime('%Y-%m-%d.cvs')
			f_path = DATA_PATH + f_name
			f_table = open(f_path, 'a', 0)
			curr_date = datetime.date.today()
		
		ftp_thread = threading.Thread(target=upload_to_ftp, args=(FTP_SERV, FTP_NAME, FTP_PASS, f_name))
		
		date_str = datetime.datetime.now().strftime('%Y/%m/%d %H:%M')
		write_lock.acquire()
		f_table.write(date_str+','+curr)
		write_lock.release()
		#upload_to_ftp(FTP_SERV, FTP_NAME, FTP_PASS, f_name)
		ftp_thread.start()
	
	sys.exit(0)

def serial_connect(dev, speed):
	try_count = 1

	while try_count <= CONNECT_TRYOUT:
		try:
			ser = serial.Serial(dev, speed)
		except SerialException as e:
			logging.error('Serial Connect Error: ' + str(e))
			print 'Connection Error'
			time.sleep(120)
		else:
			logging.info('Connected after ' + str(try_count) + ' tryouts')
			return ser
		finally:
			try_count += 1
	
	logging.critical('FAIL to connect. Exit.')
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
			raise ('unknown error')
		ftp_serv.close()
	except (socket.error, socket.gaierror) as e:
		logging.error('FTP Socket Error: ' + str(e))
	except ftplib.error_temp as e:
		logging.error('FTP Temporary Error: ' + str(e))
	except ftplib.error_perm as e:
		logging.error('FTP Permanent Error: ' + str(e))
	except ftplib.error_reply as e:
		logging.error('FTP Unexpected Error: ' + str(e))
	except uplFail as e:
		logging.error('FTP Upload Failed: ' + str(e))
	else:
		logging.info('FTP Upload Success')
	finally:
		write_lock.release()
	
	upl_file.close()

class uplFail(Exception):
	def __init__(self, value):
		self.value = value
	def __str__(self):
		return repr(self.value)

def ctrl_c_handler(signum, frame):
	print 'Exit.'
	logging.info('Exit.')
	sys.exit(0)

if __name__ == '__main__':
	main()
