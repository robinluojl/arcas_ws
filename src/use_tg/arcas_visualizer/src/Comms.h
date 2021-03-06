/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2014  sinosuke <email>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef COMMS_H
#define COMMS_H

#include "functions/Point3D.h"
#include "functions/FormattedTime.h"

// ROS includes
#include <ros/ros.h>

#include <arcas_msgs/QuadControlReferencesStamped.h>
#include <arcas_msgs/WayPointWithCruiseStamped.h>
#include <arcas_msgs/QuadStateEstimationWithCovarianceStamped.h>
#include <geometry_msgs/Twist.h>

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <actionlib/client/simple_action_client.h>
#include <arcas_actions_msgs/LandAction.h>
#include <arcas_actions_msgs/TakeOffAction.h>
#include <std_msgs/Bool.h>

typedef actionlib::SimpleActionClient<arcas_actions_msgs::LandAction> LandClient;
typedef actionlib::SimpleActionClient<arcas_actions_msgs::TakeOffAction> TakeoffClient;

class Comms
{
public:
  Comms(int argc, char **argv);
  
  virtual ~Comms();
  
  void startComms(std::vector<uint> n_uavs);
  
  void shutdownComms();
  
  void setTopics(std::string topic_prefix, std::string state_topic, std::string desired_velocity_topic,
		 std::string orca_velocity_topic, std::string state_file, std::string emergency_stop_file, std::string reference);
  
  void clearLogs();
  
  functions::Point3D getCurrentPos(unsigned int uav);
  functions::Point3D getCurrentVel(unsigned int uav);
  functions::Point3D getDesiredVel(unsigned int uav);
    
  std::vector<functions::Point3D> getCurrentPos();
  std::vector<functions::Point3D> getCurrentVel();
  std::vector<functions::Point3D> getDesiredVel();
  
  void land(unsigned int i);
  void takeoff(unsigned int i);
  void emergencyStop(unsigned int i);
  
// signals:
//   void systemStateChanged(std::vector<functions::Point3D> pos, std::vector<functions::Point3D> vel, std::vector<functions::Point3D> orca_vel);
  
private:
  double time_step;
  std::string state_topic, desired_velocity_topic, orca_velocity_topic, emergency_stop_topic, reference_topic;
  std::string topic_prefix;
  std::string state_file;
  std::vector<uint> uavs_;
  

  
  functions::FormattedTime init_log_time;
  
  //! @brief Gets the state data of an UAV
  void UALStateCallback(const arcas_msgs::QuadStateEstimationWithCovarianceStamped::ConstPtr& s, unsigned int uav_id);
  
  //! @brief Gets the desired velocity of an UAV
  void DesiredVelocityCallback(const geometry_msgs::Twist::ConstPtr& s, unsigned int uav_id);
  
  //! @brief Gets the ORCA velocity of an UAV
  void ORCAVelocityCallback(const geometry_msgs::Twist::ConstPtr& s, unsigned int uav_id);
  
  // ROS stuff
  ros::AsyncSpinner *spinner;
  
  // ROS subscribers
  std::vector<ros::Subscriber> state_subscribers;
  std::vector<ros::Subscriber> desired_vel_subscribers, orca_vel_subscribers;
  
  // ROS publishers
  std::vector<ros::Publisher> emergency_publishers;
  std::vector<ros::Publisher> reference_publishers; // Necessary for safe takeoff
  
  // Actionlib
  std::vector<LandClient*> land_actions;
  std::vector<TakeoffClient*> take_actions;
  
  // Mutex
  boost::mutex orca_mutex, pos_mutex, vel_mutex, desired_mutex;
  
  // Log data
  std::vector<arcas_msgs::QuadStateEstimationWithCovariance> current_position;
  std::vector<functions::Point3D> current_pos;
  std::vector<functions::Point3D> current_vel;
  std::vector<functions::Point3D> orca_vel;
  std::vector<functions::Point3D> desired_vel;
  
};

#endif // COMMS_H
