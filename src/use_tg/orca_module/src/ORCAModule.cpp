/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  sinosuke <email>

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


#include "orca_module/ORCAModule.h"
#include "RVO2-3D/RVOSimulator.h"
#include "functions/functions.h"
#include <ros/ros.h>
#include "functions/FormattedTime.h"
#include "functions/Point3D.h"
#include <iostream>
#include <arcas_msgs/QuadControlReferencesStamped.h>

using namespace std;
using namespace functions;

functions::Point3D fromRVO(const RVO::Vector3 &v) {
  functions::Point3D p(v.x(), v.y(), v.z());
  return p;
}

Checker *ORCAConfig::getChecker() const {
  Checker *ret = new Checker;
  ret->addProperty("timeStep", new NTimes(1));
  ret->addProperty("neighborDist", new NTimes(1));
  ret->addProperty("maxNeighbors", new NTimes(1));
  ret->addProperty("timeHorizon", new NTimes(1));
  ret->addProperty("radius", new NTimes(1));
  ret->addProperty("maxSpeed", new NTimes(1)); 
  
  return ret;
}
  
void ORCAConfig::fromBlock(ParseBlock& data) {
  try {
    timeStep = data("timeStep").as<float>();
    neighborDist = data("neighborDist").as<float>();
    maxNeighbors = data("maxNeighbors").as<size_t>();
    timeHorizon = data("timeHorizon").as<float>();
    radius = data("radius").as<float>();
    maxSpeed = data("maxSpeed").as<float>();
    if (data.hasProperty("obstacleDist")) {
      obstacleDist = data("obstacleDist").as<float>();
    } else {
      obstacleDist = neighborDist;
    }
    if (data.hasProperty("timeObstacle")) {
      timeObstacle = data("timeObstacle").as<float>();
    } else {
      timeObstacle = timeHorizon;
    }
    if (data.hasProperty("radius_z")) {
      radius_z = data("radius_z").as<double>();
    } else {
      radius_z = -1.0;
    }
    if (data.hasProperty("radius_obstacle")) {
      radius_obstacle = data("radius_obstacle").as<float>();
    } else {
      radius_obstacle = radius;
    }
    if (data.hasProperty("radius_obstacle_z")) {
      radius_obstacle_z = data("radius_obstacle_z").as<float>();
    } else {
      radius_obstacle_z = radius_z;
    }
    if (data.hasProperty("radius_warning")) {
      radius_warning = data("radius_warning").as<float>();
    } else {
      radius_warning = -1.0;
    }
    if (data.hasProperty("collision_multiplier")) {
      collision_multiplier = data("collision_multiplier").as<float>();
    } else {
      collision_multiplier = 5.0;
    }
    if (data.hasProperty("make_convex")) {
      make_convex = data("make_convex").as<bool>();
    } else {
      make_convex = false; // Make convex is still an experimental feature
    }
    if (data.hasProperty("a_max")) {
      a_max = data("a_max").as<float>();
    } else {
      a_max = -1.0; // Make convex is still an experimental feature
    }
    
    if (data.hasProperty("frozen_multiplier")) {
      frozen_mult = data("frozen_multiplier").as<float>();
    } else {
      frozen_mult = 0.02; // Make convex is still an experimental feature
    }
    if (data.hasProperty("z_multiplier")) {
      z_mult = data("z_multiplier").as<float>();
    } else {
      z_mult = 3.0; // Make convex is still an experimental feature
    }
    if (data.hasProperty("exponent")) {
      exponent = data("exponent").as<float>();
    } else {
      exponent = 1.0; // Make convex is still an experimental feature
    }
    if (data.hasProperty("exponent_z")) {
      exponent_z = data("exponent_z").as<float>();
    } else {
      exponent_z = 1.0; // Make convex is still an experimental feature
    }
    if (data.hasProperty("propagate_commands")) {
      propagate_commands = data("propagate_commands").as<bool>();
      
    } else {
      propagate_commands = false;
    }
    
  } catch (exception &e) {
    ROS_ERROR("ORCAConfig::fromBlock --> error while retrieving the configuration data. Content: %s", e.what());
  }
}

