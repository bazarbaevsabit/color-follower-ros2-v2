cmake_minimum_required(VERSION 3.8)
project(red_cube_follower)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(red_cube_follower_node src/red_cube_follower_node.cpp)
target_include_directories(red_cube_follower_node PUBLIC
  ${OpenCV_INCLUDE_DIRS}
)
ament_target_dependencies(red_cube_follower_node
  rclcpp
  sensor_msgs
  geometry_msgs
  cv_bridge
)
target_link_libraries(red_cube_follower_node
  ${OpenCV_LIBRARIES}
)

install(TARGETS red_cube_follower_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()