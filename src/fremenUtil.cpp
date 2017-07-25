#include <ros/ros.h>
#include <tf/transform_listener.h>
#include <sensor_msgs/point_cloud_conversion.h>
#include <std_msgs/Int32.h>
#include <sensor_msgs/PointCloud.h>
#include <sensor_msgs/PointCloud2.h>
#include <geometry_msgs/Point.h>
#include <std_msgs/ColorRGBA.h>
#include <std_msgs/Float32.h>
#include <visualization_msgs/MarkerArray.h>
#include <image_transport/image_transport.h>
#include <sensor_msgs/image_encodings.h>
#include <sensor_msgs/Image.h>

#include "CFremenGrid.h"
#include "CTimer.h"

#include "lifelong_benchmarking/Entropy.h"
#include "lifelong_benchmarking/SaveLoad.h"
#include "lifelong_benchmarking/AddView.h"
#include "lifelong_benchmarking/Visualize.h"
#include <std_msgs/String.h>

#define WW

#ifdef WW
#define MIN_X  -15.5
#define MIN_Y  -6.0
#define MIN_Z  -0.0
#define DIM_X 160
#define DIM_Y 120
#define DIM_Z 30
#endif

#ifdef BHAM_LARGE
#define MIN_X  -18.2
#define MIN_Y  -31.0
#define MIN_Z  -0.0
#define DIM_X 560
#define DIM_Y 990
#define DIM_Z 60
#endif

#ifdef BHAM_SMALL
#define MIN_X  -5.8
#define MIN_Y  -19.0
#define MIN_Z  -0.0
#define DIM_X 250
#define DIM_Y 500
#define DIM_Z 80
#endif

#ifdef UOL_SMALL
#define MIN_X  -7
#define MIN_Y  -5.6
#define MIN_Z  -0.0
#define DIM_X 280
#define DIM_Y 450
#define DIM_Z 80
#endif

#ifdef FLAT
#define MIN_X  -5.0
#define MIN_Y  -5.0
#define MIN_Z  -0.0
#define DIM_X  250
#define DIM_Y 200
#define DIM_Z 80
#endif

#ifdef FLAT_BIG
#define MIN_X  -5.0
#define MIN_Y  -5.0
#define MIN_Z  -0.0
#define DIM_X  260
#define DIM_Y 210
#define DIM_Z 80
#endif

#ifdef EMPTY
#define MIN_X  -18.2
#define MIN_Y  -31.0
#define MIN_Z  -0.0
#define DIM_X 560
#define DIM_Y 990
#define DIM_Z 60
#endif

#define CAMERA_RANGE 4.0

#define RESOLUTION 0.1

using namespace std;

bool debug = false;
int integrateMeasurements = 0;

CFremenGrid *grid;
tf::TransformListener *tf_listener;

ros::Publisher *octomap_pub_ptr, *estimate_pub_ptr,*clock_pub_ptr;
ros::Publisher retrieve_publisher;
ros::Publisher information_publisher;

bool projectGrid(lifelong_benchmarking::SaveLoad::Request  &req, lifelong_benchmarking::SaveLoad::Response &res)
{
    grid->load(req.filename.c_str());
    ROS_INFO("3D Grid of %ix%ix%i loaded !",grid->xDim,grid->yDim,grid->zDim);
    res.result = true;
    return true;
}

bool loadGrid(lifelong_benchmarking::SaveLoad::Request  &req, lifelong_benchmarking::SaveLoad::Response &res)
{
    grid->load(req.filename.c_str());
    char smart[1000];
    sprintf(smart,"%s.smart",req.filename.c_str());
    grid->saveSmart(smart);
    sprintf(smart,"%s.stupid",req.filename.c_str());
    grid->save(smart);
    ROS_INFO("3D Grid of %ix%ix%i loaded with %i/%i %.3f static cells!",grid->xDim,grid->yDim,grid->zDim,grid->numStatic(),grid->numCells,(float)grid->numStatic()/grid->numCells);
    for (float i = 0;i<0.5;i+=0.01){
	    int a = grid->numStatic(i);
	    printf("%.3f %i/%i %.3f\n",i,a,grid->numCells,(float)a/grid->numCells);
    }
    res.result = true;
    return true;
}

