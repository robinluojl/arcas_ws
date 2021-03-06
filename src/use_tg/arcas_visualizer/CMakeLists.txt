cmake_minimum_required(VERSION 2.8.3)
project(arcas_visualizer)

find_package(catkin REQUIRED COMPONENTS functions roscpp std_msgs geometry_msgs arcas_msgs arcas_actions_msgs tg actionlib)

## System dependencies are found with CMake's conventions
find_package(Qt4 REQUIRED QtCore QtGui)
include(${QT_USE_FILE})
SET ( QWT_INCLUDE /usr/include/qwt/)

catkin_package(
 INCLUDE_DIRS ${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${QWT_INCLUDE}
 LIBRARIES arcas_visualizer
 CATKIN_DEPENDS functions roscpp std_msgs geometry_msgs arcas_msgs arcas_actions_msgs tg actionlib
 DEPENDS Qt4
)

 SET( ARCAS_UIS
       ./ui/arcas_coordination_center.ui
       ./ui/about_arcas_visualizer.ui
  )

include_directories(${catkin_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR} ${QWT_INCLUDE})

QT4_WRAP_UI( GUI_HDRS ${ARCAS_UIS} )
QT4_WRAP_CPP( ARCAS_MOC_SRCS src/ArcasVisualizer.h src/Comms.h) # NOTE: Beware on including the files here into the add executable. If not: vtable errors
SET (ARCAS_SRCS src/ArcasVisualizer.cpp src/Comms.cpp)

add_executable(arcas_visualizer src/arcas_visualizer.cpp ${GUI_HDRS} ${ARCAS_SRCS} ${ARCAS_MOC_SRCS})
target_link_libraries(arcas_visualizer ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} ${catkin_LIBRARIES})
