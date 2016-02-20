#!/bin/bash

kill -9 `ps -ef|grep -E "collect.bin"|grep -v grep|awk '{print   $2} '`
echo "collect killed!" >> collect_kill.log
