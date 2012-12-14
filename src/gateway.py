#!/usr/bin/env python
import serial, time, sys, termios, os, traceback
import time, re, struct, socket
import socket
import BaseHTTPServer, SimpleHTTPServer, cgi

PORT=1234
WARMCHICKENHOST="192.168.0.110"
WARMCHICKENPORT=2000

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.settimeout(3)

def processkeyboardchar(c):
    global running, sock
    print "sending",c
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

class ReqHandler (SimpleHTTPServer.SimpleHTTPRequestHandler) :
    def do_GET(self) :
        #print 77777,self.path
        if self.path.count('favicon'):
            return SimpleHTTPServer.SimpleHTTPRequestHandler.do_GET(self)
        self.send_response(200)
        self.send_header("content-type","text/html")
        self.end_headers()
        self.wfile.write("<html>")
        self.wfile.write('<head>')
        self.wfile.write('<meta http-equiv="refresh" content="5">')
        self.wfile.write('<META HTTP-EQUIV="Pragma" CONTENT="no-cache"><META HTTP-EQUIV="Expires" CONTENT="-1">')
        self.wfile.write("<style>td { margin-left:auto; margin-right:auto;border: 1px;padding-right:10px; } input {width:100px}</style>")
        self.wfile.write('</head>')
        self.wfile.write("<b>WarmChicken</b>")
        self.wfile.write("<table>")
        d = status.split('|')
        self.wfile.write("<tr><td>Time of Day</td><td>"+d[0]+"</td></tr>")
        self.wfile.write("<tr><td>Temp Outside</td><td>"+d[1].split('.')[0]+"</td></tr>")
        self.wfile.write("<tr><td>Temp Inside</td><td>"+d[2].split('.')[0]+"</td></tr>")
        self.wfile.write("<tr><td>Door Status</td><td>"+d[4]+"</td></tr>")

        v = str(int(100*(float(d[3])/1000.0)))
        self.wfile.write("<tr><td>Light Level Outdoor</td><td>"+v+"</td></tr>")
        self.wfile.write("<tr><td>Light Level Indoor</td><td>"+d[5]+"</td></tr>")
        #self.wfile.write("<tr><td>Outdoor Light Status</td><td>"+d[5]+"</td></tr>")

        self.wfile.write("</table>")
        self.wfile.write("<table>")

        self.wfile.write('<tr><td><form action="/action" method="POST"><input type="hidden" value="L" name="key"/><input type="submit" value="Light On"/></form></td>')
        self.wfile.write('<td><form action="/action" method="POST" ><input type="hidden" value="K" name="key"/><input type="submit" value="Light Off"/></form></td></tr>')
        self.wfile.write('<tr><td><form action="/action" method="POST" ><input type="hidden" value="O" name="key"/><input type="submit" value="Door Open"/></form></td>')
        self.wfile.write('<td><form action="/action" method="POST" ><input type="hidden" value="C" name="key"/><input type="submit" value="Door Close"/></form></td></tr>')
        self.wfile.write("</table>")
        self.wfile.write("</body></html>")

    def do_POST(self):
        len = int(self.headers['content-length'])
        body = self.rfile.read(len).strip()
        if body.count("key="):
            k = body.split('=')[1]
            processkeyboardchar(k)
            time.sleep(.1)
            processkeyboardchar('s')
        self.send_response(301)
        self.send_header("Location","/")
        self.end_headers()
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