ORCAModule::ORCAModule(unsigned int id, string &filename):id(id),  loaded(false), emergency_stop(false) {
  ROS_INFO("Initializing ORCAModule...");
  fromBlock(filename);
  ROS_INFO("ORCAModule initialized. Parameters. id = %d. n_uavs = %d. State topic = \"%s\"", id, n_uavs, state_topic.c_str());
  if (!unstable) {
    sim = new RVO::RVOSimulator(conf.timeStep, conf.neighborDist, conf.maxNeighbors, conf.timeHorizon, conf.radius, conf.maxSpeed);
    RVO::Vector3 v(0.0, 0.0 ,0.0);
    for (uint i = 0; i < n_uavs; i++) {
      sim->addAgent(v);
    }
    sim_unst = NULL;
  } else {
    sim_unst = new RVO_UNSTABLE::RVOSimulator(conf.timeStep, conf.neighborDist, conf.maxNeighbors, conf.timeHorizon, conf.radius, conf.radius_obstacle, conf.maxSpeed, conf.timeObstacle);
    if (conf.radius_z > 0.0 && conf.radius_z != conf.radius) {
      sim_unst->setAgentDefaultRadiusZ(conf.radius_z);
    }
    if (conf.radius_obstacle_z > 0.0 && conf.radius_obstacle_z != conf.radius_obstacle) {
      sim_unst->setAgentDefaultRadiusObstacleZ(conf.radius_obstacle_z);
    }
    sim_unst->setAgentDefaultObstacleDist(conf.obstacleDist);
    sim_unst->setAgentDefaultWarningDist(conf.radius_warning);
    sim_unst->setAgentDefaultCollisionMultiplier(conf.collision_multiplier);
    sim_unst->setAgentDefaultZMultiplier(conf.z_mult);
    sim_unst->setPureDelay(pure_delay);
    sim_unst->setAgentDefaultFrozen(conf.frozen_mult);
    sim_unst ->setAgentDefaultExponent( conf.exponent, conf.exponent_z);
//     ROS_ERROR("Default Frozen: %f", conf.frozen_mult);
//     ROS_INFO("Setting radius warning: %f", conf.radius_warning);
    RVO_UNSTABLE::Vector3 v(0.0, 0.0 ,0.0);
    for (uint i = 0; i < n_uavs; i++) {
      sim_unst->addAgent(v);
    }
    addObstacles(scenario_file);
    sim = NULL;
  }
  for (uint i = 0; i < n_uavs; i++) {
    velocity_history.push_back(std::list<Point3D>());
  }
}

bool ORCAModule::addObstacles(const string& filename) {
  if (filename.size() > 0 && sim_unst != NULL && unstable) {
    return sim_unst->loadScenario(filename, conf.make_convex);
  }
  return false;
}

