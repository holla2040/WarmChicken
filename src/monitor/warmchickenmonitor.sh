#!/bin/bash

case "$1" in
    open    ) 
        dooropen=`curl --connect-timeout 5 -s http://192.168.0.10/ | sed "s/^.*switchDoorOpen: //g" | sed "s/<.*//g" | sed "s/^ *//g"`
        if [ -n "$dooropen" ]
        then
            if [ "$dooropen" -ne "1" ]
            then
                echo "wc door not open" | /usr/bin/mail -s "wc door not open" holla2040@gmail.com
                echo "door not open"
            fi
        else 
            msg="wc no connection"
            echo "$msg" | /usr/bin/mail -s "$msg" holla2040@gmail.com
        fi
        ;;
    closed  ) 
        doorclosed=`curl -s http://192.168.0.10/ | sed "s/^.*switchDoorClosed: //g" | sed "s/<.*//g" | sed "s/^ *//g"`
        if [ -n "$doorclosed" ]
        then
            if [ "$doorclosed" -ne "1" ]
            then
                echo "wc door not closed" | /usr/bin/mail -s "wc door not closed" holla2040@gmail.com
                echo "door not closed"
            fi
        else 
            msg="wc no connection"
            echo "$msg" | /usr/bin/mail -s "$msg" holla2040@gmail.com
        fi
        ;;
    *       ) 
        echo usage: warmchickenmonitor.sh open\|closed
        ;;
esac

#if [ "$dooropen" -eq 0 ]
#then
#    echo "${t} F - hot" | /usr/bin/mail -s "greenhouse: ${t} F - hot" holla2040@gmail.com
#    echo "${t} F - email sent"
#fi