bool saveGrid(lifelong_benchmarking::SaveLoad::Request  &req, lifelong_benchmarking::SaveLoad::Response &res)
{
    grid->save(req.filename.c_str(), (bool) req.lossy,req.order);
    ROS_INFO("3D Grid of %ix%ix%i saved !",grid->xDim,grid->yDim,grid->zDim);
    return true;
}

void points(const sensor_msgs::PointCloud2ConstPtr& points2)
{

	CTimer timer;
    if (integrateMeasurements == 20){
		timer.reset();
		timer.start();
		sensor_msgs::PointCloud points1,points;
		sensor_msgs::convertPointCloud2ToPointCloud(*points2,points1);
		tf::StampedTransform st;
		try {
			tf_listener->waitForTransform("/map","/head_xtion_depth_optical_frame",points2->header.stamp, ros::Duration(1.0));
			tf_listener->lookupTransform("/map","/head_xtion_depth_optical_frame",points2->header.stamp,st);
		}
		catch (tf::TransformException ex) {
			ROS_ERROR("FreMEn map cound not incorporate the latest measurements %s",ex.what());
		        return;
		}
		printf("Transform arrived %i \n",timer.getTime());
		timer.reset();
		tf_listener->transformPointCloud("/map", points1,points);
		int size=points.points.size();
		std::cout << "There are " << size << " fields." << std::endl;
		std::cout << "Point " << st.getOrigin().x() << " " <<  st.getOrigin().y() << " " << st.getOrigin().z() << " " << std::endl;
		float x[size+1],y[size+1],z[size+1],d[size+1];
		int last = 0;
		for(unsigned int i = 0; i < size; i++){
			if (isnormal(points.points[i].x) > 0)
			{
				x[last] = points.points[i].x;
				y[last] = points.points[i].y;
				z[last] = points.points[i].z;
				d[last] = 1;
				last++;
			}
		}
		x[last] = st.getOrigin().x();
		y[last] = st.getOrigin().y();
		z[last] = st.getOrigin().z();
		printf("Point cloud processed %i \n",timer.getTime());
		timer.reset();
		grid->incorporate(x,y,z,d,last,points2->header.stamp.sec);
		printf("Grid updated %i \n",timer.getTime());
		integrateMeasurements--;
	}
	if (integrateMeasurements == 1)
	{
		integrateMeasurements--;
		lifelong_benchmarking::Visualize::Request req;
		req.green = req.blue = 0.0;
		req.red = req.alpha = 1.0;
	}
}

bool addView(lifelong_benchmarking::AddView::Request  &req, lifelong_benchmarking::AddView::Response &res)
{
	std_msgs::Float32 info;
	integrateMeasurements = 2;
	res.result = true;
	info.data = res.information = grid->getObtainedInformationLast();
	information_publisher.publish(info);
	return true;
}

bool addDepth(lifelong_benchmarking::AddView::Request  &req, lifelong_benchmarking::AddView::Response &res)
{
	std_msgs::Float32 info;
	integrateMeasurements = 3;
	res.result = true;
	info.data = res.information = grid->getObtainedInformationLast();
	information_publisher.publish(info);
	return true;
}

bool estimateEntropy(lifelong_benchmarking::Entropy::Request  &req, lifelong_benchmarking::Entropy::Response &res)
{
	grid->recalculate(req.t);
	res.value = grid->estimateInformation(req.x,req.y,req.z,req.r,req.t);
	res.obstacle = grid->getClosestObstacle(req.x,req.y,0.5,5.0);
	return true;
}