int ORCAModule::startROSComms() {
  ros::NodeHandle n;
  
  if (!loaded) {
    ROS_ERROR("ORCAModule error: --> Could not start the communications. The configuration file has not been successfully read.\n");
  }
  
  for (unsigned int i = 0; i < n_uavs; i++) {
    int num = i + 1;
    if (i < remap.size()) {
      num = remap.at(i);
    }
    // ARCAS type
    ostringstream os;
    
    if (i == id) {
      os << "/" << prefix << num << "/" << state_topic ;
    } else {
      os << "/" << prefix << num << "/" << state_topic_others;
    }
    ROS_INFO("%s\"%s\"", "Subscribing to arcas state. Topic: " , os.str().c_str() );
    boost::function<void (const arcas_msgs::QuadStateEstimationWithCovarianceStamped &)> f = boost::bind(&ORCAModule::receiveARCAS, this, i, _1);
    state_subscribers.push_back(n.subscribe<arcas_msgs::QuadStateEstimationWithCovarianceStamped&>(os.str(), 1, f)); 
    
    ostringstream v_pref_real_topic;
    v_pref_real_topic << "orca_" << num << "/" << v_pref_topic;
    ROS_INFO("%s\"%s\"", "Subscribing to preferred velocity topic: " , v_pref_real_topic.str().c_str());
    boost::function<void (const geometry_msgs::Twist &)> f_ = boost::bind(&ORCAModule::receivePreferredVelocity, this, i, _1);
    pref_vel_subscribers.push_back(n.subscribe<const geometry_msgs::Twist &>(v_pref_real_topic.str(), 1, f_));
    
    
    // Subscribe to emergency stop
    stringstream ss3;
    ss3 << "ual_" << id << "/emergency_stop";
    ROS_INFO("%s\"%s\"", "Subscribing to emergency stop topic: " , ss3.str().c_str());
    emergency_subscriber = n.subscribe<const std_msgs::Bool &>( ss3.str().c_str(), 0, &ORCAModule::receiveEmergency, this);
    
    pos.push_back(RVO::Vector3());
    pos_3d.push_back(Point3D());
    vel.push_back(RVO::Vector3());
      
    if (unstable) {
      pos_unst.push_back(RVO_UNSTABLE::Vector3());
      vel_unst.push_back(RVO_UNSTABLE::Vector3());
    }
    state_received.push_back(false);
  }
  
  end_ = false;
  started = false;
    
  ostringstream final_pub_topic;
  
  int n_ = id + 1;
  if (id < remap.size()) {
    n_ = remap.at(id);
  }
  final_pub_topic << "ual_" << n_ << "/" << publish_topic;
  ROS_INFO("%s\"%s\"", "Advertising topic: " , final_pub_topic.str().c_str());
  control_reference_pub = n.advertise<arcas_msgs::QuadControlReferencesStamped>(final_pub_topic.str(), 1);
  return 0;
}

void ORCAModule::mainLoop() {
  // Retrieve position and velocity from the quads
  
  double time_step;
  if (sim != NULL) {
    time_step = sim->getTimeStep();
  } else if (sim_unst != NULL) {
    time_step = sim_unst->getTimeStep();
  }
  
  // Init the distance logs and maneuvers
  min_separation_xy.clear();
  min_separation_z.clear();
  maneuver_log.clear();
  for (unsigned int i = 0; i < n_uavs; i++) {
    min_separation_xy.push_back(1e100);
    min_separation_z.push_back(1e100);
  }
  
  ros::Rate r(1.0 / time_step);
  while (ros::ok() && loaded) {
    started = true;
    for (uint i = 0; i < n_uavs /*&& started*/; i++) {
      if (state_received.at(i)) {
	if (!unstable) {
	  sim->setAgentPosition(i, pos.at(i));
	  sim->setAgentVelocity(i, vel.at(i));
	} else {
	  sim_unst->setAgentPosition(i, pos_unst.at(i));
	  sim_unst->setAgentVelocity(i, vel_unst.at(i));
	}
//      Discomment this if the preferred velocity is not transmitted
	if (i != id) {
	  if (!unstable) {
	    sim->setAgentPrefVelocity(i, vel.at(i));
	  } else {
	    if (!transmit_preferred) {
	      sim_unst->setAgentPrefVelocity(i, vel_unst.at(i));
	    }
	  }
	}
      } else {
	started = false;
      }
    }
    
    RVO::Vector3 v_publish(0.0, 0.0, 0.0);
    
    if (started && !emergency_stop) {
      if (dynamic_conf.active.size() > id && dynamic_conf.active.at(id) == '0') {
	// Bypass the preferred if ORCA is not active (for example when performing grasping maneuvers)
	if (unstable) {
	  RVO_UNSTABLE::Vector3 v = sim_unst->getAgentPrefVelocity(id);
	  v_publish[0] = v.x();
	  v_publish[1] = v.y();
	  v_publish[2] = v.z();
 	} else {
	  v_publish = sim->getAgentPrefVelocity(id);
	}
      } else if (!unstable) {
	functions::FormattedTime t;
	t.getTime();
	
	sim->doStep();
	functions::FormattedTime t2;
	t2.getTime();
	
	// Log the expended time
	if (time_file.size() > 0) {
	  ofstream tf;
	  tf.open(time_file.c_str(), std::fstream::out | std::fstream::app);
	  if (tf.is_open()) {
	    tf << t2 - t << " ";
	  }
	}
	
	v_publish = sim->getAgentVelocity(id);
      } else {
	functions::FormattedTime t;
	t.getTime();
	
	if (conf.propagate_commands) {
	  sim_unst->setPropagateCommands(true);
	  sim_unst->doStep();
	} else {
	  sim_unst->doStep(id);
	}
	
	// Publish the spended time if the time_file is not empty
	functions::FormattedTime t2;
	t2.getTime();
	if (time_file.size() > 0) {
	  ofstream tf;
	  tf.open(time_file.c_str(), std::fstream::out | std::fstream::app);
	  if (tf.is_open()) {
	    tf << t2 - t << " ";
	  }
	}
	
	
	
	// Filter the velocities
	filterVelocities();
	RVO_UNSTABLE::Vector3 v = sim_unst->getAgentVelocity(id);
	v_publish[0] = v.x();
	v_publish[1] = v.y();
	v_publish[2] = v.z();
      }
      // Publish the computed velocity
      
      
      
      publish_orca_vel(v_publish);
      
      if (conf.propagate_commands) {
	// Add the published velocity to the history
	for (unsigned int i = 0; i < sim_unst->getNumAgents(); i++) {
	  sim_unst->setLastCommand(i, sim_unst->getAgentVelocity(i));
	}
      }
      
      log();
    }
    
    ros::spinOnce();
    r.sleep();
  }
  
  saveLog(); // After the execution is completed --> save the summary of the results to file
}

