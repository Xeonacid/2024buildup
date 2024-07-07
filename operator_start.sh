#!/bin/sh

pkill -9 envset_proc

sleep 1

sh run_cube.sh exec_def/_center.def &
sleep 1
sh run_cube.sh exec_def/_plc_device.def &
sleep 1
sh run_cube.sh exec_def/_operator_station.def &
sleep 1
sh run_cube.sh exec_def/_operator_user.def login.msg plc_start.msg plc_set_t.msg
sleep 1