bool visualizeGrid(lifelong_benchmarking::Visualize::Request  &req, lifelong_benchmarking::Visualize::Response &res)
{
	//init visualization markers:
	visualization_msgs::Marker markers;
	geometry_msgs::Point cubeCenter;

	/// set color
	std_msgs::ColorRGBA m_color;
	m_color.r = req.red;
	m_color.g = req.green;
	m_color.b = req.blue;
	m_color.a = req.alpha;
	markers.color = m_color;

	/// set type, frame and size
	markers.header.frame_id = "/map";
	markers.header.stamp = ros::Time::now();
	markers.ns = req.name;
	markers.action = visualization_msgs::Marker::ADD;
	markers.type = visualization_msgs::Marker::CUBE_LIST;
	markers.scale.x = RESOLUTION;
	markers.scale.y = RESOLUTION;
	markers.scale.z = RESOLUTION;
	markers.points.clear();

	// prepare to iterate over the entire grid
	float resolution = grid->resolution;
	float minX = grid->oX;
	float minY = grid->oY;
	float minZ = grid->oZ;
	float maxX = minX+grid->xDim*grid->resolution-3*grid->resolution/4;
	float maxY = minY+grid->yDim*grid->resolution-3*grid->resolution/4;
	float maxZ = 2.1;//minZ+grid->zDim*grid->resolution-3*grid->resolution/4;
	int cnt = 0;
	int cells = 0;
	float estimate,minP,maxP;
	minP = req.minProbability;
	maxP = req.maxProbability;
	if (req.stamp != 0 && req.type == 1) grid->recalculate(req.stamp);

	//iterate over the cells' probabilities
	for(float z = minZ;z<maxZ;z+=resolution){
		for(float y = minY;y<maxY;y+=resolution){
			for(float x = minX;x<maxX;x+=resolution){
				if (req.type == 0) estimate = grid->retrieve(cnt);			//short-term memory grid
				if (req.type == 1) estimate = grid->estimate(cnt,0);			//long-term memory grid
				if (req.type == 2) estimate = grid->aux[cnt];				//auxiliary grid
				if (req.type == 3) estimate = grid->getDominant(cnt,req.period);	//dominant frequency amplitude

				if(estimate > minP && estimate < maxP)
				{
                    if(req.set_color == 0)
                    {
                        m_color.r = 0.0;
                        m_color.b = z/maxZ;
                        m_color.g = 1.0 - m_color.b;
                        m_color.a = 0.8;
                    }
					cubeCenter.x = x;
					cubeCenter.y = y;
					cubeCenter.z = z;
					markers.points.push_back(cubeCenter);
					markers.colors.push_back(m_color);
					cells++;
				}
				cnt++;
			}
		}
	}

	//publish results
	retrieve_publisher.publish(markers);
	res.number = cells;
	return true;
}


