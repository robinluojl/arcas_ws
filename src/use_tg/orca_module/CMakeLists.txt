cmake_minimum_required(VERSION 2.8.3)
project(orca_module)

find_package(catkin REQUIRED COMPONENTS RVO2-3D RVO2-3D_unstable sparser functions dynamic_reconfigure roscpp std_msgs arcas_msgs geometry_msgs)

generate_dynamic_reconfigure_options(
  cfg/Active.cfg
)

catkin_package(
 INCLUDE_DIRS include
 LIBRARIES orca_module
 CATKIN_DEPENDS RVO2-3D RVO2-3D_unstable sparser functions
#  DEPENDS system_lib
)


set(LIBS sparser functions RVO2-3D RVO2-3D_unstable assimp)


include_directories(include ${catkin_INCLUDE_DIRS} cfg/cpp/)

add_library(${PROJECT_NAME} src/ORCAModule.cpp src/Maneuver.cpp)
add_dependencies(${PROJECT_NAME} arcas_msgs_generate_messages_cpp arcas_actions_msgs_generate_messages_cpp)
target_link_libraries(${PROJECT_NAME} ${LIBS} ${catkin_LIBRARIES})

add_executable(orca_launcher src/orca_launcher.cpp)
target_link_libraries(orca_launcher ${LIBS} ${PROJECT_NAME} ${catkin_LIBRARIES})
add_dependencies(orca_launcher ${PROJECT_NAME}_gencfg)
