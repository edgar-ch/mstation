#!/usr/bin/env python
# -*- coding: utf-8 -*-

import serial
from serial import SerialException
import datetime
import re
import ftplib
import socket
import os
import sys
import time
import signal
import logging
from logging.handlers import WatchedFileHandler
import threading
import requests
from requests.exceptions import *
import ConfigParser
import json

LOG_FORMAT = '[%(asctime)s] %(levelname)s: %(funcName)s: %(message)s'
CONF_PATH = 'mstation_serv.cfg'
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

config = ConfigParser.SafeConfigParser()
config.read(CONF_PATH)

func_stat = {
    'FTP': True,
    'URL': True,
    'OpenWeather': False
}

all_settings = {
    'MAIN': {
        'log_path': '/var/log/mstation.log',
        'serial_dev': '/dev/ttyACM0',
        'serial_speed': 115200,
        'connect_tryout': 10,
        'data_path': '/usr/local/MStation/data/'
    },
    'FTP': {
        'ftp_serv': None,
        'ftp_passw': None,
        'ftp_name': None,
        'ftp_path': None
    },
    'URL': {
        'upload_url': None
    }
}

data_head = ['date', 'press', 'temp1', 'temp2', 'temp3', 'humid', 'lux']


def init_logger(log_path='mstation.log'):
    global log
    log = logging.getLogger('main_log')
    log.setLevel(logging.INFO)
    log_handler = WatchedFileHandler(log_path)
    log_fmt = logging.Formatter(fmt=LOG_FORMAT)
    log_handler.setFormatter(log_fmt)
    log.addHandler(log_handler)


def main():
    signal.signal(signal.SIGINT, ctrl_c_handler)

    if config.has_option('MAIN', 'log_path'):
        init_logger(config.get('MAIN', 'log_path'))
    else:
        init_logger()
    parse_conf()

    SERIAL_DEV = all_settings['MAIN']['serial_dev']
    SERIAL_SPEED = all_settings['MAIN']['serial_speed']
    CONNECT_TRYS = all_settings['MAIN']['connect_tryout']

    ser = serial_connect(SERIAL_DEV, SERIAL_SPEED)

    init_patt = re.compile(r'MStation')
    time_patt = re.compile(r'T{1,1}')
    meas_patt = re.compile(r'^MEAS:')

    while True:
        try:
            curr = ser.readline()
        except SerialException as e:
            log.error('Serial error: ' + str(e))
            time.sleep(120)
            ser = serial_connect(SERIAL_DEV, SERIAL_SPEED, CONNECT_TRYS)
            continue

        init_match = re.match(init_patt, curr)
        time_match = re.match(time_patt, curr)
        meas_match = re.match(meas_patt, curr)

        if init_match:
            log.info(curr)
            continue
        if time_match:
            try:
                ser.write(datetime.datetime.now().strftime('T%s'))
            except SerialTimeoutException as e:
                log.error('Fail to send time to serial' + str(e))
            else:
                log.info('Send time to serial success')
            continue
        if meas_match:
            curr = meas_patt.sub('', curr)
            process_meas_data(curr)
            continue
        log.info('Get from serial:' + str(curr))

    sys.exit(0)


def process_meas_data(meas_string):
    f_name = datetime.datetime.now().strftime('%Y-%m-%d.json')
    f_path = all_settings['MAIN']['data_path'] + f_name
    f_table = open(f_path, 'a', 0)

    data_dict = dict(zip(meas_string.split(','), data_head))
    final_str = json.dumps(
        data_dict,
        sort_keys=True
    )

    write_lock.acquire()
    f_table.write(final_str)
    f_table.close()
    write_lock.release()

    if func_stat['FTP']:
        ftp_thread = threading.Thread(
            target=upload_to_ftp,
            args=(
                all_settings['FTP']['ftp_serv'],
                all_settings['FTP']['ftp_name'],
                all_settings['FTP']['ftp_name'],
                f_path
            )
        )
        ftp_thread.start()
    if func_stat['URL']:
        upl_thread = threading.Thread(
            target=upload_to_site,
            args=(all_settings['URL']['upload_url'], data_dict)
        )
        upl_thread.start()
    if func_stat['OpenWeather']:
        openw_thread = threading.Thread(
            target=upload_to_openweathermap,
            args=[final_str]
        )
        openw_thread.start()