void imageCallback(const sensor_msgs::ImageConstPtr& msg)
{
	float depth = msg->data[640*480+640]+256*msg->data[640*480+640+1];
	int len =  msg->height*msg->width;
	float vx = 1/570.0;
	float vy = 1/570.0;
	float cx = -320.5;
	float cy = -240.5;
	int width = msg->width;
	int height = msg->height;
	float fx = (1+cx)*vx;
	float fy = (1+cy)*vy;
	float lx = (width+cx)*vx;
	float ly = (height+cy)*vy;
	float x[len+1];
	float y[len+1];
	float z[len+1];
	float d[len+1];
	float di,psi,phi,phiPtu,psiPtu,xPtu,yPtu,zPtu,ix,iy,iz;
	int cnt = 0;
	di=psi=phi=phiPtu=psiPtu=xPtu=yPtu=zPtu=0;
	if (integrateMeasurements == 3)
	{
		tf::StampedTransform st;
		try {
			tf_listener->waitForTransform("/map","/head_xtion_depth_optical_frame",msg->header.stamp, ros::Duration(0.5));
			tf_listener->lookupTransform("/map","/head_xtion_depth_optical_frame",msg->header.stamp,st);
		}
		catch (tf::TransformException ex) {
			ROS_ERROR("FreMEn map cound not incorporate the latest depth map %s",ex.what());
			return;
		}
		CTimer timer;
		timer.reset();
		timer.start();
		x[len] = xPtu = st.getOrigin().x();
		y[len] = yPtu = st.getOrigin().y();
		z[len] = zPtu = st.getOrigin().z();
		double a,b,c;
		tf::Matrix3x3  rot = st.getBasis();
		rot.getEulerYPR(a,b,c,1);
		printf("%.3f %.3f %.3f\n",a,b,c);
		psi = -M_PI/2-c;
		phi = a-c-psi;
		for (float h = fy;h<ly;h+=vy)
		{
			for (float w = fx;w<lx;w+=vx)
			{
				di = (msg->data[cnt*2]+256*msg->data[cnt*2+1])/1000.0;
				//printf("%f.3\n",di);
				d[cnt] = 1;
                if (di < 0.05 || di > CAMERA_RANGE){
					di = CAMERA_RANGE;
					d[cnt] = 0;
				}
				ix = di*(cos(psi)-sin(psi)*h);
				iy = -w*di;
				iz = -di*(sin(psi)+cos(psi)*h);
				x[cnt] = cos(phi)*ix-sin(phi)*iy+xPtu;
				y[cnt] = sin(phi)*ix+cos(phi)*iy+yPtu;
				z[cnt] = iz+zPtu;
				cnt++;
			}
		}
		printf("Depth image to point cloud took %i ms,",timer.getTime());
		grid->incorporate(x,y,z,d,len,msg->header.stamp.sec);
		//printf("Grid updated %i \n",timer.getTime());
		integrateMeasurements=0;
		//printf("XXX: %i %i %i %i %s %.3f\n",len,cnt,msg->width,msg->height,msg->encoding.c_str(),depth);
	}
}

int main(int argc,char *argv[])
{
    ros::init(argc, argv, "fremenutil");
    ros::NodeHandle n;
    grid = new CFremenGrid(MIN_X,MIN_Y,MIN_Z,DIM_X,DIM_Y,DIM_Z,RESOLUTION);

    n.setParam("/fremenUtil/minX",MIN_X);
    n.setParam("/fremenUtil/minY",MIN_Y);
    n.setParam("/fremenUtil/minZ",MIN_Z);
    n.setParam("/fremenUtil/dimX",DIM_X);
    n.setParam("/fremenUtil/dimY",DIM_Y);
    n.setParam("/fremenUtil/dimZ",DIM_Z);
    n.setParam("/fremenUtil/resolution",RESOLUTION);

    tf_listener    = new tf::TransformListener();
    image_transport::ImageTransport imageTransporter(n);

    ros::Time now = ros::Time(0);
    tf_listener->waitForTransform("/head_xtion_depth_optical_frame","/map",now, ros::Duration(3.0));
    ROS_INFO("Fremen grid start");

    //Subscribers:
    ros::Subscriber point_subscriber = n.subscribe<sensor_msgs::PointCloud2> ("/head_xtion/depth/points",  1000, points);
    image_transport::Subscriber image_subscriber = imageTransporter.subscribe("/head_xtion/depth/image_raw", 1, imageCallback);
    retrieve_publisher = n.advertise<visualization_msgs::Marker>("/fremenUtil/visCells", 100);
    information_publisher  = n.advertise<std_msgs::Float32>("/fremenUtil/obtainedInformation", 100);

    //Services:
    ros::ServiceServer retrieve_service = n.advertiseService("/fremenUtil/visualize", visualizeGrid);
    ros::ServiceServer information_gain = n.advertiseService("/fremenUtil/entropy", estimateEntropy);
//    ros::ServiceServer add_service = n.advertiseService("/fremenUtil/measure", addView);
    ros::ServiceServer depth_service = n.advertiseService("/fremenUtil/depth", addDepth);
    ros::ServiceServer save_service = n.advertiseService("/fremenUtil/save", saveGrid);
    ros::ServiceServer load_service = n.advertiseService("/fremenUtil/load", loadGrid);

    ros::spin();
    delete tf_listener;
    delete grid;
    return 0;
}
