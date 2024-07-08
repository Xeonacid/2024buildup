#!/bin/bash
export CUBE_PATH=/root/centoscloud/cube-1.3
export CUBEAPPPATH=$(pwd) 
export LD_LIBRARY_PATH=$CUBE_PATH/cubelib/lib/:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$CUBEAPPPATH/locallib/bin/:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/root/cube-shell/plugin/:$LD_LIBRARY_PATH
$CUBE_PATH/proc/main/envset_proc $1 $2 $3 $4 $5 $6 $7
