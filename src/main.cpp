#include <opencv2/opencv.hpp>

int main()
{
    cv::VideoCapture cap(0); // 0 = default webcam
    cv::Mat frame;

    while(true)
    {
        cap >> frame;
        cv::imshow("Webcam", frame);

        if(cv::waitKey(1) == 27) // press ESC to quit
            break;
    }

    return 0;
}
