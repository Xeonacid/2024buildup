#!/bin/sh
yes | cp -a instance/operator/station/plugin_config.cfg.task_4_2_1 instance/operator/station/plugin_config.cfg 
yes | cp instance/operator/station/aspect_policy.cfg.task_4_2_1 instance/operator/station/aspect_policy.cfg 

yes | cp -a instance/plc/device/plugin_config.cfg.task_4_2_2 instance/plc/device/plugin_config.cfg 
yes | cp instance/plc/device/aspect_policy.cfg.task_4_2_2 instance/plc/device/aspect_policy.cfg 
pkill -9 envset_proc

sleep 1

sh run_cube.sh exec_def/_center.def &
sleep 1
sh run_cube.sh exec_def/_plc_device.def &
sleep 3
sh run_cube.sh exec_def/_modbus_hacker.def &
sleep 3
sh run_cube.sh exec_def/_operator_station.def &
sleep 3
sh run_cube.sh exec_def/_operator_user.def operator_login.msg plc_start.msg plc_read_t.msg plc_set_t.msg ctrl_exit.msg
sleep 5 
sh run_cube.sh exec_def/_operator_user.def plc_read_t.msg plc_read_gear.msg ctrl_exit.msg
sleep 5
sh run_cube.sh exec_def/_operator_user.def plc_read_t.msg plc_read_gear.msg ctrl_exit.msg
sleep 5
sh run_cube.sh exec_def/_operator_user.def plc_read_t.msg plc_read_gear.msg ctrl_exit.msg
sleep 5
sh run_cube.sh exec_def/_operator_user.def plc_read_t.msg plc_read_gear.msg ctrl_exit.msg
sleep 5
pkill -9 envset_proc
