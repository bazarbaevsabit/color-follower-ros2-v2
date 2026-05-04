#!/bin/bash

xhost +local:docker > /dev/null 2>&1

docker run -it --rm \
    --net host \
    --ipc host \
    -e DISPLAY=$DISPLAY \
    -e XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR \
    -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
    -v /home/user/color-follower-ros2-v2/src:/home/work \
    --device /dev/dri:/dev/dri \
    --name gazebo_test \
    ros2_full:v1 \
    /bin/bash -c "
            cd /home/work/
            exec /bin/bash
        "

xhost -local:docker > /dev/null 2>&1
