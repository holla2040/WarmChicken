#!/bin/bash
if [ -z "$NODENAME" ] 
then
    NODENAME=192.168.0.41
fi


cd data
for file in `ls -A1` 
do 
    echo ${file} to ${NODENAME}
    curl -F "file=@$PWD/${file}" ${NODENAME}/edit
done
