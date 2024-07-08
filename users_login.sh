#!/bin/sh

pkill -9 envset_proc

sleep 1

sh run_cube.sh exec_def/_center.def &
sleep 1
sh run_cube.sh exec_def/_login_hacker.def &
sleep 1
sh run_cube.sh exec_def/_engineer_station.def &
sleep 1
sh run_cube.sh exec_def/_engineer_user.def engineer_login.msg ctrl_exit.msg

sleep 1
sh run_cube.sh exec_def/_plc_device.def &
sleep 1
sh run_cube.sh exec_def/_modbus_hacker.def &
sleep 1
sh run_cube.sh exec_def/_operator_station.def &
sleep 1
sh run_cube.sh exec_def/_operator_user.def operator_login.msg ctrl_exit.msg
