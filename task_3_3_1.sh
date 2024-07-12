#!/bin/sh
yes | cp -a instance/plc/device/plugin_config.cfg.task_3_3_1 instance/plc/device/plugin_config.cfg 
yes | cp instance/plc/device/aspect_policy.cfg.task_3_3_1 instance/plc/device/aspect_policy.cfg 

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
sh run_cube.sh exec_def/_operator_user.def plc_set_gear.msg plc_read_gear.msg ctrl_exit.msg
sleep 5
sh run_cube.sh exec_def/_operator_user.def plc_read_t.msg plc_read_gear.msg ctrl_exit.msg
sleep 5
sh run_cube.sh exec_def/_operator_user.def plc_read_t.msg plc_read_gear.msg ctrl_exit.msg
sleep 5
sh run_cube.sh exec_def/_operator_user.def plc_read_t.msg plc_read_gear.msg ctrl_exit.msg
sleep 5
pkill -9 envset_proc
