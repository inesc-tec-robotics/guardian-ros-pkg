/*
 * guardian_pad
 * Copyright (c) 2011, Robotnik Automation, SLL
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Robotnik Automation, SLL. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \author Robotnik Automation SLL
 * \brief Allows to use a pad with the guardian_controller, sending the messages received through the joystick input, correctly adapted, to the "guardian_controller/command" by default.
 */

#include <vector>
#include <ros/ros.h>
#include <sensor_msgs/Joy.h>
#include <geometry_msgs/Twist.h>
#include <std_srvs/Empty.h>

// Optional (modbus or rly_08)
// #include <modbus_io/write_digital_output.h>
// 
//#include <rly_08/SetRelayStatus.h>
#include <robotnik_msgs/set_digital_output.h>

// Optional (sphere or axis)
//#include <sphere_camera/ptz_state.h>
//#include <axis_camera/Axis.h>
#include <robotnik_msgs/ptz.h>

#include <diagnostic_updater/diagnostic_updater.h>
#include <diagnostic_updater/publisher.h>
#include <guardian_pad/enable_disable.h>

#define DEFAULT_NUM_OF_BUTTONS		20
#define DEFAULT_AXIS_LINEAR			1
#define DEFAULT_AXIS_ANGULAR		2
#define DEFAULT_SCALE_LINEAR		1.0
#define DEFAULT_SCALE_ANGULAR		1.0

#define DEFAULT_MAX_LINEAR_SPEED	2.0		// m/s
#define DEFAULT_MAX_ANGULAR_SPEED	40.0	// rads /s

////////////////////////////////////////////////////////////////////////
//                               NOTE:                                //
// This configuration is made for a THRUSTMASTER T-Wireless 3in1 Joy  //
//   please feel free to modify to adapt for your own joystick.       //   
// 								      //
////////////////////////////////////////////////////////////////////////

class GuardianPad {
	public:
		GuardianPad();
		//! Updates diagnostics
		void Update();

	private:
		void padCallback(const sensor_msgs::Joy::ConstPtr& joy);
		bool EnableDisable(guardian_pad::enable_disable::Request &req, guardian_pad::enable_disable::Response &res);

		ros::NodeHandle nh_;

		int linear_, angular_;
		double l_scale_, a_scale_;
		//! It will publish into command velocity (for the robot) and the ptz_state (for the pantilt)
		ros::Publisher vel_pub_, ptz_pub_;
		//! It will be suscribed to the joystick
		ros::Subscriber pad_sub_;
		//! Name of the topic where it will be publishing the velocity
		std::string cmd_topic_vel_;
		//! Name of the service where it will be modifying the digital outputs
		std::string cmd_service_io_;
		std::string tilt_laser_start_service_io_, tilt_laser_stop_service_io_;
		//! Name of the topic where it will be publishing the pant-tilt values
		std::string cmd_topic_ptz_;
		double current_linear_vel;
		double current_angular_vel;
		//! Number of the DEADMAN button
		int dead_man_button_;
		//! Number of the button for increase or decrease the speed max of the joystick
		int speed_linear_up_button_, speed_linear_down_button_;
		int speed_angular_up_button_, speed_angular_down_button_;
		int button_output_1_, button_output_2_;
		int button_tilt_laser_start_, button_tilt_laser_stop_;
		int output_1_, output_2_;
		bool bOutput1, bOutput2;
		//! buttons to the pan-tilt-zoom camera
		int ptz_tilt_up_, ptz_tilt_down_, ptz_pan_right_, ptz_pan_left_;
		int ptz_zoom_wide_, ptz_zoom_tele_;
		//! Service to modify the digital outputs
		ros::ServiceClient set_digital_outputs_client_, button_tilt_laser_start_client_, button_tilt_laser_stop_client_;
		bool tilt_laser_active_;
		std_srvs::Empty empty_srv_;
		//! Enables/disables the pad
		ros::ServiceServer enable_disable_srv_;
		//! Number of buttons of the joystick
		int num_of_buttons_;
		//! Pointer to a vector for controlling the event when pushing the buttons
		std::vector<bool> registered_button_event_;
		// DIAGNOSTICS
		//! Diagnostic to control the frequency of the published command velocity topic
		diagnostic_updater::HeaderlessTopicDiagnostic *pub_command_freq;
		//! Diagnostic to control the reception frequency of the subscribed joy topic
		diagnostic_updater::HeaderlessTopicDiagnostic *sus_joy_freq;
		//! General status diagnostic updater
		diagnostic_updater::Updater updater_pad;
		//! Diagnostics min freq
		double min_freq_command, min_freq_joy; //
		//! Diagnostics max freq
		double max_freq_command, max_freq_joy; //
		//! Flag to enable/disable the communication with the publishers topics
		bool bEnable;
		//! Pan & tilt increment (degrees)
		int pan_increment_, tilt_increment_;
		//! Zoom increment (steps)
		int zoom_increment_;
		double speed_linear_increment_;
		double speed_angular_increment_;
		double max_linear_speed_, max_angular_speed_;
};

