#include <ual/roscommunication.h>


RosCommunication::RosCommunication(ros::NodeHandle *n, int uavId):
    _last_seq(0)
{
    quad_state_pub_ =
            n->advertise<arcas_msgs::QuadStateEstimationWithCovarianceStamped> ("quad_state_estimation", 0);

    quad_control_references_sub_ = n->subscribe("quad_control_references",
                                                1, &RosCommunication::quadControlRefCallback,
                                                this);

    send_loop_timer_ = n->createTimer(ros::Duration(1/SEND_RATE),
                                      boost::bind(&RosCommunication::sendLoop, this, _1));
}

QuadControlReferencesStamped RosCommunication::getQuadControlReferences()
{
    return last_quad_control_references_;
}

void RosCommunication::setQuadStateEstimation(QuadStateEstimationWithCovarianceStamped q_state)
{
    quad_state_to_send_ = q_state;
}

void RosCommunication::quadControlRefCallback(const arcas_msgs::QuadControlReferencesStamped::ConstPtr& q)
{
    last_quad_control_references_ = *q;

    if(last_quad_control_references_.quad_control_references.position_ref.z < TAKE_OFF_Z)
    {
        last_quad_control_references_.quad_control_references.position_ref.z = TAKE_OFF_Z;
    } else if (last_quad_control_references_.quad_control_references.position_ref.z > CATEC_MAX_Z) {
        last_quad_control_references_.quad_control_references.position_ref.z = CATEC_MAX_Z;
    }
    
    

    if(last_quad_control_references_.quad_control_references.velocity_ref	> MAXIMUM_VELOCITY)
    {
        last_quad_control_references_.quad_control_references.velocity_ref = MAXIMUM_VELOCITY;
    }

    while(last_quad_control_references_.quad_control_references.heading > M_PI)
        last_quad_control_references_.quad_control_references.heading -= 2*M_PI;

    while(last_quad_control_references_.quad_control_references.heading < -M_PI)
        last_quad_control_references_.quad_control_references.heading += 2*M_PI;
}
void RosCommunication::sendLoop(const ros::TimerEvent& te)
{
    //std::cerr << "RosCommunication::sendLoop" << std::endl;
    if(quad_state_to_send_.header.seq!= _last_seq)
    {
        quad_state_pub_.publish(quad_state_to_send_);
        _last_seq = quad_state_to_send_.header.seq;
    }
}
