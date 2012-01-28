#!/usr/bin/env python

STATUS_URL = 'http://chromium-status.appspot.com/current?format=json'
DRIVER_DIR = '/sys/bus/usb/drivers/usb_crts'

import json
import os
import string
import urllib

def open_device():
    for root, dirs, files in os.walk(DRIVER_DIR):
        for name in dirs:
            if name[0] in string.digits:
                return open('%s/%s/crts' % (DRIVER_DIR, name), 'w')
    return None

def main():
    status = urllib.urlopen(STATUS_URL)
    json_status = json.loads(status.read())
    device = open_device()
    if 'open' == json_status['general_state']:
        device.write('05')  # green: on
    elif 'closed' == json_status['general_state']:
        device.write('14')  # red: on
    else:
        device.write('25')  # green: flash

if __name__ == "__main__":
    main()

