#!/usr/bin/env python
import serial, time, sys, termios, os, traceback
import time, re, struct, socket
import socket
import BaseHTTPServer, SimpleHTTPServer, cgi

PORT=8000
WARMCHICKENHOST="192.168.0.110"
WARMCHICKENPORT=2000

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.settimeout(3)

class ReqHandler (SimpleHTTPServer.SimpleHTTPRequestHandler) :
    def do_GET(self) :
        self.send_response(200)
        self.send_header("content-type","text/html")
        self.end_headers()
        self.wfile.write("<html><body>"+status+"</body></html>")
#     def do_POST(self):
#        len = int(self.headers['content-length'])
#        body = self.rfile.read(len)
#        #params = dict(cgi.parse_qsl(body))
#        print body
#        #print params
#        self.send_response(200)
#        self.end_headers()

http=BaseHTTPServer.HTTPServer(('',PORT),ReqHandler)
print 'port=%d'%PORT
http.socket.settimeout(0.01)

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

def sendTime():
    global sock
    t = "T%d"%(int(time.time()) + 60*60*-7)
    sock.send(t)

def processkeyboardchar(c):
    global running, sock
    if c == 'q':
        running = 0
    if c == 'p':
        sock.close()
        os.system("make tcp")
        running = 0
    if c == 'T':
        sendTime()
    if c:
        sock.send(c)

sendTime()
sock.send('s')

try:
    status = ""
    temp = ""
    while running == 1:
        c = getchar() # keyboard
        if c:
            processkeyboardchar(c)
        try:
            c = sock.recv(1)
            if len(c):
                #sys.stdout.write(c)
                #sys.stdout.flush()
                if c == '\n':
                    status = temp.strip()
                    temp = ""
                    print status
                else:
                    temp += c
        except socket.timeout:
            pass
        http.handle_request()

except:
    traceback.print_exc(file=sys.stdout)


termios.tcsetattr(fd, termios.TCSAFLUSH, old)

