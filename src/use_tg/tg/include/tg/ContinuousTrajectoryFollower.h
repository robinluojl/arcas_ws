#ifndef __TG_CONTINUOUS_TRAJECTORY__
#define __TG_CONTINUOUS_TRAJECTORY__

#include <arcas_msgs/QuadStateEstimationStamped.h>
#include <arcas_msgs/Position.h>
#include <arcas_msgs/PathWithCruiseStamped.h>
#include <arcas_actions_msgs/TakeOffAction.h>

#include <functions/Point3D.h>
#include <functions/functions.h>

#include "tg/TrajectoryFollower.h"

//! @class ContinuousTrajectoryFollower This class makes the UAV smoothly follow a continuous path (hundreds or thousands of waypoints)
//! Its behaviour depends greatly on the look_ahead_distance. Lower values of look_ahead_distance make path to be followed with more fidelity, but 
//! more aggressive maneuvers are required.
//! On the other hand, the upper look_ahead_distance, the generated path is smoother.
//! It is somehow based on the PurePursuit algorithm
//! TODO: implementation with 4d trajectories (the cruise can change depending on the time restrictions
//! TODO: adaptative lookahead implementation
class ContinuousTrajectoryFollower : public TrajectoryFollower {
public:
  ContinuousTrajectoryFollower(const arcas_msgs::PathWithCruise &path, double look_ahead_distance);
  
  inline bool done() const {
    return current_index == path.waypoints.size() - 1;
  };
  
  uint actualizeIndex(const arcas_msgs::Position &pos);
  
  uint getCurrentIndex() {return target_index;}
  
protected:
  uint target_index;
  double lookahead_distance;  
  uint actualizeTargetIndex();
};

#endif
