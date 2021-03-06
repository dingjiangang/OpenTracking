#include <opencv2/opencv.hpp>
#include "tld_utils.h"
#include "mropencv.h"
#include <iostream>
#include <sstream>
#include "TLD.h"
#include <stdio.h>
using namespace cv;
using namespace std;
//Global variables
Rect box;
bool drawing_box = false;
bool gotBB = false;
bool tl = true;
bool rep = false;
bool fromfile=false;
string video;
//读取记录bounding box的文件，获得bounding box的四个参数：左上角坐标x，y和宽高  
//如在\datasets\06_car\init.txt中：记录了初始目标的bounding box，内容如下 
//142,125,232,164     
void readBB(char* file){
  ifstream bb_file (file);//以输入方式打开文件 
  string line;
  getline(bb_file,line); //将输入流is中读到的字符存入str中，终结符默认为 '\n'（换行符）   
  istringstream linestream(line);//istringstream对象可以绑定一行字符串，然后以空格为分隔符把该行分隔开来。  
  string x1,y1,x2,y2;
  getline (linestream,x1, ',');//将输入流is中读到的字符存入str中，直到遇到终结符delim才结束。  
  getline (linestream,y1, ',');
  getline (linestream,x2, ',');
  getline (linestream,y2, ',');
  int x = atoi(x1.c_str());// = (int)file["bb_x"];//atoi 功 能： 把字符串转换成整型数   
  int y = atoi(y1.c_str());// = (int)file["bb_y"];
  int w = atoi(x2.c_str())-x;// = (int)file["bb_w"];
  int h = atoi(y2.c_str())-y;// = (int)file["bb_h"];
  box = Rect(x,y,w,h);
}
//bounding box mouse callback//鼠标的响应就是得到目标区域的范围，用鼠标选中bounding box。   
void mouseHandler(int event, int x, int y, int flags, void *param){
  switch( event ){
  case CV_EVENT_MOUSEMOVE:
    if (drawing_box){
        box.width = x-box.x;
        box.height = y-box.y;
    }
    break;
  case CV_EVENT_LBUTTONDOWN:
    drawing_box = true;
    box = Rect( x, y, 0, 0 );
    break;
  case CV_EVENT_LBUTTONUP:
    drawing_box = false;
    if( box.width < 0 ){
        box.x += box.width;
        box.width *= -1;
    }
    if( box.height < 0 ){
        box.y += box.height;
        box.height *= -1;
    }
    gotBB = true;//已经获得bounding box  
    break;
  }
}

