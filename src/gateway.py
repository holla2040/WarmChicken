#!/usr/bin/env python
import serial, time, sys, termios, os, traceback, threading, urllib
import time, re, struct, socket
import socket
import BaseHTTPServer, SimpleHTTPServer, cgi

PORT=1234
WARMCHICKENHOST="192.168.0.110"
WARMCHICKENPORT=2000

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.settimeout(.1)

running = 1

class ReqHandler (SimpleHTTPServer.SimpleHTTPRequestHandler) :
    def do_GET(self) :
        print 77777,self.path
        if self.path.count('favicon'):
            return SimpleHTTPServer.SimpleHTTPRequestHandler.do_GET(self)
        self.send_response(200)
        self.send_header("content-type","text/html")
        self.end_headers()
        self.wfile.write("<html>")
        self.wfile.write('<head>')
        self.wfile.write('<meta http-equiv="refresh" content="15">')
        self.wfile.write('<META HTTP-EQUIV="Pragma" CONTENT="no-cache"><META HTTP-EQUIV="Expires" CONTENT="-1">')
        #self.wfile.write("<style>body {width:100%} table,tr{font-size:180%; width:100%} td{border:1px solid black;} input{width:350px} td.l{border:1px solid red;} td.v{width:20%}</style>")
        self.wfile.write("<style>body {width:100%}  table,td {padding-right:5px;font-size:220%; overflow: hidden; display: inline-block; white-space: nowrap; }input{width:400px} </style>")
        self.wfile.write('</head>')
        self.wfile.write("<b>WarmChicken</b><br>")
        self.wfile.write("<table>")
        d = status.split('|')
        self.wfile.write("<tr><td class='l'>Time of Day</td><td class='v'>"+d[0]+"</td></tr>")
        self.wfile.write("<tr><td class='l'>Temp Outside</td><td class='v'>"+d[2].split('.')[0]+"&deg;F</td></tr>")
        self.wfile.write("<tr><td class='l'>Temp Inside</td><td class='v'>"+d[1].split('.')[0]+"&deg;F</td></tr>")
        self.wfile.write("<tr><td class='l'>Door Status</td><td class='v'>"+d[4]+"</td></tr>")

        v = str(int(100*(float(d[3])/1000.0)))
        self.wfile.write("<tr><td class='l'>Light Outside</td><td class='v'>"+v+"</td></tr>")
        self.wfile.write("<tr><td class='l'>Light Inside</td><td class='v'>"+d[5]+"</td></tr>")
        #self.wfile.write("<tr><td>Outdoor Light Status</td><td>"+d[5]+"</td></tr>")

        self.wfile.write("</table>")
        self.wfile.write("<table>")

        self.wfile.write('<tr><td><form action="/action" method="POST"><input type="hidden" value="L" name="key"/><input type="submit" value="Light On"/></form></td>')
        self.wfile.write('<td><form action="/action" method="POST" ><input type="hidden" value="K" name="key"/><input type="submit" value="Light Off"/></form></td></tr>')
        self.wfile.write('<tr><td><form action="/action" method="POST" ><input type="hidden" value="O" name="key"/><input type="submit" value="Door Open"/></form></td>')
        self.wfile.write('<td><form action="/action" method="POST" ><input type="hidden" value="C" name="key"/><input type="submit" value="Door Close"/></form></td></tr>')
        self.wfile.write('<tr><td><form action="/action" method="POST" ><input type="hidden" value="S" name="key"/><input type="submit" value="E-Stop"/></form></td></tr>')
        self.wfile.write("</table>")
        self.wfile.write("</body></html>")
        sys.stdout.write('1')
        sys.stdout.flush()
        self.wfile.flush()
        self.wfile.close()

    def do_POST(self):
        len = int(self.headers['content-length'])
        body = self.rfile.read(len).strip()
        if body.count("key="):
            k = body.split('=')[1]
            processkeyboardchar(k)
            time.sleep(.1)
            processkeyboardchar('s')
        self.send_response(200)
        self.send_header("content-type","text/html")
        self.end_headers()
        self.wfile.write("<html>")
        self.wfile.write('<head>')
        self.wfile.write('<meta http-equiv="refresh" content=".5;url=/">')
#        #params = dict(cgi.parse_qsl(body))
#        print body
#        #print params
#        self.send_response(200)
#        self.end_headers()

class httpthread(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.http=BaseHTTPServer.HTTPServer(('',PORT),ReqHandler)
        self.http.socket.settimeout(0.01)

    def run(self):
        global running
        print 'port=%d'%PORT
        while running:
            self.http.handle_request()

httpserver = httpthread()
httpserver.start()

def processkeyboardchar(c):
    global running, sock
    print "sending",c
    if c == 'q':
        running = 0
        urllib.urlopen("http://127.0.0.1:%d"%PORT).read()
        time.sleep(.1)
    if c == 'p':
        sock.close()
        os.system("make tcp")
        running = 0
    if c == 'T':
        sendTime()
    if c:
        sock.send(c)

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

def sendTime():
    global sock
    t = "T%d"%(int(time.time()) + 60*60*-7)
    sock.send(t)


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

except:
    traceback.print_exc(file=sys.stdout)


termios.tcsetattr(fd, termios.TCSAFLUSH, old)

