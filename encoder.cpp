#include <iostream>
#include <string.h>
#include "encoder.h"
using namespace std;
H264Encoder::H264Encoder(int w, int h, PktQueue *queue)
{
    pqueue = queue;
    in_w = w;
    in_h = h;
    avcodec_register_all();
    pCodec = avcodec_find_encoder(codec_id);
    if (!pCodec)
        cout << "Codec not found" << endl;
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx)
        cout << "Could not allocate video codec context" << endl;
    pCodecCtx->bit_rate = 400000;
    pCodecCtx->width = in_w;
    pCodecCtx->height = in_h;
    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;
    pCodecCtx->gop_size = 10;
    pCodecCtx->max_b_frames = 1;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    av_opt_set(pCodecCtx->priv_data, "preset", "superfast", 0);
    av_opt_set(pCodecCtx->priv_data, "tune", "zerolatency", 0);
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
        cout << "Could not open codec" << endl;
    pFrame = av_frame_alloc();
    if (!pFrame)
        cout << "Could not allocate video frame" << endl;
    pFrame->format = pCodecCtx->pix_fmt;
    pFrame->width = pCodecCtx->width;
    pFrame->height = pCodecCtx->height;
    ret = av_image_alloc(pFrame->data, pFrame->linesize, pCodecCtx->width, pCodecCtx->height,
                         pCodecCtx->pix_fmt, 16);
    if (ret < 0)
        cout << "Could not allocate raw picture buffer";
    y_size = pCodecCtx->width * pCodecCtx->height;
}

int H264Encoder::Encode(uchar* data)
{
    av_init_packet(&pkt);
    pkt.data = NULL; // packet data will be allocated by the encoder
    pkt.size = 0;
    if(i % 50 == 0)
    {
    pFrame->pict_type = AV_PICTURE_TYPE_I;
    pFrame->key_frame = 1;
    }
    pFrame->pts = i++;
    memcpy(pFrame->data[0], data, in_w * in_h);
    memcpy(pFrame->data[1], data + in_w * in_h, in_w * in_h / 4);
    memcpy(pFrame->data[2], data + in_w * in_h * 5 / 4, in_w * in_h / 4);
    ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_output);
    if (ret < 0)
    {
        cout << "Error encoding frame" << endl;
        return -1;
    }
    if (got_output)
    {
        //cout << "Succeed to encode frame " << pkt.size << endl;
        packet = new Packet;
        packet->size = pkt.size;
        packet->data = new uchar[pkt.size];
        packet->name = nullptr;
        memcpy(packet->data, pkt.data, pkt.size);
        pqueue->push(packet);
        av_free_packet(&pkt);
    }
    return 0;
}
int H264Encoder::flush()
{
    for (got_output = 1; got_output; i++)
    {
        ret = avcodec_encode_video2(pCodecCtx, &pkt, NULL, &got_output);
        if (ret < 0)
        {
            cout << "Error encoding frame" << endl;
            return -1;
        }
        if (got_output)
        {
            //cout << "Flush Encoder: Succeed to encode 1 frame " << pkt.size << endl;
            packet = new Packet;
            packet->size = pkt.size;
            packet->data = new uchar[pkt.size];
            memcpy(packet->data, pkt.data, pkt.size);
            pqueue->push(packet);
            av_free_packet(&pkt);
        }
    }
    avcodec_close(pCodecCtx);
    av_free(pCodecCtx);
    av_freep(&pFrame->data[0]);
    av_frame_free(&pFrame);
    return 0;
}