void print_help(char** argv){
  printf("use:\n     %s -p /path/parameters.yml\n",argv[0]);
  printf("-s    source video\n-b        bounding box file\n-tl  track and learn\n-r     repeat\n");
}
//分析运行程序时的命令行参数   
void read_options(int argc, char** argv,VideoCapture& capture,FileStorage &fs){
  for (int i=0;i<argc;i++){
      if (strcmp(argv[i],"-b")==0){
          if (argc>i){
              readBB(argv[i+1]);//是否指定初始的bounding box  
              gotBB = true;
          }
          else
            print_help(argv);
      }
      if (strcmp(argv[i],"-s")==0){//从视频文件中读取  
          if (argc>i){
              video = string(argv[i+1]);
              capture.open(video);
              fromfile = true;
          }
          else
            print_help(argv);

      }
      if (strcmp(argv[i],"-p")==0){//读取参数文件parameters.yml  
          if (argc>i){
              fs.open(argv[i+1], FileStorage::READ); //FileStorage类的读取方式可以是：FileStorage fs(".\\parameters.yml", FileStorage::READ);  
          }
          else
            print_help(argv);
      }
      if (strcmp(argv[i],"-no_tl")==0){
          tl = false;
      }
      if (strcmp(argv[i],"-r")==0){
          rep = true;
      }
  }
}
//对起始帧进行初始化工作，然后逐帧读入图片序列，进行算法处理。
int main(int argc, char * argv[]){
  VideoCapture capture;
  capture.open(0);// open the default camera
  //OpenCV的C++接口中，用于保存图像的imwrite只能保存整数数据，且需作为图像格式。当需要保存浮  
  //点数据或XML/YML文件时，OpenCV的C语言接口提供了cvSave函数，但这一函数在C++接口中已经被删除。  
  //取而代之的是FileStorage类。   
  FileStorage fs;
  //直接固定参数
  argc = 3;
  argv[0] = "./run_tld";
  argv[1] = "-p";
  argv[2] = "parameters.yml";
  argv[3] = "-s";
  argv[4] = "../datasets/06_car/car.mpg";
  argv[5] = "-b";
  argv[6] = "../datasets/06_car/init.txt";

  //Read options
  read_options(argc,argv,capture,fs);//分析命令行参数  
  //Init camera
  if (!capture.isOpened())
  {
	cout << "capture device failed to open!" << endl;
    return 1;
  }
  //Register mouse callback to draw the bounding box
  cvNamedWindow("TLD",CV_WINDOW_AUTOSIZE);
  cvSetMouseCallback( "TLD", mouseHandler, NULL );//设置鼠标回调
  //TLD framework
  TLD tld;
  //Read parameters file
  tld.read(fs.getFirstTopLevelNode());
  Mat frame;
  Mat last_gray;
  Mat first;
  if (fromfile){//如果指定为从文件读取  
      capture >> frame;//读当前帧  
      cvtColor(frame, last_gray, CV_RGB2GRAY);//转换为灰度图像  
      frame.copyTo(first);//拷贝作为第一帧  
  }else{//如果为读取摄像头，则设置获取的图像大小为320x240   
      capture.set(CV_CAP_PROP_FRAME_WIDTH,340);
      capture.set(CV_CAP_PROP_FRAME_HEIGHT,240);
  }

  ///Initialization
GETBOUNDINGBOX://标号：获取bounding box   
  while(!gotBB)
  {
    if (!fromfile){
      capture >> frame;
    }
    else
      first.copyTo(frame);
    cvtColor(frame, last_gray, CV_RGB2GRAY);
    drawBox(frame,box);//把bounding box 画出来  
    imshow("TLD", frame);
    if (cvWaitKey(33) == 'q')
	    return 0;
  }
  if (min(box.width,box.height)<(int)fs.getFirstTopLevelNode()["min_win"]){
      cout << "Bounding box too small, try again." << endl;
      gotBB = false;
      goto GETBOUNDINGBOX;
  }
  //Remove callback
  cvSetMouseCallback( "TLD", NULL, NULL ); //如果已经获得第一帧用户框定的box了，就取消
  printf("Initial Bounding Box = x:%d y:%d h:%d w:%d\n",box.x,box.y,box.width,box.height); 
  //Output file
  FILE  *bb_file = fopen("bounding_boxes.txt","w");
  //-------------- TLD initialization -----------------------
  tld.init(last_gray,box,bb_file);// 第一帧灰度图像last_gray，框出的包围框初始目标位置存储在box，结果存储文件bb_file

  ///Run-time
  Mat current_gray;
  BoundingBox pbox;
  vector<Point2f> pts1; //
  vector<Point2f> pts2; // 
  bool status=true;//记录跟踪成功与否的状态 lastbox been found  
  int frames = 1;//记录已过去帧数
  int detections = 1;//记录成功检测到的目标box数目 
REPEAT://
  while(capture.read(frame)){
    //get frame
    cvtColor(frame, current_gray, CV_RGB2GRAY);//灰度图，酱紫就舍弃颜色信息，不开心
    //--------------------------Process Frame----------------------
	//last_gray和current_gray分别是上一帧和当前帧的灰度图；pts1和pts2是跟踪点的坐标，
	//初始化为空，在函数内部实现赋值和应用；pbox是上一帧跟踪的结果；
	//status是bool值用于表示上一帧是否有跟踪到窗口；
	//tl个人理解t对应track，l对应learn，也即tl是表示是否进行track和learn的bool标记；
	//bb_file是存放跟踪结果的文件的路径。
	//这个函数依次执行以下四个模块：跟踪模块、检测模块、综合模块、学习模块。
    tld.processFrame(last_gray,current_gray,pts1,pts2,pbox,status,tl,bb_file);
    //Draw Points
    if (status){ //如果跟踪成功  
      drawPoints(frame,pts1);
      drawPoints(frame,pts2,Scalar(0,255,0)); //当前的特征点用蓝色点表示  
      drawBox(frame,pbox);
      detections++;
    }
    //Display
    imshow("TLD", frame);
    //swap points and images
    swap(last_gray,current_gray);//STL函数swap()用来交换两对象的值。其泛型化版本定义
    pts1.clear();
    pts2.clear();
    frames++;
    printf("Detection rate: %d/%d\n",detections,frames);
    if (cvWaitKey(33) == 'q')
      break;
  }
  if (rep){//rep 控制循环
    rep = false;
    tl = false;
    fclose(bb_file);
    bb_file = fopen("final_detector.txt","w");
    //capture.set(CV_CAP_PROP_POS_AVI_RATIO,0);
    capture.release();
    capture.open(video);
    goto REPEAT;
  }
  fclose(bb_file);
  return 0;
}
