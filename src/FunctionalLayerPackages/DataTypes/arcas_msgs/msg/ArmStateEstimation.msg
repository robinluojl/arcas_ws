# This contains the state estimation of the robotic arm on board. That is, the velocity and position values of the arm angles and the battery position. The angle values are
# always given in the following order: Yaw1, Pitch1, Pitch2, Roll1 Pitch3, Roll2, Gripper. These angles are shown in the code API documentation of the arcas_msgs node and the developer manual
# of the functional layer.

# It indicates if the message is valid or not.
# If status = 1 => The data is valid.
# If status = 0 => The data is invalid (its values can be corrupted).
# A different value has no meaning but may be established by a particular application.
byte status

# The arm angles values in radians [rad] with respect to the arm frame.
float64[7] position

# The arm angles velocity values in radians per second [rad/s] with respect to the arm frame.
float64[7] velocity

# The servo angle value of the battery measured in radians [rad]. Its range is [-PI, PI].
float64 battery

# It indicates the arm state without checking the position values.
uint8 arm_state

# It indicates the arm is contracted.
uint8 CONTRACTED=1

# It indicates the arm is contracting.
uint8 CONTRACTING=2

# It indicates the arm is extended.
uint8 EXTENDED=3

# It indicates the arm is extending.
uint8 EXTENDING=4