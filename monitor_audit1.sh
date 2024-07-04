#!/bin/sh

pkill -9 envset_proc

#sleep 1

#sh run_cube.sh exec_def/_center.def &
#sleep 1
#sh run_cube.sh exec_def/_plc_device.def &
#sleep 1
#sh run_cube.sh exec_def/_operator_station.def &
sleep 1
sh run_cube.sh exec_def/_monitor_term.def &
sleep 1
sh run_cube.sh exec_def/_monitor_user.def login.msg plc_audit1.msg  
sleep 1