GuardianPad::GuardianPad() :
		linear_(1), angular_(2), speed_linear_increment_(0.05), speed_angular_increment_(0.07), tilt_laser_active_(false) {
	// 
	nh_.param("num_of_buttons", num_of_buttons_, DEFAULT_NUM_OF_BUTTONS);
	// MOTION CONF
	nh_.param("axis_linear", linear_, DEFAULT_AXIS_LINEAR);
	nh_.param("axis_angular", angular_, DEFAULT_AXIS_ANGULAR);
	nh_.param("scale_angular", a_scale_, DEFAULT_SCALE_ANGULAR);
	nh_.param("scale_linear", l_scale_, DEFAULT_SCALE_LINEAR);
	nh_.param("cmd_topic_vel", cmd_topic_vel_, std::string("cmd_vel"));
	nh_.param("button_dead_man", dead_man_button_, -1);
	nh_.param("button_linear_speed_up", speed_linear_up_button_, -1);
	nh_.param("button_linear_speed_down", speed_linear_down_button_, -1);
	nh_.param("button_angular_speed_up", speed_angular_up_button_, -1);
	nh_.param("button_angular_speed_down", speed_angular_down_button_, -1);

	// DIGITAL OUTPUTS CONF
	nh_.param("cmd_service_io", cmd_service_io_, cmd_service_io_);
	nh_.param("button_output_1", button_output_1_, -1);
	nh_.param("button_output_2", button_output_2_, -1);
	nh_.param("output_1", output_1_, -1);
	nh_.param("output_2", output_2_, -1);
	// PANTILT-ZOOM CONF
	nh_.param("cmd_topic_ptz", cmd_topic_ptz_, cmd_topic_ptz_);
	nh_.param("button_ptz_tilt_up", ptz_tilt_up_, -1);
	nh_.param("button_ptz_tilt_down", ptz_tilt_down_, -1);
	nh_.param("button_ptz_pan_right", ptz_pan_right_, -1);
	nh_.param("button_ptz_pan_left", ptz_pan_left_, -1);
	nh_.param("button_ptz_zoom_wide", ptz_zoom_wide_, -1);
	nh_.param("button_ptz_zoom_tele", ptz_zoom_tele_, -1);
	nh_.param("button_tilt_laser_start_stop", button_tilt_laser_start_, -1);
	nh_.param("pan_increment", pan_increment_, -1);
	nh_.param("tilt_increment", tilt_increment_, -1);
	nh_.param("zoom_increment", zoom_increment_, -1);
	nh_.param("speed_linear_increment", speed_linear_increment_, 0.05);
	nh_.param("speed_angular_increment", speed_angular_increment_, 0.07);
	nh_.param("tilt_laser_start_service_io", tilt_laser_start_service_io_, std::string("/robotnik_tilt_laser/start"));
	nh_.param("tilt_laser_stop_service_io", tilt_laser_stop_service_io_, std::string("/robotnik_tilt_laser/stop"));
	nh_.param("max_linear_speed", max_linear_speed_, 1.5);
	nh_.param("max_angular_speed", max_angular_speed_, 1.57);
	nh_.param("initial_linear_vel", current_linear_vel, 0.5);
	nh_.param("initial_angular_vel", current_angular_vel, 0.79);

	registered_button_event_.resize(num_of_buttons_, false);

	/*ROS_INFO("Service I/O = [%s]", cmd_service_io_.c_str());
	 ROS_INFO("Topic PTZ = [%s]", cmd_topic_ptz_.c_str());
	 ROS_INFO("Service I/O = [%s]", cmd_topic_vel_.c_str());
	 ROS_INFO("Axis linear = %d", linear_);
	 ROS_INFO("Axis angular = %d", angular_);
	 ROS_INFO("Scale angular = %d", a_scale_);
	 ROS_INFO("Deadman button = %d", dead_man_button_);
	 ROS_INFO("OUTPUT1 button %d", button_output_1_);
	 ROS_INFO("OUTPUT2 button %d", button_output_2_);
	 ROS_INFO("OUTPUT1 button %d", button_output_1_);
	 ROS_INFO("OUTPUT2 button %d", button_output_2_);*/
	// Publish through the node handle Twist type messages to the guardian_controller/command topic
	vel_pub_ = nh_.advertise<geometry_msgs::Twist>(cmd_topic_vel_, 1);
	//  Publishes into the pant-tilt spherecam
	// ptz_pub_ = nh_.advertise<sphere_camera::ptz_state>(cmd_topic_ptz_, 1);
	ptz_pub_ = nh_.advertise<robotnik_msgs::ptz>(cmd_topic_ptz_, 1);
	//
	// Listen through the node handle sensor_msgs::Joy messages from joystick (these are the orders that we will send to guardian_controller/command)
	pad_sub_ = nh_.subscribe<sensor_msgs::Joy>("joy", 10, &GuardianPad::padCallback, this);

	// Request service to activate / deactivate digital I/O
	//set_digital_outputs_client_ = nh_.serviceClient<modbus_io::write_digital_output>(cmd_service_io_);
	set_digital_outputs_client_ = nh_.serviceClient<robotnik_msgs::set_digital_output>(cmd_service_io_);
	button_tilt_laser_start_client_ = nh_.serviceClient<std_srvs::Empty>(tilt_laser_start_service_io_);
	button_tilt_laser_stop_client_ = nh_.serviceClient<std_srvs::Empty>(tilt_laser_stop_service_io_);
	bOutput1 = bOutput2 = false;
	// Advertises new service to enable/disable the pad
	enable_disable_srv_ = nh_.advertiseService("/guardian_pad/enable_disable", &GuardianPad::EnableDisable, this);

	// Diagnostics
	updater_pad.setHardwareID("None");
	// Topics freq control 
	min_freq_command = min_freq_joy = 5.0;
	max_freq_command = max_freq_joy = 50.0;
	sus_joy_freq = new diagnostic_updater::HeaderlessTopicDiagnostic("/joy", updater_pad, diagnostic_updater::FrequencyStatusParam(&min_freq_joy, &max_freq_joy, 0.1, 10));

	pub_command_freq = new diagnostic_updater::HeaderlessTopicDiagnostic(cmd_topic_vel_.c_str(), updater_pad, diagnostic_updater::FrequencyStatusParam(&min_freq_command, &max_freq_command, 0.1, 10));

	bEnable = true;	// Communication flag enabled by default

}

