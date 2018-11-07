//example ROS client:
// first run: rosrun example_ROS_service example_ROS_service
// then start this node:  rosrun example_ROS_service example_ROS_client


#include <ros/ros.h>
#include <std_srvs/Trigger.h>
#include <iostream>
#include <string>
#include <osrf_gear/ConveyorBeltControl.h>
#include <osrf_gear/DroneControl.h>
#include <osrf_gear/LogicalCameraImage.h>

bool g_take_new_snapshot = false;
osrf_gear::LogicalCameraImage g_cam1_data;
using namespace std;

void cam2CB(const osrf_gear::LogicalCameraImage& message_holder){
    if(g_take_new_snapshot){
	ROS_INFO_STREAM("first camera image: " << message_holder << endl);
	ROS_INFO_STREAM("second camera image: " << message_holder.models.size() << endl);
	g_cam1_data = message_holder;
	ROS_INFO_STREAM("g_cam1_data: " << g_cam1_data.models.size() << endl);
    }
}

int main(int argc, char **argv) {
    ros::init(argc, argv, "ps6");
    ros::NodeHandle n;
    ros::ServiceClient startup_client = n.serviceClient<std_srvs::Trigger>("/ariac/start_competition");
    ros::ServiceClient conveyor_client = n.serviceClient<osrf_gear::ConveyorBeltControl>("/ariac/conveyor/control");
    ros::ServiceClient drone_client = n.serviceClient<osrf_gear::DroneControl>("/ariac/drone");
    std_srvs::Trigger startup_server;
    osrf_gear::ConveyorBeltControl conveyor_server;
    osrf_gear::DroneControl drone_server; 
   
    ros::Subscriber cam2_subscriber = n.subscribe("/ariac/logical_camera_2", 1, cam2CB);
    
    startup_server.response.success = false;
    while(!startup_server.response.success){
        ROS_WARN("not successful start up");
        startup_client.call(startup_server);
        ros::Duration(0.5).sleep();
    }
    ROS_INFO("got success response");

    conveyor_server.request.power = 100.0;
    conveyor_server.response.success = false;
    while(!conveyor_server.response.success){
        ROS_WARN("not successful starting conveyor yet");
        conveyor_client.call(conveyor_server);
        ros::Duration(0.5).sleep();
    }
    ROS_INFO("I see a box");
    ROS_INFO("got success response");


    //if box is under or found 
    bool found = false;
    bool under = false;
    while(!found){
        g_take_new_snapshot = true;
        while(g_cam1_data.models[0].pose.position.z < 0.1 && g_cam1_data.models[0].pose.position.z > -0.1){
            ros::spinOnce();
            ros::Duration(0.5).sleep();
        }
        ROS_INFO("found box");
        found = true;
        while(!under){
	    ros::spinOnce();
	    ros::Duration(0.5).sleep();
	    if(g_cam1_data.models[0].pose.position.z < 0.2 && g_cam1_data.models[0].pose.position.z > -0.2){
	        //box under camera
	        ROS_INFO("box is under");
		under = true;
		//conveyor stops
		conveyor_server.request.power = 0.0;
		conveyor_server.response.success = false;
		//make sure conveyor stops
		while(!conveyor_server.response.success){
		    ROS_WARN("not successful stopping conveyor yet...");
		    conveyor_client.call(conveyor_server);
		    ros::Duration(0.5).sleep();
		}
		//keep box under camera for 5 seconds 
		ros::Duration(5.0).sleep();
		//restart conveyor
		conveyor_server.request.power = 100.0;
		conveyor_server.response.success = false;
		while(!conveyor_server.response.success){
		    ROS_WARN("not successful start");
		    conveyor_client.call(conveyor_server);
		    ros::Duration(0.5).sleep();
		}
            }
        }
	g_take_new_snapshot = false;
    }
    drone_server.request.shipment_type = "dummy";
    drone_server.response.success = false;
    while(!drone_server.response.success){
	ROS_WARN("not successful starting drone yet...");
	drone_client.call(drone_server);
	ros::Duration(0.5).sleep();
    }
    ROS_INFO("got success response from drone service");
}



