#!/bin/sh

pkill -9 envset_proc

rm -f instance/monitor/center/logic_code/*
sleep 1

sh run_cube.sh exec_def/_center.def &
sleep 1
sh run_cube.sh exec_def/_engineer_station.def &
sleep 1
sh run_cube.sh exec_def/_engineer_user.def login.msg code_upload.msg ctrl_exit.msg
sleep 1
