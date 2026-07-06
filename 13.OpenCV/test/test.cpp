#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;

#define ERODE
int main() {
    // 读取图片
    Mat imgSource = imread("./img/input.jpg");
    if (imgSource.empty()) {
        cout << "无法读取图片！" << endl;
        return -1;
    }
 
#ifdef ERODE
    // 定义腐蚀内核
    Mat element = getStructuringElement(MORPH_RECT, Size(15, 15));
    // 执行腐蚀操作
    Mat imgDest;
    erode(imgSource, imgDest, element);
    imwrite("./img/output.jpg", imgDest);
#elif defined(BLUR)
    // 执行模糊操作
    Mat imgDest;
    blur(imgSource, imgDest, Size(20, 20));
    imwrite("./img/output.jpg", imgDest);
#elif defined(DILATE)
    // 定义膨胀内核
    Mat element = getStructuringElement(MORPH_RECT, Size(15, 15));
    Mat imgDest;
    // 执行膨胀操作
    dilate(imgSource, imgDest, element);
    imwrite("./img/output.jpg", imgDest);
#elif defined(CANNY)
    // 转换为灰度图像
    Mat grayImage;
    cvtColor(imgSource, grayImage, COLOR_BGR2GRAY);
    // 降噪
    Mat edge;
    blur(grayImage, edge, Size(3, 3));
    // 执行 Canny 边缘检测
    Canny(edge, edge, 50, 150, 3);
    imwrite("./img/output.jpg", edge);
#endif
  
    return 0;
}