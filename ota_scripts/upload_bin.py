import requests
import pathlib
import sys

ip_address = sys.argv[1]
bin_directory = sys.argv[2]

f = open(bin_directory, 'rb')

requests.post('http://{}/update'.format(ip_address), data=f.read())

f.close()
