#include <opencv2/opencv.hpp>
#include <iostream>

int main()
{
    std::string path = "images.png";
    cv::Mat image = cv::imread(path, cv::IMREAD_COLOR);
    cv::imshow("test", image);
    cv::waitKey(0);
    return 0;
}