/*
 *	\brief Updates the diagnostic component. Diagnostics
 *
 */
void GuardianPad::Update() {
	updater_pad.update();
}

/*
 *	\brief Enables/Disables the pad
 *
 */
bool GuardianPad::EnableDisable(guardian_pad::enable_disable::Request &req, guardian_pad::enable_disable::Response &res) {
	bEnable = req.value;

	ROS_INFO("GuardianPad::EnablaDisable: Setting to %d", req.value);
	res.ret = true;
	return true;
}

/*
 *	\brief Method call when receiving a message from the topic /joy
 *
 */
void GuardianPad::padCallback(const sensor_msgs::Joy::ConstPtr& joy) {
	geometry_msgs::Twist vel;
	robotnik_msgs::ptz ptz;
	bool ptzEvent = false;
	bool stop_pub_cmd = false; //

	//ROS_ERROR("EVENT JOY");	
	// Actions dependant on dead-man button
	if (dead_man_button_ >= 0 && dead_man_button_ < joy->buttons.size() && joy->buttons[dead_man_button_] == 1) {
		bEnable = true;
		//ROS_ERROR("GuardianPan::padCallback: DEADMAN button %d", dead_man_button_);
		// Set the current velocity level
		bool changed_speed = false;
		if (speed_linear_down_button_ >= 0 && speed_linear_down_button_ < joy->buttons.size()) {
			if (joy->buttons[speed_linear_down_button_] == 1) {
				if (!registered_button_event_[speed_linear_down_button_]) {
					if (current_linear_vel > speed_linear_increment_) { current_linear_vel -= speed_linear_increment_; } else { current_linear_vel = 0.0; }
					registered_button_event_[speed_linear_down_button_] = true;
					changed_speed = true;
				}
			} else {
				registered_button_event_[speed_linear_down_button_] = false;
			}
		}

		if (speed_angular_down_button_ >= 0 && speed_angular_down_button_ < joy->buttons.size()) {
			if (joy->buttons[speed_angular_down_button_] == 1) {
				if (!registered_button_event_[speed_angular_down_button_]) {
					if (current_angular_vel > speed_angular_increment_) { current_angular_vel -= speed_angular_increment_; } else { current_angular_vel = 0.0; }
					registered_button_event_[speed_angular_down_button_] = true;
					changed_speed = true;
				}
			} else {
				registered_button_event_[speed_angular_down_button_] = false;
			}
		}

		if (speed_linear_up_button_ >= 0 && speed_linear_up_button_< joy->buttons.size()) {
			if (joy->buttons[speed_linear_up_button_] == 1) {
				if (!registered_button_event_[speed_linear_up_button_]) {
					if (current_linear_vel < max_linear_speed_) { current_linear_vel += speed_linear_increment_; } else { current_linear_vel = max_linear_speed_; }
					registered_button_event_[speed_linear_up_button_] = true;
					changed_speed = true;
				}
			} else {
				registered_button_event_[speed_linear_up_button_] = false;
			}
		}

		if (speed_angular_up_button_ >= 0 && speed_angular_up_button_< joy->buttons.size()) {
			if (joy->buttons[speed_angular_up_button_] == 1) {
				if (!registered_button_event_[speed_angular_up_button_]) {
					if (current_angular_vel < max_angular_speed_) { current_angular_vel += speed_angular_increment_; } else { current_angular_vel = max_angular_speed_; }
					registered_button_event_[speed_angular_up_button_] = true;
					changed_speed = true;
				}
			} else {
				registered_button_event_[speed_angular_up_button_] = false;
			}
		}

		if (current_angular_vel < -max_angular_speed_) current_angular_vel = -max_angular_speed_;
		if (current_angular_vel > max_angular_speed_) current_angular_vel = max_angular_speed_;
		if (current_linear_vel < -max_linear_speed_) current_linear_vel = -max_linear_speed_;
		if (current_linear_vel > max_linear_speed_) current_linear_vel = max_linear_speed_;

		vel.angular.x = 0.0;
		vel.angular.y = 0.0;
		vel.angular.z = current_angular_vel * a_scale_ * joy->axes[angular_];
		vel.linear.x = current_linear_vel * l_scale_ * joy->axes[linear_];
		vel.linear.y = 0.0;
		vel.linear.z = 0.0;

		if (vel.angular.z < -max_angular_speed_) vel.angular.z = -max_angular_speed_;
		if (vel.angular.z > max_angular_speed_) vel.angular.z = max_angular_speed_;
		if (vel.linear.x < -max_linear_speed_) vel.linear.x = -max_linear_speed_;
		if (vel.linear.x > max_linear_speed_) vel.linear.x = max_linear_speed_;

		if (changed_speed) {
			ROS_INFO("Linear velocity: %f m/s | %f%% || Angular velocity: %f rad/s | %f%%", current_linear_vel, (current_linear_vel / max_linear_speed_) * 100.0, current_angular_vel, (current_angular_vel / max_angular_speed_) * 100.0);
		}

		// LIGHTS
		if (button_output_1_ >= 0 && button_output_1_ < joy->buttons.size()) {
			if (joy->buttons[button_output_1_] == 1) {
				if (!registered_button_event_[button_output_1_]) {
					//ROS_INFO("GuardianPan::padCallback: OUTPUT1 button %d", button_output_1_);
					/*modbus_io::write_digital_output write_do_srv;
					 write_do_srv.request.output = output_1_;
					 bOutput1=!bOutput1;
					 write_do_srv.request.value = bOutput1;*/
					robotnik_msgs::set_digital_output write_do_srv;
					write_do_srv.request.output = output_1_;
					bOutput1 = !bOutput1;
					write_do_srv.request.value = bOutput1;

					if (bEnable) {
						set_digital_outputs_client_.call(write_do_srv);
						registered_button_event_[button_output_1_] = true;
					}
				}
			} else {
				registered_button_event_[button_output_1_] = false;
			}
		}

		if (button_output_2_ >= 0 && button_output_2_ < joy->buttons.size()) {
			if (joy->buttons[button_output_2_] == 1) {
				if (!registered_button_event_[button_output_2_]) {
					//ROS_INFO("GuardianPan::padCallback: OUTPUT2 button %d", button_output_2_);
					/*modbus_io::write_digital_output write_do_srv;
					 write_do_srv.request.output = output_2_;
					 bOutput2=!bOutput2;
					 write_do_srv.request.value = bOutput2;*/
					robotnik_msgs::set_digital_output write_do_srv;
					write_do_srv.request.output = output_2_;
					bOutput2 = !bOutput2;
					write_do_srv.request.value = bOutput2;

					if (bEnable) {
						set_digital_outputs_client_.call(write_do_srv);
						registered_button_event_[button_output_2_] = true;
					}
				}
			} else {
				registered_button_event_[button_output_2_] = false;
			}
		}

		// SPHERECAM
		// TILT-MOVEMENTS (RELATIVE POS)
		ptz.pan = ptz.tilt = ptz.zoom = 0.0;
		ptz.relative = true;
		if (ptz_tilt_up_ >= 0 && ptz_tilt_up_ < joy->buttons.size()) {
			if (joy->buttons[ptz_tilt_up_] == 1) {
				if (!registered_button_event_[ptz_tilt_up_]) {
					ptz.tilt = tilt_increment_;
					//ROS_INFO("GuardianPan::padCallback: TILT UP");
					registered_button_event_[ptz_tilt_up_] = true;
					ptzEvent = true;
				}
			} else {
				registered_button_event_[ptz_tilt_up_] = false;
			}
		}

		if (ptz_tilt_down_ >= 0 && ptz_tilt_down_ < joy->buttons.size()) {
			if (joy->buttons[ptz_tilt_down_] == 1) {
				if (!registered_button_event_[ptz_tilt_down_]) {
					ptz.tilt = -tilt_increment_;
					//ROS_INFO("GuardianPan::padCallback: TILT DOWN");
					registered_button_event_[ptz_tilt_down_] = true;
					ptzEvent = true;
				}
			} else {
				registered_button_event_[ptz_tilt_down_] = false;
			}
		}

		// PAN-MOVEMENTS (RELATIVE POS)
		if (ptz_pan_left_ >= 0 && ptz_pan_left_ < joy->buttons.size()) {
			if (joy->buttons[ptz_pan_left_] == 1) {
				if (!registered_button_event_[ptz_pan_left_]) {
					ptz.pan = -pan_increment_;
					//ROS_INFO("GuardianPan::padCallback: PAN LEFT");
					registered_button_event_[ptz_pan_left_] = true;
					ptzEvent = true;
				}
			} else {
				registered_button_event_[ptz_pan_left_] = false;
			}
		}

		if (ptz_pan_right_ >= 0 && ptz_pan_right_ < joy->buttons.size()) {
			if (joy->buttons[ptz_pan_right_] == 1) {
				if (!registered_button_event_[ptz_pan_right_]) {
					ptz.pan = pan_increment_;
					//ROS_INFO("GuardianPan::padCallback: PAN RIGHT");
					registered_button_event_[ptz_pan_right_] = true;
					ptzEvent = true;
				}
			} else {
				registered_button_event_[ptz_pan_right_] = false;
			}
		}

		// TILT LASER
		if (button_tilt_laser_start_ >= 0 && button_tilt_laser_start_ < joy->buttons.size()) {
			if (joy->buttons[button_tilt_laser_start_] == 1) {
				if (!registered_button_event_[button_tilt_laser_start_]) {
					if (tilt_laser_active_) {
						ROS_INFO("Stopping tilt laser");
						button_tilt_laser_stop_client_.call(empty_srv_);
						tilt_laser_active_ = false;
					} else {
						ROS_INFO("Starting tilt laser");
						button_tilt_laser_start_client_.call(empty_srv_);
						tilt_laser_active_ = true;
					}
					registered_button_event_[button_tilt_laser_start_] = true;
				}
			} else {
				registered_button_event_[button_tilt_laser_start_] = false;
			}
		}


		// ZOOM Settings (RELATIVE)
		if (ptz_zoom_wide_ >= 0 && ptz_zoom_wide_ < joy->buttons.size()) {
			if (joy->buttons[ptz_zoom_wide_] == 1) {
				if (!registered_button_event_[ptz_zoom_wide_]) {
					ptz.zoom = -zoom_increment_;
					//ROS_INFO("GuardianPan::padCallback: ZOOM WIDe");
					registered_button_event_[ptz_zoom_wide_] = true;
					ptzEvent = true;
				}
			} else {
				registered_button_event_[ptz_zoom_wide_] = false;
			}
		}

		if (ptz_zoom_tele_ >= 0 && ptz_zoom_tele_ < joy->buttons.size()) {
			if (joy->buttons[ptz_zoom_tele_] == 1) {
				if (!registered_button_event_[ptz_zoom_tele_]) {
					ptz.zoom = zoom_increment_;
					//ROS_INFO("GuardianPan::padCallback: ZOOM TELE");
					registered_button_event_[ptz_zoom_tele_] = true;
					ptzEvent = true;
				}
			} else {
				registered_button_event_[ptz_zoom_tele_] = false;
			}
		}
	} else {
		vel.angular.x = 0.0;
		vel.angular.y = 0.0;
		vel.angular.z = 0.0;
		vel.linear.x = 0.0;
		vel.linear.y = 0.0;
		vel.linear.z = 0.0;
		stop_pub_cmd = true; //we still want to send vel = 0 to stop the robot
		//bEnable = false;
	}

	sus_joy_freq->tick();	// Ticks the reception of joy events

	// Only publishes if it's enabled
	if (bEnable) {
		if (ptzEvent) ptz_pub_.publish(ptz);
		vel_pub_.publish(vel);
		pub_command_freq->tick();
		if (stop_pub_cmd) //but after send vel 0 we don't want to send more
			bEnable = false;
	}

}

int main(int argc, char** argv) {
	ros::init(argc, argv, "guardian_pad");
	GuardianPad guardian_pad;

	ros::Rate r(50.0);

	while (ros::ok()) {

		// UPDATING DIAGNOSTICS
		guardian_pad.Update();
		ros::spinOnce();
		r.sleep();
	}

}

