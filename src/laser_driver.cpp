
#include <rpad/client.h>
#include <rpad_ros/logger.h>
#include <ros/ros.h>
#include <sensor_msgs/LaserScan.h>

int main(int argc, char **argv)
{
    ros::init(argc, argv, "rpad_driver");
    ros::NodeHandle node;
    ros::NodeHandle priv("~");

    rpad_ros::LogBridge log_bridge("rpad", node);

    ROS_INFO("Initializing rpad driver ...");

    std::string host = priv.param("host", std::string("192.168.11.11"));
    ROS_INFO("  host: %s", host.c_str());

    int port = priv.param("port", 1445);
    ROS_INFO("  port: %d", port);

    std::string frame_id = priv.param("frame_id", std::string("laser"));
    ROS_INFO("  frame_id: %s", frame_id.c_str());

    float min_range = priv.param("min_range", 0.0);
    ROS_INFO("  min_range: %f", min_range);

    float max_range = priv.param("max_range", 40.0);
    ROS_INFO("  max_range: %f", max_range);

    float rate = priv.param("rate", 20.0);
    ROS_INFO("  rate: %f", rate);

    ros::Publisher scan_pub = node.advertise<sensor_msgs::LaserScan>("scan", 10);

    rpad::Client client;

    ros::Time last_stamp = ros::Time::now();
    ros::Rate spin_rate(rate);
    while (ros::ok()) {
        ros::spinOnce();

        if (!client.connected()) {
            ROS_INFO("Connecting to device ...");
            if (!client.connect(host, port, 500)) {
                ROS_WARN("Failed to connect to device.");
                ros::Duration(1.0).sleep();
                continue;
            }
        }

        auto scan = client.getLaserScan();
        if (!scan || scan->points.size() < 2) {
            ROS_WARN("Failed to get laser scan.  Disconnecting...");
            client.disconnect();
            continue;
        }

        ros::Time stamp = ros::Time::now();
        double scan_time = (stamp - last_stamp).toSec();
        last_stamp = stamp;

        ROS_DEBUG("got scan with %zu points", scan->points.size());
        auto msg = boost::make_shared<sensor_msgs::LaserScan>();
        msg->header.stamp = stamp;
        msg->header.frame_id = frame_id;
        msg->angle_min = scan->points.front().angle;
        msg->angle_max = scan->points.back().angle;
        msg->angle_increment = (msg->angle_max - msg->angle_min) / (scan->points.size() - 1);
        msg->scan_time = scan_time;
        msg->time_increment = scan_time / (scan->points.size() - 1);
        msg->range_min = min_range;
        msg->range_max = max_range;
        msg->ranges.resize(scan->points.size());
        for (size_t i = 0; i < scan->points.size(); i++) {
            msg->ranges[i] = scan->points[i].distance;
        }
        scan_pub.publish(msg);
        spin_rate.sleep();
    }

    return 0;
}
