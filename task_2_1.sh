#!/bin/sh

pkill -9 envset_proc

sleep 1
# 删除上一次的运行结果
rm -f instance/monitor/center/logic_code/*
rm -f instance/monitor/center/lib/PLC_ENGINEER-LOGIC_CODE.lib
rm -f instance/plc/plc_loader/logic_bin/*
rm -f plugin/libthermostat_logic.so

# 启动各服务程序
sh run_cube.sh exec_def/_center.def &
sleep 1
sh run_cube.sh exec_def/_plc_loader.def &
sleep 1
sh run_cube.sh exec_def/_login_hacker.def &
sleep 1
sh run_cube.sh exec_def/_engineer_station.def &
sleep 1

# 执行工程师登录行为
sh run_cube.sh exec_def/_engineer_user.def attack_login1.msg attack_cmd1.msg ctrl_exit.msg
sleep 5
sh run_cube.sh exec_def/_engineer_user.def attack_login2.msg attack_cmd2.msg ctrl_exit.msg
sleep 5

# 启动plc设备模拟
sh run_cube.sh exec_def/_plc_device.def &
sleep 2

sh run_cube.sh exec_def/_modbus_hacker.def &
sleep 2

# 启动操作员站
sh run_cube.sh exec_def/_operator_station.def &
sleep 2

# 执行操作员站恶意访问行为
sh run_cube.sh exec_def/_operator_user.def attack_login3.msg attack_cmd3.msg ctrl_exit.msg 
sleep 5
sh run_cube.sh exec_def/_operator_user.def attack_login4.msg attack_cmd4.msg ctrl_exit.msg 
sleep 5

# 启动管理终端
sh run_cube.sh exec_def/_monitor_term.def &
sleep 2

# 执行管理终端恶意访问行为
sh run_cube.sh exec_def/_monitor_user.def attack_login5.msg attack_cmd5.msg ctrl_exit.msg 
sleep 5
sh run_cube.sh exec_def/_monitor_user.def attack_login6.msg attack_cmd6.msg ctrl_exit.msg 
sleep 5