void ORCAModule::filterVelocities()
{
  for (unsigned int i = 0; sim_unst != NULL && i < sim_unst->getNumAgents(); i++) {
    RVO_UNSTABLE::Vector3 v = sim_unst->getAgentVelocity(i);
    Point3D p(v[0], v[1], v[2]);
    if (last_vel.size() <= i) {
      last_vel.push_back(Point3D());
    }
    filterVelocity(p, i);
    v[0] = p.x;
    v[1] = p.y;
    v[2] = p.z;
    last_vel.at(i) = p;
    sim_unst->setAgentVelocity(i, v);
  }
}


void ORCAModule::filterVelocity(Point3D &p, unsigned int uav_id)
{
  // Truncate a_max
  float &time_step = conf.timeStep;
  
  if (last_vel.size() <= uav_id) {
    last_vel.push_back(Point3D());
  }
  
  if (p.distance(last_vel.at(uav_id)) > conf.a_max * time_step) {
    functions::Point3D des_vel(p);
    functions::Point3D dist = p - last_vel.at(uav_id);
    dist.normalize();
    p = last_vel.at(uav_id) + dist*conf.a_max * time_step;
  }
  
  // Low pass filter (mean)
  if (filter_size > 0) {
    if (last_velocity.size() <= uav_id) {
      last_velocity.push_back(velocity_history.at(uav_id).begin());
    }
    std::list<Point3D> &l = velocity_history.at(uav_id);
//     
//     
//     
    if (l.size() < filter_size) {
      l.push_back(p);
      last_velocity.at(uav_id) = l.begin();
    } else {
      *last_velocity.at(uav_id) = p;
      last_velocity.at(uav_id)++;
      if (last_velocity.at(uav_id) == l.end()) {
	last_velocity.at(uav_id) = l.begin();
      }
    }
    p = Point3D(0.0, 0.0, 0.0);
    int cont = 1;
    int sum = 0;
    list<Point3D>::iterator it = last_velocity.at(uav_id);
    do {
      p = p + (*it) * cont;
      it++; 
      sum+=cont;
      if (it == l.end()) {
	it = l.begin();
      }
      if (weighted_filter) {
	cont++;
      }
    } while (it != last_velocity.at(uav_id));
    p = p * (1.0 / (double)sum);
  }
}


void ORCAModule::publish_orca_vel(RVO::Vector3 velocity_) {
  Point3D curr_vel(velocity_.x(), velocity_.y(), velocity_.z());
  
  sendControlReferences(curr_vel);
//   ROS_INFO("Velocity command: %s.", result.toString().c_str());
}

