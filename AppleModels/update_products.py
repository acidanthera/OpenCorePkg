#!/usr/bin/env python

"""
Check and store product information

Relevant links:
http://support-sp.apple.com/sp/index?page=cpuspec&cc=XXXX
http://support-sp.apple.com/sp/product?cc=XXXX

Interesting examples:
-  01P - three character
- J094 - missing name
- J6FL - unicode
- CURL - easter egg (returns 403)
- MSDB - easter egg (returns 403)
- TFTP - easter egg (returns 403)
- WGET - easter egg (returns 403)
"""

import argparse
import datetime
import json
import os
import random
import signal
import sys
import time
import xml.etree.ElementTree
import yaml

try:
  from urllib.request import build_opener
except ImportError:
  from urllib2 import build_opener

KEY_NAME         = 'n'
KEY_EXCEPT       = 'e'
KEY_STATUS       = 's'
KEY_DATE         = 'd'

STATUS_OK        = 1
STATUS_PENDING   = 2
STATUS_EXCEPT    = 3
STATUS_NOT_FOUND = 4

ADD_DUMMY  = 0
ADD_EXCEPT = 1
ADD_NEW    = 2
ADD_SKIP   = -1

apple_base34 = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',
               'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K', 'L', 'M', 'N', 'P',
               'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z']

def current_date():
  return time.mktime(datetime.datetime.now().date().timetuple())

def map_legacy_status(status):
  if status == 'ok':
    return STATUS_OK
  if status == 'pending':
    return STATUS_PENDING
  if status == 'except':
    return STATUS_EXCEPT
  if status == 'not found':
    return STATUS_NOT_FOUND
  raise RuntimeError('Invalid legacy status {}'.format(status))

def base34_to_num(str):
    num = 0
    for i in str:
      num *= 34
      num += apple_base34.index(i)
    return num

def num_to_base34(num):
    str = ''
    while num > 0:
      num, r = divmod(num, 34)
      str    = apple_base34[r] + str
    return str.zfill(3)

def load_products(path='Products.json'):
  try:
    with open(path, 'r') as fh:
      if path.endswith('.json'):
        db = json.load(fh)
      else:
        db = yaml.safe_load(fh)
        for entry in db:
          db[entry]['date'] = time.mktime(db[entry]['date'].timetuple())
      if len(db) > 0 and db[next(iter(db))].get('date', None) is not None:
        newdb = {}
        for entry in db:
          newdb[entry] = {
            KEY_NAME:   db[entry]['name'] if db[entry]['name'] is not None else 0,
            KEY_EXCEPT: db[entry]['except'] if db[entry]['except'] is not None else 0,
            KEY_STATUS: map_legacy_status(db[entry]['status']),
            KEY_DATE:   db[entry]['date'],
          }
        return newdb
      return db
  except IOError:
    return {}
  except Exception as e:
    print("Failed to parse file {} - {}".format(path, str(e)))
    sys.exit(1)

def save_database(database, path):
  # We are not using yaml for speed reasons.
  with open(path, 'w') as fh:
    json.dump(database, fh, indent=0, separators=(',', ':'), sort_keys=True)

def store_product(database, model, name, exception, status, date = None):
  database[model] = {
    KEY_NAME: name if name is not None else 0,
    KEY_EXCEPT: exception if exception is not None else 0,
    KEY_STATUS: status,
    KEY_DATE: current_date() if date is None else date
  }

