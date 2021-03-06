// Copyright (c) 2019 Intel Corporation. All Rights Reserved
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file control_base.hpp
 * @author Yu Yan
 * @date 29 Sep 2019
 * @brief Native robot control interface for visual manipulation.
 *
 * This file contains the control interface template to make the visual grasping. The interface is used 
 * for the control between a PC and an industrial robot controller. Collision detection is not considered in this
 * interface. The specific behaviors are supposed to be filled with the communication protocal of an 
 * industrial robot, which are usually specified by the robot manufacturors. 
 */

#pragma once

#include <mutex>
#include <Eigen/Geometry>
#include <rclcpp/rclcpp.hpp>
#include <tf2_eigen/tf2_eigen.h>
#include <sensor_msgs/msg/joint_state.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>

/**
 * @brief Data type to represent robot arm's end-effector pose in 3D cartesian space.
 * 
 * @note <b>TCP</b> stands for Tool Center Point. Usually it refers to The location on the end effector 
 * or tool of a robot manipulator whose position and orientation define the coordinates of the controlled object.
 */
struct TcpPose
{
  double x; /**< Translation along X axis. */
  double y; /**< Translation along Y axis. */
  double z; /**< Translation along Z axis. */
  double alpha; /**< Euler angle of the rotation along X axis. */
  double beta;  /**< Euler angle of the rotation along Y axis. */
  double gamma; /**< Euler angle of the rotation along Z axis. */
};

/**
 * @brief Robot arm control interface.
 */
class ArmControlBase: public rclcpp::Node
{
public:

  /**
   * @brief Constructor of class #ArmControlBase.
   * @param node_name The name of the ROS2 node.
   * @param options ROS2 node options.
   */
  ArmControlBase(const std::string node_name, const rclcpp::NodeOptions & options)
  : Node(node_name, options)
  {
    joint_pub_ = this->create_publisher<sensor_msgs::msg::JointState>("/joint_states", 1);
    time_out_ = 15.0;
  }

  /**
   * @brief Default destructor of class #ArmControlBase.
   */
  virtual ~ArmControlBase()
  {
  }

  /**
   * @brief Move the robot end-effector to a goal pose (position and orientation) in 3D Cartesian space.
   * @param x Goal position on X dimension.
   * @param y Goal position on Y dimension.
   * @param z Goal position on Z dimension.
   * @param alpha Goal rotation euler angle along X axis.
   * @param beta Goal rotation euler angle along Y axis.
   * @param gamma Goal rotation euler angle along Z axis.
   * @param vel Max joint velocity. 
   * @param acc Max joint acceleration.
   * @return If the robot successfully receives the "move" command, return True. Otherwise, return false.
   */
  virtual bool moveToTcpPose(double x, double y, double z, 
                             double alpha, double beta, double gamma, 
                             double vel, double acc) = 0;

  /**
   * @brief Move the robot end-effector to a goal pose (position and orientation) in 3D Cartesian space.
   * @param pose Goal pose as a Eigen transform (Isometry3d).
   * @param vel Max joint velocity. 
   * @param acc Max joint acceleration.
   * @return If the robot successfully receives the "move" command, return True. Otherwise, return false.
   */
  virtual bool moveToTcpPose(const Eigen::Isometry3d& pose, double vel, double acc);

  /**
   * @brief Open the robot gripper and make it ready for grasping.
   * @param distance How large the fingers of the gripper open.
   * @return If the robot successfully receives the "open" command, return true. Otherwise, return false.
   */
  virtual bool open(const double distance = 0) = 0;

  /**
   * @brief Close the robot gripper and let it grasp an object.
   * @param distance How large the fingers of the gripper close.
   * @return If the robot successfully receives the "close" command, return true. Otherwise, return false.
   */
  virtual bool close(const double distance = 0) = 0;

  /**
   * @brief Make the robot arm to pick an object from a grasp pose.
   * 
   * This function contains a sequence of motions: 
   * -# Move the end-effector to a pose above the object.
   * -# Open gripper.
   * -# Stretch the end-effector along its Z axis to the grasp pose that gripper can grasp the object.
   * -# Close gripper.
   * -# Move the end-effector back to the pose above the object.
   * 
   * @param x Position of grasp pose on X dimension.
   * @param y Position of grasp pose on Y dimension.
   * @param z Position of grasp pose on Z dimension.
   * @param alpha Rotation euler angle of grasp pose along X axis.
   * @param beta Rotation euler angle of grasp pose along Y axis.
   * @param gamma Rotation euler angle of grasp pose along Z axis.
   * @param vel Max joint velocity. 
   * @param acc Max joint acceleration.
   * @param vel_scale Scale factor to slow down the end-effector velocity, when it stretch to or move back from the grasp pose.
   * @param approach The stretch distance.
   * @return If the robot successfully finishes the "pick" motions, return True. Otherwise, return false.
   * @note The grasp pose should have Z axis point out from the end-effector link.
   */
  virtual bool pick(double x, double y, double z, 
                    double alpha, double beta, double gamma, 
                    double vel, double acc, double vel_scale, double approach);
  