void ORCAModule::sendControlReferences(const Point3D new_vel)
{
  arcas_msgs::QuadControlReferencesStamped res;
  
  res.quad_control_references.position_ref.x = pos_3d.at(id).x + new_vel.x;
  res.quad_control_references.position_ref.y = pos_3d.at(id).y + new_vel.y;
  res.quad_control_references.position_ref.z = pos_3d.at(id).z + new_vel.z;
  res.quad_control_references.velocity_ref = new_vel.norm();
  res.quad_control_references.heading = heading;


  // Reduce the angle to (-PI, PI]
  while(res.quad_control_references.heading > PI)
  {
    res.quad_control_references.heading -= 2*PI;
  }
  while (res.quad_control_references.heading < (-PI))
  {
    res.quad_control_references.heading += 2*PI;
  }

  control_reference_pub.publish(res);
}

void ORCAModule::receivePreferredVelocity(uint uav, const geometry_msgs::Twist &pref_vel) {
  if (sim != NULL) {
    started = true;
    
    RVO::Vector3 pv(pref_vel.linear.x, pref_vel.linear.y, pref_vel.linear.z);
    functions::Point3D p = fromRVO(pv);
    ROS_INFO("Received a preferred_velocity: %s", p.toString().c_str());
    sim->setAgentPrefVelocity(uav, pv);
  } else if (unstable && sim_unst != NULL) {
    started = true;
    RVO::Vector3 pv(pref_vel.linear.x, pref_vel.linear.y, pref_vel.linear.z);
    RVO_UNSTABLE::Vector3 pv_(pref_vel.linear.x, pref_vel.linear.y, pref_vel.linear.z);
    functions::Point3D p = fromRVO(pv);
    ROS_INFO("Received a preferred_velocity of UAV %u: %s", uav, p.toString().c_str());
    sim_unst->setAgentPrefVelocity(uav, pv_);
  }
}

Checker *ORCAModule::getChecker() const {
  Checker *ret = new Checker;
  
  ret->addProperty("publish_topic", new NTimes(1));
  ret->addProperty("v_pref_topic", new NTimes(1));
  ret->addProperty("state_topic", new NTimes(1));
  ret->addProperty("n_uavs", new NTimes(1));
  ret->addBlock("config", new NTimes(1));
  ret->addChecker("config", conf.getChecker());
  ret->addProperty("state_type", new NTimes(1));
  ret->addProperty("unstable", new NTimes(1));
  
  return ret;
}

