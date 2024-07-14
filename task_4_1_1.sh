#!/bin/sh
yes | cp -a instance/engineer/station/plugin_config.cfg.task_4_1_1 instance/engineer/station/plugin_config.cfg 
yes | cp instance/engineer/station/aspect_policy.cfg.task_4_1_1 instance/engineer/station/aspect_policy.cfg 


pkill -9 envset_proc

sh run_cube.sh exec_def/_center.def &
sleep 1
sh run_cube.sh exec_def/_plc_loader.def &
sleep 1
sh run_cube.sh exec_def/_login_hacker.def &
sleep 1
sh run_cube.sh exec_def/_engineer_station.def &
sleep 5
pkill -9 envset_proc
cp -f instance/engineer/station/Pub.key instance/plc/plc_loader
