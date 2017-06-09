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
class Decoder
{

  private:
    AVCodec *pCodec;
    AVCodecContext *pCodecCtx = NULL;
    AVFrame *pFrame;
    AVPacket packet;
    int ret, got_picture;
    int y_size;
    AVCodecID codec_id = AV_CODEC_ID_H264;
    int first_time = 1;
    PktQueue *pqueue;
  public:
    Decoder(PktQueue *queue);
    int decode();
    int flush();
};