void ORCAModule::fromBlock(const std::string &filename) {
  Checker *check = getChecker();
  try {
    ParseBlock data;
    data.load(filename.c_str());
    data.checkUsing(check);
    publish_topic = data("publish_topic").value;
    v_pref_topic = data("v_pref_topic").value;
    state_topic = data("state_topic").value;
    if (data.hasProperty("state_topic_others")) {
      state_topic_others = data("state_topic_others").value;
    } else {
      state_topic_others = state_topic;
    }
    n_uavs = data("n_uavs").as<double>();
//     if (data.hasProperty("id")) {
//       id = data("id").as<uint>();	
//     }
    conf.fromBlock(data["config"]);
    data_type = data("state_type").value;
    
    if (data.hasProperty("scenario_file")) {
      scenario_file = data("scenario_file").value;
    } else {
      scenario_file = "";
    }
    
    if (data.hasProperty("prefix")) {
      prefix = data("prefix").value;
    } else {
      prefix = "uav_";
    }
    if (data.hasProperty("filter_size")) {
      filter_size = data("filter_size").as<uint>();
    } else {
      filter_size = 0;
    }
    
    unstable = data("unstable").as<bool>();
    loaded = true;
    
    ostringstream os;
    os << id + 1;
    if (data.hasProperty("distance_file")) {
      ROS_INFO("Distance file configured.");
      distance_file = data("distance_file").value;
      distance_file.append(os.str());
    } else {
      distance_file = "";
    }
    if (data.hasProperty("orca_file")) {
      ROS_INFO("ORCA file configured.");
      orca_file = data("orca_file").value;
      orca_file.append(os.str());
    } else {
      orca_file = "";
    }
    if (data.hasProperty("preferred_file")) {
      ROS_INFO("Preferred file configured.");
      preferred_file = data("preferred_file").value;
      preferred_file.append(os.str());
    } else {
      preferred_file = "";
    }
    remap.clear();
    if (data.hasProperty("remap")) {
      std::vector<double> v = data("remap").as<vector<double> >();
      for (unsigned int i = 0; i < v.size(); i++) {
	remap.push_back(v.at(i));
      }
    }
    if (data.hasProperty("separation_file")) {
      file_sep = data("separation_file").value;
      file_sep.append(os.str());
    } else {
      file_sep = "";
    }
    if (data.hasProperty("maneuver_file")) {
      file_maneuvers = data("maneuver_file").value;
      file_maneuvers.append(os.str());
    } else {
      file_maneuvers = "";
    }
    if (data.hasProperty("time_file")) {
      time_file = data("time_file").value;
      time_file.append(os.str());
    } else {
      time_file = "";
    }
    
    if (data.hasProperty("transmit_preferred")) {
      transmit_preferred = data("transmit_preferred").as<bool>();
    } else {
      transmit_preferred = false;
    }
    
    if (data.hasProperty("weighted_filter")) {
      weighted_filter = data("weighted_filter").as<bool>();
    } else {
      weighted_filter = false;
    }
    
    if (data.hasProperty("heading")) {
      heading = data("heading").as<double>();
    } else {
      heading = 0.0;
    }
    
    if (data.hasProperty("pure_delay")) {
      pure_delay = data("pure_delay").as<double>();
    } else {
      pure_delay = 0;
    }
    
  } catch (exception &e) {
    ROS_ERROR("ORCAModule::fromBlock --> error while retrieving the configuration data. Filename: %s. Content: %s.", filename.c_str(), e.what());
    loaded = false;
  }
}

 ORCAModule::~ORCAModule() {
   delete sim;
   delete sim_unst;
   
  }

  void ORCAModule::receiveARCAS(uint uav, const arcas_msgs::QuadStateEstimationWithCovarianceStamped &msg) {
    RVO::Vector3 pos_ant = pos[uav];
    RVO::Vector3 pos_act(msg.quad_state_estimation_with_covariance.position.x, msg.quad_state_estimation_with_covariance.position.y, msg.quad_state_estimation_with_covariance.position.z);
    pos[uav] = pos_act;
  
    if (unstable) {
      pos_unst[uav] = RVO_UNSTABLE::Vector3(pos_act.x(), pos_act.y(), pos_act.z());
    }
  
    functions::Point3D vel_;
    functions::Point3D pos0 = fromRVO(pos_ant);
    functions::Point3D pos1 = fromRVO(pos[uav]);

    vel_.x = msg.quad_state_estimation_with_covariance.linear_velocity.x;
    vel_.y = msg.quad_state_estimation_with_covariance.linear_velocity.y;
    vel_.z = msg.quad_state_estimation_with_covariance.linear_velocity.z;
    vel[uav] = RVO::Vector3(vel_.x, vel_.y, vel_.z);
  
    if (unstable) {
      vel_unst[uav] = RVO_UNSTABLE::Vector3(vel_.x, vel_.y, vel_.z);
    }
    
    pos_3d.at(uav) = pos1;
  
  state_received[uav] = true;
//   ostringstream os;
//   os << "Received state of UAV " << uav << ". Content: " << pos1.toString()<< "\t";
//   os << "Estimated velocity:  " << vel_.toString();
//   ROS_INFO("%s", os.str().c_str());
  }
  
  bool ORCAModule::saveLog() const {
    bool ret_val = true;
    
    if (file_sep.length() > 0) {
      ofstream sep_file;
      sep_file.open(file_sep.c_str());

      if (sep_file.is_open()) {
	for (unsigned int i = 0; i < min_separation_xy.size(); i++) {
	  sep_file << min_separation_xy.at(i) << " " << min_separation_z.at(i) << endl;
	}
      }
      sep_file.close();
    }
    
    if(file_maneuvers.length() > 0) {
      ofstream man_file;
      man_file.open(file_maneuvers.c_str());
      
      if (man_file.is_open()) {
	for (unsigned int i = 0; i < maneuver_log.size(); i++) {
	  man_file << maneuver_log.at(i).toString() << endl;
	}
      }
      man_file.close();
    }
    
    return ret_val;
  }
  
  void ORCAModule::receiveActive (orca_module::activeConfig &config, uint32_t level) {
    this->dynamic_conf = config;
  }

