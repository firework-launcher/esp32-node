import socket
import json
import sys

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
recv_s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
recv_s.connect((sys.argv[1], 4444))
s.connect((sys.argv[1], 3333))
s.send(json.dumps({'code': 5}).encode())
s.close()
recv_s.close()
