cmake_minimum_required(VERSION 2.8.3)
project(pqp)

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
find_package(catkin REQUIRED)

###################################
## catkin specific configuration ##
###################################
## The catkin_package macro generates cmake config files for your package
## Declare things to be passed to dependent projects
## INCLUDE_DIRS: uncomment this if you package contains header files
## LIBRARIES: libraries you create in this project that dependent projects also need
## CATKIN_DEPENDS: catkin_packages dependent projects also need
## DEPENDS: system dependencies of this project that dependent projects also need
catkin_package(
 INCLUDE_DIRS include
 LIBRARIES pqp
#  CATKIN_DEPENDS other_catkin_pkg
#  DEPENDS system_lib
)

###########
## Build ##
###########

## Specify additional locations of header files
## Your package locations should be listed before other locations
include_directories(include/pqp include)

## Declare a cpp library
add_library(${PROJECT_NAME}
  src/Build.cpp src/BV.cpp src/PQP.cpp src/TriDist.cpp
)

#############
## Install ##
#############

## Mark executables and/or libraries for installation
install(TARGETS pqp
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
)
