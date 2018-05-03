import socket
import time

while True:
    s = socket.socket()
    s.connect(('127.0.0.1', 8888))
    print(s.recv(1024).decode())
    s.close()
    time.sleep(1)