void ORCAModule::log() {
  FormattedTime t;
  t.getTime();
  double t_ = t.getSec() + t.getUSec() * 0.000001;
  if (distance_file != "") {
    ofstream ofs;
    ofs.open(distance_file.c_str(), std::fstream::out | std::fstream::app);
// 	  ROS_INFO("Trying to log distance to %s", distance_file.c_str());
  
    if (ofs.is_open()) {
      ofs.precision(15);
      ofs << t_ << " ";
      ofs.precision(6);
      ofs << sim_unst->getObstacleDistance(id) << endl;
      
      ofs.close();
    } else {
      ROS_ERROR("Could not log distance to %s", distance_file.c_str());
    }
  }
  
  if (preferred_file!= "") {
    ofstream ofs;
    ofs.open(preferred_file.c_str(), std::fstream::out | std::fstream::app);
    RVO_UNSTABLE::Vector3 v = sim_unst->getAgentPrefVelocity(id);
    functions::Point3D p(v.x(), v.y(), v.z());
  
    if (ofs.is_open()) {
      ofs.precision(15);
      ofs << t_ << " " << p.toString(false) << endl;
    }
    ofs.close();
  }
  if (orca_file != "") {
    ofstream ofs;
    ofs.open(orca_file.c_str(), std::fstream::out | std::fstream::app);
  
    RVO_UNSTABLE::Vector3 v = sim_unst->getAgentVelocity(id);
    functions::Point3D p(v.x(), v.y(), v.z());
  
    if (ofs.is_open()) {
      ofs.precision(15);
      ofs << t_ << " " << p.toString(false) << endl;
    }
    ofs.close();
  }
  
  // Register the minimum separations
  // a) static obstacles
  if (min_separation_xy.at(id) > sim_unst->getObstacleDistance(id)) {
    min_separation_xy.at(id) = sim_unst->getObstacleDistance(id);
    
  }
  // b) Between pairs of uavs
  Point3D pos_id (pos.at(id).x(), pos.at(id).y(), pos.at(id).z());
  for (unsigned int j = 0; j < n_uavs; j++) {
    if (n_uavs == id) {
      continue;
    }
    Point3D pos_robot(pos.at(j).x(), pos.at(j).y(), pos.at(j).z());
    if (abs(pos_robot.z -pos_id.z) < conf.radius_z * conf.radius_z) {
      min_separation_xy.at(j) = minimum(min_separation_xy.at(j), pos_robot.distance2d(pos_id));
    }
    if (pos_robot.distance2d(pos_id) < conf.radius * conf.radius) {
      min_separation_z.at(j) = minimum(min_separation_z.at(j), abs(pos_robot.z - pos_id.z));
    }
  }
  
  // And the maneuvers
  const RVO_UNSTABLE::Vector3 &cv = sim_unst->getAgentVelocity(id);
  const RVO_UNSTABLE::Vector3 &cpv = sim_unst->getAgentPrefVelocity(id);
  Point3D curr_vel (cv.x(), cv.y(), cv.z());
  Point3D curr_pref_vel(cpv.x(), cpv.y(), cpv.z());
  if (maneuver_log.size() == 0 || !maneuver_log.at(maneuver_log.size() - 1).isAlive()) {
    // No alive maneuver --> has one started?
    
    if (sim_unst != NULL) {
      if(curr_pref_vel.distance(curr_vel) > MIN_DIFFERENCE) {
	Maneuver new_man(pos_3d, curr_vel, curr_pref_vel, id);
	maneuver_log.push_back(new_man);
      }
    }
  } else {
    maneuver_log.at(maneuver_log.size() - 1).updateManeuver(pos_3d, curr_vel, curr_pref_vel, id, sim_unst->getObstacleDistance(id));
  }
}

void ORCAModule::receiveEmergency(const std_msgs::Bool &data) {
  ROS_ERROR("Received emergency stop in uav %d.", id);
  emergency_stop = data.data != 0;
}