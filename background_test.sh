#!/bin/sh

pkill -9 envset_proc

sleep 1

sh run_cube.sh exec_def/_center.def &
sleep 1
sh run_cube.sh exec_def/_engineer_station.def &
sleep 1
sh run_cube.sh exec_def/_engineer_user.def login.msg ctrl_exit.msg
sleep 1
sh run_cube.sh exec_def/_engineer_user.def code_upload.msg ctrl_exit.msg
sleep 1
sh run_cube.sh exec_def/_engineer_user.def bin_upload.msg ctrl_exit.msg
sleep 1
sh run_cube.sh exec_def/_engineer_user.def bin_upload.msg ctrl_exit.msg
sleep 1
sh run_cube.sh exec_def/_plc_device.def &
sleep 1
sh run_cube.sh exec_def/_operator_station.def &
sleep 1
sh run_cube.sh exec_def/_operator_user.def login.msg ctrl_exit.msg 
sleep 1
sh run_cube.sh exec_def/_operator_user.def plc_start.msg 
sleep 1
sh run_cube.sh exec_def/_operator_user.def plc_read_t.msg 
sleep 1
sh run_cube.sh exec_def/_operator_user.def plc_set_t.msg 
sleep 1
