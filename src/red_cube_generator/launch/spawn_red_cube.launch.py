import random
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import ExecuteProcess
from launch.substitutions import PathJoinSubstitution

def generate_launch_description():
    pkg_share = get_package_share_directory('red_cube_generator')
    sdf_path = PathJoinSubstitution([pkg_share, 'worlds', 'red_cube.sdf'])

    # Случайные координаты
    x = random.uniform(0.0, 10.0)
    y = random.uniform(-5.0, 5.0)
    z = 0.5

    # Спавн с помощью утилиты ros_gz_sim
    spawn_cube = ExecuteProcess(
        cmd=[
            'ros2', 'run', 'ros_gz_sim', 'create',
            '-world', 'car_world',        
            '-name', 'red_cube',
            '-file', sdf_path,
            '-x', str(x), '-y', str(y), '-z', str(z),
            '-R', '0', '-P', '0', '-Y', '0'
        ],
        name='spawn_red_cube',
        output='screen'
    )

    return LaunchDescription([spawn_cube])