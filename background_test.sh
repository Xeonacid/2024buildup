#!/bin/sh

pkill -9 envset_proc

sleep 1
# 删除上一次的运行结果
rm -f instance/monitor/center/logic_code/*
rm -f instance/monitor/center/lib/PLC_ENGINEER-LOGIC_CODE.lib
rm -f instance/plc/plc_loader/logic_bin/*
rm -f plugin/libthermostat_logic.so

# 启动各服务程序
#sh run_cube.sh exec_def/_center.def &
sleep 1
sh run_cube.sh exec_def/_plc_loader.def &
sleep 1
sh run_cube.sh exec_def/_engineer_station.def &
sleep 1

# 执行工程师登录行为
sh run_cube.sh exec_def/_engineer_user.def engineer_login.msg ctrl_exit.msg
sleep 1

# 执行代码上传行为
sh run_cube.sh exec_def/_engineer_user.def code_upload.msg ctrl_exit.msg
sleep 2

# 执行二进制文件上传行为
#sh run_cube.sh exec_def/_engineer_user.def bin_upload.msg ctrl_exit.msg
sleep 2

# 启动plc设备模拟
#sh run_cube.sh exec_def/_plc_device.def &
sleep 1

# 启动操作员站
#sh run_cube.sh exec_def/_operator_station.def &
sleep 1

# 执行操作员登录行为
#sh run_cube.sh exec_def/_operator_user.def operator_login.msg ctrl_exit.msg 
sleep 1

# 操作员执行设备温度开关打开行为
#sh run_cube.sh exec_def/_operator_user.def plc_start.msg ctrl_exit.msg
sleep 1

# 操作员获取当前设置温度信息
#sh run_cube.sh exec_def/_operator_user.def plc_read_t.msg  ctrl_exit.msg
sleep 1

#  操作员主动设置设备温度
#sh run_cube.sh exec_def/_operator_user.def plc_set_t.msg  ctrl_exit.msg
sleep 1
