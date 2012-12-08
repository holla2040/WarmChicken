#!/usr/bin/env python
import serial, time, sys, termios, os, traceback
import time, re, struct, socket
import socket

WARMCHICKENHOST="192.168.0.110"
WARMCHICKENPORT=2000

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.settimeout(3)

def connect(sock):
    try:
        sock.connect((WARMCHICKENHOST,WARMCHICKENPORT))
    except socket.timeout:
        print "socket timeout to",WARMCHICKENHOST
        sys.exit(1) 
    except socket.error: 
        print "connection refused to",WARMCHICKENHOST
        sys.exit(1) 

connect(sock)
sock.settimeout(0.001)

def getchar():
    return os.read(fd,7)

fd = sys.stdin.fileno()

old = termios.tcgetattr(fd)
new = termios.tcgetattr(fd)
new[3] = new[3] & ~termios.ICANON & ~termios.ECHO
new[6] [termios.VMIN] = 0
new[6] [termios.VTIME] = 0

termios.tcsetattr(fd, termios.TCSANOW, new)
termios.tcsendbreak(fd,0)

line = ""

running = 1

def processkeyboardchar(c):
    global running, sock
    if c == 'q':
        running = 0
    if c == 'p':
        sock.close()
        os.system("make tcp")
        running = 0
    if c == 'T':
        t = "T%d"%(int(time.time()) + 60*60*-7)
        sock.send(t)
        #for l in t:
            #sock.send(l)
            #sys.stdout.write(l)
            #sys.stdout.flush()
            #time.sleep(0.5)
    if c:
        sock.send(c)

try:
    while running == 1:
        c = getchar() # keyboard
        if c:
            processkeyboardchar(c)
        try:
            c = sock.recv(1)
            if len(c):
                sys.stdout.write(c)
                sys.stdout.flush()
        except socket.timeout:
            pass
except:
    traceback.print_exc(file=sys.stdout)


termios.tcsetattr(fd, termios.TCSAFLUSH, old)
