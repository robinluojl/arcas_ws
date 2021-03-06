#include "util.h"
#include <ros/node_handle.h>

using namespace std;
using namespace functions;
using namespace arcas_msgs;

arcas_msgs::PathWithCruise loadTrajetory(const char* filename, bool velocity, bool debug)
{
  arcas_msgs::PathWithCruise ret;
  
  vector<vector<double> > vec;
  double t = 0.0;
  functions::Point3D p0;
  bool first = true;
  
  if (getMatrixFromFile(string(filename), vec)) {
    unsigned int starting_column = velocity?0:1;
    for (unsigned int i = 0; i < vec.size(); i++) {
      vector<double> &row = vec.at(i);
      if (row.size() >= 4 ) {
	WayPointWithCruise curr_way;
	// The row is valid --> continue
	functions::Point3D p1(row.at(starting_column), row.at(starting_column + 1), row.at(starting_column + 2));
	if (first && !velocity) {
	  first = false;
	  curr_way.cruise = 0.5;
	} else {
	  if (!velocity) {
	    if (row.at(0) - t > 0.0000001) {
	      curr_way.cruise = p1.distance(p0) / (row.at(0) - t);
	    } else {
	      curr_way.cruise = ret.waypoints.at(ret.waypoints.size() - 1).cruise;
	    }
	  } else {
	    curr_way.cruise = row.at(3);
	  }
	}
	curr_way.x = p1.x;
	curr_way.y = p1.y;
	curr_way.z = p1.z;
	p0 = p1;
	t = row.at(0);
	ret.waypoints.push_back(curr_way);
	if (debug) {
	  cout << "Loaded waypoint: " << p1.toString() << ". Cruise: " << curr_way.cruise << endl;
	}
      }
    }
  }
  
  return ret;
}

Visualization::Visualization(double width)
{
  ros::NodeHandle n;
  pub_marker = n.advertise<visualization_msgs::Marker>( "path_marker", 0 );
  this->width = width;
}

void Visualization::representTrajectory(int uav, const PathWithCruise& path)
{
  visualization_msgs::Marker marker;
  marker.header.frame_id = "/map";
  marker.header.stamp = ros::Time();
  marker.ns = "";
  marker.id = uav;
  marker.type = visualization_msgs::Marker::LINE_STRIP;
  marker.action = visualization_msgs::Marker::ADD;
  marker.scale.x = width;
  marker.color.a = 0.5;
  marker.color.b = 0.0;
  marker.color.g = 0.0;
  marker.color.r = 0.0;
  switch (uav%6) {
    case 1:
      marker.color.b = 1.0;
      break;
      
    case 2:
      marker.color.r = 1.0;
      break;
      
    case 3:
      marker.color.g = 1.0;
      marker.color.b = 1.0;
      marker.color.r = 1.0;
      break;
      
    case 4:
      marker.color.b = 1.0;
      marker.color.r = 1.0;
      break;
    
    case 5:
      marker.color.g = 1.0;
      marker.color.r = 1.0;
      break;
      
    default:
      marker.color.b = 1.0;
      marker.color.g = 1.0;
      break;
  }
  
  for (unsigned int i = 0; i < path.waypoints.size(); i++) {
    geometry_msgs::Point p;
    p.x = path.waypoints.at(i).x;
    p.y = path.waypoints.at(i).y;
    p.z = path.waypoints.at(i).z;
    marker.points.push_back(p);
  }
  pub_marker.publish( marker );
}
