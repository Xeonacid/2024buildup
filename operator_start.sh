#!/bin/sh

pkill -9 envset_proc

sleep 1

sh run_cube.sh exec_def/_center.def &
sleep 1
sh run_cube.sh exec_def/_plc_device.def &
sleep 1
sh run_cube.sh exec_def/_modbus_hacker.def &
sleep 1
sh run_cube.sh exec_def/_operator_station.def &
sleep 1
sh run_cube.sh exec_def/_operator_user.def login.msg plc_start.msg plc_read_t.msg plc_set_t.msg ctrl_exit.msg
sleep 5 
sh run_cube.sh exec_def/_operator_user.def plc_read_t.msg plc_read_gear.msg ctrl_exit.msg
sleep 5
sh run_cube.sh exec_def/_operator_user.def plc_read_t.msg plc_read_gear.msg ctrl_exit.msg
sleep 5
sh run_cube.sh exec_def/_operator_user.def plc_read_t.msg plc_read_gear.msg ctrl_exit.msg
sleep 5
sh run_cube.sh exec_def/_operator_user.def plc_read_t.msg plc_read_gear.msg ctrl_exit.msg
