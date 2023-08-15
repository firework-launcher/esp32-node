import socket
import json

mappings = [4, 3, 2, 1, 8, 7, 6, 5, 12, 11, 10, 9, 16, 15, 14, 13]

rx_connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
tx_connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

ip_address = input('IP Address: ')

rx_connection.connect((ip_address, 4444))
tx_connection.connect((ip_address, 3333))
print('Connected to ESP32')
while True:
    try:
        recv = json.loads(rx_connection.recv(1024).decode('utf-8'))
    except json.decoder.JSONDecodeError:
        print('JSON Failed to parse')
    else:
        inputs = recv['inputData']
        if not inputs == []:
            data1 = '{0:b}'.format(inputs[0])
            zeros_to_add = 8-len(data1)
            zero_string = ''
            for zero in range(zeros_to_add):
                zero_string += '0'
            data1 = zero_string + data1

            data2 = '{0:b}'.format(inputs[1])
            zeros_to_add = 8-len(data2)
            zero_string = ''
            for zero in range(zeros_to_add):
                zero_string += '0'
            data2 = zero_string + data2

            data = data1 + data2
            new_data = list('0000000000000000')
            x = 0
            for mapping in mappings:
                mapping -= 1
                new_data[mapping] = data[x]
                x += 1
            data = ''.join(new_data)
            print(data)