def serial_connect(dev, speed, tryouts=5):
    try_count = 1

    while try_count <= tryouts:
        try:
            ser = serial.Serial(dev, speed)
        except SerialException as e:
            log.error('Serial Connect Error: ' + str(e))
            print 'Connection Error'
            time.sleep(120)
        else:
            log.info('Connected after ' + str(try_count) + ' tryouts')
            ser.writeTimeout = 10
            return ser
        finally:
            try_count += 1

    log.critical('FAIL to connect. Exit.')
    sys.exit(-1)


def parse_conf(filename='mstation_serv.cfg'):
    if not config.has_section('MAIN'):
        log.error('Not have MAIN section in config')
        safe_quit()

    try:
        all_settings['MAIN']['serial_dev'] = config.get('MAIN', 'serial_dev')
        all_settings['MAIN']['serial_speed'] = config.getint(
            'MAIN',
            'serial_speed'
        )
        all_settings['MAIN']['connect_tryout'] = config.getint(
            'MAIN',
            'connect_tryout'
        )
        all_settings['MAIN']['data_path'] = config.get('MAIN', 'data_path')
    except ConfigParser.NoOptionError as e:
        log.error('Not found required option' + str(e))
        safe_quit()

    try:
        all_settings['FTP']['ftp_serv'] = config.get('FTP', 'ftp_serv')
        all_settings['FTP']['ftp_name'] = config.get('FTP', 'ftp_name')
        all_settings['FTP']['ftp_passw'] = config.get('FTP', 'ftp_passw')
        all_settings['FTP']['ftp_path'] = config.get('FTP', 'ftp_path')
    except ConfigParser.NoOptionError as e:
        log.error('Not found required option for FTP' + str(e))
        func_stat['FTP'] = False
    except ConfigParser.NoSectionError as e:
        log.error('Not found FTP settings' + str(e))
        func_stat['FTP'] = False
    else:
        func_stat['FTP'] = True

    try:
        all_settings['URL']['upload_url'] = config.get('URL', 'upload_url')
    except ConfigParser.NoOptionError as e:
        log.error('Not found required option for URL' + str(e))
        func_stat['URL'] = False
    except ConfigParser.NoSectionError as e:
        log.error('Not found settings for URL' + str(e))
        func_stat['URL'] = False
    else:
        func_stat['URL'] = True


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


def upload_to_site(url, data):
    if not upload_to_url(url, data):
        failed_upload.append(data)
    else:
        if upload_lock.acquire(False):
            while len(failed_upload) != 0:
                data_old = failed_upload[0]
                if upload_to_url(url, data_old):
                    failed_upload.pop(0)
            upload_lock.release()


def upload_to_url(url, data_req):
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
    data_post = {
        'temp': data_send[1],
        'humidity': data_send[3],
        'lat': LATITUDE,
        'long': LONGITUDE,
        'alt': ALTITUDE,
        'name': STATION_NAME
    }

    try:
        req = requests.post(
            URL_OPENW,
            auth=(USER_NAME, PASSW),
            data=data_post,
            timeout=60
        )
    except (ConnectionError, HTTPError, URLRequired, Timeout) as e:
        log.error(str(e))
    else:
        if (req.status_code == requests.codes.ok):
            log.info('Upload to OpenWeather success.')
        else:
            log.error(
                'Upload to OpenWeather failed with code '+str(req.status_code)
            )


def ctrl_c_handler(signum, frame):
    log.info('Get Ctrl-C.')
    safe_quit()


def safe_quit():
    print 'Exit.'
    log.info('Exit.')
    logging.shutdown()
    sys.exit(0)

if __name__ == '__main__':
    main()
