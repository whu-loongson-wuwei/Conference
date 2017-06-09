#include <iostream>
#include <opencv2/opencv.hpp>
#include "encoder.h"
#include "decoder.h"
using namespace cv;
using namespace std;
#include "client.hpp"
int main()
{

    VideoCapture capture(0); //从摄像头读入图像
    Mat Frame;
    capture >> Frame;
    int h =  Frame.rows/2;
    int w =  Frame.cols/2;
    capture.set(CAP_PROP_FRAME_WIDTH , w);
    capture.set(CAP_PROP_FRAME_HEIGHT , h);
    PktQueue queue;
    H264Encoder *enc = new H264Encoder(w, h, &queue);
    //Decoder *dec = new Decoder(&queue);
    Sender sen(&queue);
    sen.Send();
   // namedWindow("Chat");
    int c = 0;
    while(1)
    {
        Mat rgbFrame;
        Mat yuvFrame;
        capture >> rgbFrame;
        cvtColor(rgbFrame, yuvFrame, COLOR_BGR2YUV_I420);
        enc->Encode(yuvFrame.data);
        //imshow("Chat", rgbFrame);
        if (waitKey(30) == 'q')
          break;
    }
    enc->flush();
    return 0;
}
