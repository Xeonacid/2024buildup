#!/bin/sh

pkill -9 envset_proc

sleep 1

sh run_cube.sh exec_def/_center.def &
sleep 1
sh run_cube.sh exec_def/_engineer_station.def &
sleep 1
sh run_cube.sh exec_def/_engineer_user.def login.msg ctrl_exit.msg
sleep 1
sh run_cube.sh exec_def/_monitor_term.def &
sleep 1
sh run_cube.sh exec_def/_monitor_user.def login.msg ctrl_exit.msg
