#!/bin/sh
yes | cp instance/engineer/station/aspect_policy.cfg.task_1_3 instance/engineer/station/aspect_policy.cfg 
yes | cp -a instance/login_hacker/plugin_config.cfg.task_1_2 instance/login_hacker/plugin_config.cfg 

pkill -9 envset_proc
sleep 1

sh run_cube.sh exec_def/_center.def &
sleep 1
sh run_cube.sh exec_def/_login_hacker.def &
sleep 1
sh run_cube.sh exec_def/_engineer_station.def &
sleep 1
#sh run_cube.sh exec_def/_engineer_user.def login.msg   ctrl_exit.msg
#sleep 1
#sh run_cube.sh exec_def/_engineer_user.def logout.msg  ctrl_exit.msg
#sleep 1
sh run_cube.sh exec_def/_engineer_user.def engineer_login.msg  logout.msg  ctrl_exit.msg
sleep 1
sh run_cube.sh exec_def/_engineer_user.def engineer_login_nopass.msg  ctrl_exit.msg
sleep 1



