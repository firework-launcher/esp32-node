import requests
import sys

ip_address = sys.argv[1]

requests.get('http://{}/restart'.format(ip_address))
