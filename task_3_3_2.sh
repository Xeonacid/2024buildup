#!/bin/sh
yes | cp -a instance/operator/station/plugin_config.cfg.task_3_3_2 instance/operator/station/plugin_config.cfg 
yes | cp instance/operator/station/aspect_policy.cfg.task_3_3_2 instance/operator/station/aspect_policy.cfg 

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


# 启动管理终端
sh run_cube.sh exec_def/_monitor_term.def &

sleep 1
sh run_cube.sh exec_def/_monitor_user.def plc_set_t.msg plc_read_gear.msg ctrl_exit.msg

sleep 1
sh run_cube.sh exec_def/_operator_user.def plc_set_t.msg ctrl_exit.msg

sleep 1
sh run_cube.sh exec_def/_monitor_user.def plc_read_gear.msg ctrl_exit.msg

sleep 1
sh run_cube.sh exec_def/_operator_user.def plc_read_set.msg ctrl_exit.msg

sleep 5
pkill -9 envset_proc
