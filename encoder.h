
#ifndef __CODE_H__
#define __CODE_H__
#define __STDC_CONSTANT_MACROS
#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
};
#endif
#include "common.h"
#endif
class H264Encoder
{

  private:
    AVCodec *pCodec;
    AVCodecContext *pCodecCtx;
    int i, ret, got_output;
    AVFrame *pFrame;
    AVPacket pkt;
    int y_size;
    AVCodecID codec_id = AV_CODEC_ID_H264;
    char filename_out[10] = "ds.h264";
    int in_w, in_h;
    Packet *packet = NULL;
    PktQueue *pqueue = NULL;
  public:
     H264Encoder(int w, int h, PktQueue *queue);
     int Encode(uchar *data);
     int flush();
   
};