def update_product(database, model, database_path, force = False, retention = 90):
  prev = database.get(model, None)

  if prev is not None:
    if force is False and prev[KEY_STATUS] == STATUS_OK:
      print(u'{} - {} (skip)'.format(model, prev[KEY_NAME]))
      return ADD_SKIP

    curr   = current_date()
    expire = prev[KEY_DATE] + retention*24*3600
    if expire > curr and prev[KEY_STATUS] == STATUS_NOT_FOUND:
      print(u'{} - not found (skip {})'.format(model, datetime.date.fromtimestamp(prev[KEY_DATE])))
      return ADD_SKIP

    expire = prev[KEY_DATE] + retention*24*3600 / 2
    if expire > curr and prev[KEY_STATUS] == STATUS_PENDING:
      print(u'{} - pending (skip {})'.format(model, datetime.date.fromtimestamp(prev[KEY_DATE])))
      return ADD_SKIP

  try:
    url    = 'http://support-sp.apple.com/sp/product?cc={}'.format(model)
    opener = build_opener()
    mm     = random.choice(range(11, 16))
    ff     = random.choice(range(50, 70))
    agent  = 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10.{}; rv:{}) Gecko/20100101 Firefox/{}'.format(mm, ff, ff)
    opener.addheaders = [
      ('Host', 'support-sp.apple.com'),
      ('User-Agent', agent),
      ('Accept', 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8'),
      ('Accept-Language', 'en-US,en;q=0.5'),
      ('Accept-Encoding', 'gzip, deflate, br'),
      ('DNT', '1'),
      ('Connection', 'keep-alive'),
      ('Upgrade-Insecure-Requests', '1'),
      ('Cache-Control', 'max-age=0'),
    ]
    response = opener.open(url)
    root     = xml.etree.ElementTree.fromstring(response.read())
  except Exception as e:
    print(u'{} - except ({})'.format(model, str(e)))
    store_product(database, model, None, str(e), STATUS_EXCEPT)
    time.sleep(1)
    return ADD_EXCEPT

  if root.find('error') is None and root.find('configCode') is not None:
    name   = root.find('configCode').text
    status = STATUS_OK if name is not None else STATUS_PENDING
    print(u'{} - {}'.format(model, name))
    store_product(database, model, name, None, status)
    save_database(database, database_path) # Always store valid product
    return ADD_NEW
  else:
    print(u'{} - not found'.format(model))
    store_product(database, model, None, None, STATUS_NOT_FOUND)
    return ADD_DUMMY

def merge_products(database, database_path, filename):
  print('Merging {}'.format(filename))

  if not os.path.exists(filename):
    print(u'File {} is missing'.format(filename))
    sys.exit(1)

  new_database = load_products(filename)

  for model in new_database:
    new = new_database[model]
    old = database.get(model, None)
    store = False
    if old is None:
      store = True
    elif old != new:
      if old[KEY_STATUS] == new[KEY_STATUS] and old[KEY_NAME] == new[KEY_NAME] and old[KEY_EXCEPT] == new[KEY_EXCEPT]:
        store = new[KEY_DATE] > old[KEY_DATE]  # Update date if newer
      elif old[KEY_STATUS] != STATUS_OK and new[KEY_DATE] > old[KEY_DATE]:
        store = True # Update new status
      else:
        print(u'Skipping {} entry for {}'.format(str(old), str(new)))

    if store:
      store_product(database, model, new[KEY_NAME],
        new[KEY_EXCEPT], new[KEY_STATUS], new[KEY_DATE])

  save_database(database, database_path)

def update_products(database, start_from, end_with, database_path, force = False, retention = 45, savenum = 2048):
  start     = base34_to_num(start_from)
  end       = base34_to_num(end_with)
  countdown = savenum
  while start <= end:
    new    = update_product(database, num_to_base34(start), database_path, force, retention)
    start += 1
    if new == ADD_NEW:
      countdown  = savenum
    elif new == ADD_DUMMY or new == ADD_EXCEPT:
      if countdown == 0:
        save_database(database, database_path)
        countdown  = savenum
      else:
        countdown -= 1

  save_database(database, database_path)

def main():
  parser = argparse.ArgumentParser(description='Update product database')
  parser.add_argument('start', default='000', nargs='?', help='Starting product ID')
  parser.add_argument('end', default='ZZZZ', nargs='?', help='Ending product ID')
  parser.add_argument('--force', action='store_true', help='Recheck all products')
  parser.add_argument('--retention', type=int, default=90, help='Check products older than N days')
  parser.add_argument('--savenum', type=int, default=2048, help='Save every N products while invalid')
  parser.add_argument('--merge', type=str, default=None, help='Merge specified database DB into main')
  parser.add_argument('--database', type=str, default='Products.json', help='Use specified database file')

  args = parser.parse_args()

  db = load_products(args.database)
  
  if args.merge is not None:
    return merge_products(db, args.database, args.merge)

  for id in [args.start, args.end]:
    if len(id) < 3 or len(id) > 4:
      print(u'Invalid length for ID {}'.format(id))
      sys.exit(1)

    if not set(id) < set(apple_base34):
      print(u'Invalid characters in ID {}'.format(id))
      sys.exit(1)

  def abort_save_database(sig, frame):
    print('Aborting with database save on SIGINT!')
    save_database(db, args.database)
    sys.exit(0)

  signal.signal(signal.SIGINT, abort_save_database)

  return update_products(db, args.start, args.end, args.database, args.force, args.retention, args.savenum)

if __name__ == '__main__':
  main()
