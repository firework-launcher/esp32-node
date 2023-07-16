"""
Simple program to send commands directly to ESP32.
"""

import argparse
import socket

parser = argparse.ArgumentParser()
parser.add_argument('address', help='IP Address of ESP32')

args = parser.parse_args()

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((args.address, 3333))

print('Connected to ESP32')

cmds = {}
while True:
    cmd = bytes([int(input('Code: '), 16)])

    values = []
    while True:
        values.append(input('Value: ').encode())
        new_value = input('Send another value? (y, n): ')
        while not new_value == 'y' and not new_value == 'n':
            new_value = input('Send another value? (y, n): ')
        if new_value == 'n':
            break

    cmds[cmd] = values

    new_cmd = input('Send another command? (y, n): ')
    while not new_cmd == 'y' and not new_cmd == 'n':
        new_cmd = input('Send another command? (y, n): ')
    if new_cmd == 'n':
        break

message = b''
print(cmds)
for cmd in cmds:
    message += cmd
    x = 0
    for value in cmds[cmd]:
        message += value
        if not x == len(cmds[cmd])-1:
            message += b'\x7f'
        x += 1
    message += b'\x80'
message += b'\n'

s.send(message)
s.close()

message_formatted = '{}'.format(message)[1:]

print('Sent message: {}, and closed connection.'.format(message_formatted))