    /**
   * @brief Make the robot arm to pick an object from a grasp pose.
   * 
   * This function contains a sequence of motions: 
   * -# Move the end-effector to a pose above the object.
   * -# Open gripper.
   * -# Stretch the end-effector along its Z axis to the grasp pose that gripper can grasp the object.
   * -# Close gripper.
   * -# Move the end-effector back to the pose above the object.
   * 
   * @param pose_stamped Pose received from the grasp planning algorithm. See also https://github.com/intel/ros2_grasp_library.
   * @param vel Max joint velocity. 
   * @param acc Max joint acceleration.
   * @param vel_scale Scale factor to slow down the end-effector velocity, when it stretch to or move back from the grasp pose.
   * @param approach The stretch distance.
   * @return If the robot successfully finishes the "pick" motions, return True. Otherwise, return false.
   * @note The grasp pose should have Z axis point out from the end-effector link.
   */
  virtual bool pick(const geometry_msgs::msg::PoseStamped& pose_stamped, 
                    double vel, double acc, double vel_scale, double approach);

  /**
   * @brief Make the robot arm to place an object from a place pose.
   * 
   * This function contains a sequence of motions: 
   * -# Move the end-effector to a pre-place pose.
   * -# Stretch the end-effector along its Z axis to the place pose.
   * -# Open gripper.
   * -# Move the end-effector back to the pre-place pose.
   * 
   * @param x Position of place pose on X dimension.
   * @param y Position of place pose on Y dimension.
   * @param z Position of place pose on Z dimension.
   * @param alpha Rotation euler angle of place pose along X axis.
   * @param beta Rotation euler angle of place pose along Y axis.
   * @param gamma Rotation euler angle of place pose along Z axis.
   * @param vel Max joint velocity. 
   * @param acc Max joint acceleration.
   * @param vel_scale Scale factor to slow down the end-effector velocity, when it stretch to or move back from the place pose.
   * @param retract The retract distance from the place pose.
   * @return If the robot successfully finishes the "place" motions, return True. Otherwise, return false.
   * @note The place pose should have Z axis point out from the end-effector link.
   */
  virtual bool place(double x, double y, double z, 
                     double alpha, double beta, double gamma,
                     double vel, double acc, double vel_scale, double retract);

  /**
   * @brief Make the robot arm to place an object from a place pose.
   * 
   * This function contains a sequence of motions: 
   * -# Move the end-effector to a pre-place pose.
   * -# Stretch the end-effector along its Z axis to the place pose.
   * -# Open gripper.
   * -# Move the end-effector back to the pre-place pose.
   * 
   * @param pose_stamped Pose of the end-effector to place an object.
   * @param vel Max joint velocity. 
   * @param acc Max joint acceleration.
   * @param vel_scale Scale factor to slow down the end-effector velocity, when it stretch to or retract back from the place pose.
   * @param retract The retract distance from the place pose.
   * @return If the robot successfully finishes the "place" motions, return True. Otherwise, return false.
   * @note The place pose should have Z axis point out from the end-effector link.
   */
  virtual bool place(const geometry_msgs::msg::PoseStamped& pose_stamped,
                     double vel, double acc, double vel_scale, double retract);

  /**
   * @brief Convert <b>geometry_msgs::msg::PoseStamped</b> to #TcpPose
   * 
   * @param pose_stamped Pose of the end-effector.
   * @param tcp_pose Variable to store the converted result.
   */
  void toTcpPose(const geometry_msgs::msg::PoseStamped& pose_stamped, TcpPose& tcp_pose);

  /**
   * @brief Convert <b>Eigen::Isometry3d</b> to #TcpPose
   * 
   * @param pose Pose of the end-effector.
   * @param tcp_pose Variable to store the converted result.
   */
  void toTcpPose(const Eigen::Isometry3d& pose, TcpPose& tcp_pose);

  /**
   * @brief Function to check if the end-effector arrived the goal pose.
   * 
   * @param tcp_goal Goal pose of the end-effector.
   * @return If the end-effector arrived the goal pose within a <b>time_out_</b> duration, return true. Otherwise, return false.
   */
  virtual bool checkTcpGoalArrived(Eigen::Isometry3d& tcp_goal);

  /** 
   * @brief Parse arguments
   */
  virtual void parseArgs() = 0;

  /**
   * @brief Start control loop
   */
  virtual bool startLoop() = 0;

protected:
  /// Joint state publisher
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_pub_;
  /// Joint names
  std::vector<std::string> joint_names_;
  /// Current end-effctor pose
  TcpPose tcp_pose_;
  /// Mutex to guard the tcp_pose usage
  std::mutex m_;
  /// Time duration to finish a pick or place task
  double time_out_;
};