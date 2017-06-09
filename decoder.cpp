#include <iostream>
#include <set>
#include <cstring>
#include <opencv2/opencv.hpp>
#include "decoder.h"
#include <unistd.h>
using namespace cv;
using namespace std;
Decoder::Decoder(PktQueue *queue)
{
    pqueue = queue;
    avcodec_register_all();
    pCodec = avcodec_find_decoder(codec_id);
    if (!pCodec)
        cout << "Codec not found" << endl;
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx)
        cout << "Could not allocate video codec context" << endl;
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
        cout << "Could not open codec\n"
             << endl;
    av_opt_set(pCodecCtx->priv_data, "tune", "zerolatency", 0);
    pFrame = av_frame_alloc();
    av_init_packet(&packet);
    //namedWindow("Chat1");
}
int Decoder::decode()
{
    set<string> uname; // user list
    Packet *pkt = pqueue->pop();
    uchar *buf;
    while (1)
    {
        if (pkt)
        {
            packet.data = pkt->data;
            packet.size = pkt->size;
            string st = pkt->name;
            if (uname.find(st) == uname.end())
            {
                uname.insert(st);
                namedWindow(st);
            }
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, &packet);
            if (ret < 0)
            {
                cout << "Decode Error" << endl;
                return ret;
            }
            if (got_picture)
            {
                if (first_time)
                {
                    first_time = 0;
                    buf = new uchar[pFrame->height * pFrame->width * 3 / 2];
                }
                for (int i = 0; i < pFrame->height; i++)
                    memcpy(buf + i * pFrame->width, pFrame->data[0] + pFrame->linesize[0] * i, pFrame->width);
                for (int i = 0; i < pFrame->height / 2; i++)
                    memcpy(buf + pFrame->height * pFrame->width + i * pFrame->width / 2, pFrame->data[1] + pFrame->linesize[1] * i, pFrame->width / 2);
                for (int i = 0; i < pFrame->height / 2; i++)
                    memcpy(buf + pFrame->height * pFrame->width * 5 / 4 + i * pFrame->width / 2, pFrame->data[2] + pFrame->linesize[2] * i, pFrame->width / 2);
                Mat yuvFrame(pFrame->height * 3 / 2, pFrame->width, CV_8UC1, buf);
                Mat rgbFrame;
                cvtColor(yuvFrame, rgbFrame, COLOR_YUV2BGR_I420);
                imshow(st, rgbFrame);
                if (waitKey(30) == 'q')
                    break;
                // delete[] buf;
            }
        }

        pkt = pqueue->pop();
    }
    return 0;
}

int Decoder::flush()
{
    packet.data = NULL;
    packet.size = 0;
    uchar *buf = new uchar[1000];
    while (1)
    {
        ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, &packet);
        if (ret < 0)
        {
            cout << "Decode Error" << endl;
            return ret;
        }
        if (!got_picture)
        {
            break;
        }
        else
        {
            //Y, U, V
            for (int i = 0; i < pFrame->height; i++)
                memcpy(buf + i * pFrame->width, pFrame->data[0] + pFrame->linesize[0] * i, pFrame->width);
            for (int i = 0; i < pFrame->height / 2; i++)
                memcpy(buf + pFrame->height * pFrame->width + i * pFrame->width / 2, pFrame->data[1] + pFrame->linesize[1] * i, pFrame->width / 2);
            for (int i = 0; i < pFrame->height / 2; i++)
                memcpy(buf + pFrame->height * pFrame->width * 5 / 4 + i * pFrame->width / 2, pFrame->data[2] + pFrame->linesize[2] * i, pFrame->width / 2);

            cout << "Flush Decoder: Succeed to decode 1 frame!" << endl;
        }
    }
    delete[] buf;
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    av_free(pCodecCtx);
    return 0;
}