#include <opencv2/opencv.hpp>

int main()
{
    // Load an image
    cv::Mat img = cv::imread("image.jpg");

    // Show it in a window
    cv::imshow("Window", img);
    cv::waitKey(0);

    return 0;
}
