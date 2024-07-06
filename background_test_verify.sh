#!/bin/sh

pkill -9 envset_proc

sleep 1
sh run_cube.sh exec_def/_verify_proxy.def &
sleep 1

rm -f instance/monitor/center/logic_code/*
rm -f instance/monitor/center/lib/PLC_ENGINEER-LOGIC_CODE.lib
sh run_cube.sh exec_def/_center.def &
sleep 1

rm -f instance/plc/plc_loader/logic_bin/*
sh run_cube.sh exec_def/_plc_loader.def &
sleep 1
sh run_cube.sh exec_def/_engineer_station.def &
sleep 1
sh run_cube.sh exec_def/_engineer_user.def login.msg ctrl_exit.msg
sleep 1
sh run_cube.sh exec_def/_engineer_user.def code_upload.msg ctrl_exit.msg
sleep 2
sh run_cube.sh exec_def/_engineer_user.def bin_upload.msg ctrl_exit.msg
sleep 2
sh run_cube.sh exec_def/_plc_device.def &
sleep 1
sh run_cube.sh exec_def/_operator_station.def &
sleep 1
sh run_cube.sh exec_def/_operator_user.def login.msg ctrl_exit.msg 
sleep 1
sh run_cube.sh exec_def/_operator_user.def plc_start.msg ctrl_exit.msg
sleep 1
sh run_cube.sh exec_def/_operator_user.def plc_read_t.msg  ctrl_exit.msg
sleep 1
sh run_cube.sh exec_def/_operator_user.def plc_set_t.msg  ctrl_exit.msg
sleep 1
