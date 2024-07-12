#!/bin/sh

yes | cp -a instance/plc/plc_loader/plugin_config.cfg.task_4_1_2 instance/plc/plc_loader/plugin_config.cfg 
yes | cp instance/plc/plc_loader/aspect_policy.cfg.task_4_1_2 instance/plc/plc_loader/aspect_policy.cfg 

pkill -9 envset_proc

rm -f instance/plc/plc_loader/logic_bin/*

sh run_cube.sh exec_def/_center.def &
sleep 1
sh run_cube.sh exec_def/_plc_loader.def &
sleep 1
sh run_cube.sh exec_def/_login_hacker.def &
sleep 1
sh run_cube.sh exec_def/_engineer_station.def &
sleep 1
sh run_cube.sh exec_def/_engineer_user.def engineer_login.msg bin_upload.msg ctrl_exit.msg
sleep 1
