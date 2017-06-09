#include <portaudio.h>
#include <iostream>
#define __STDC_CONSTANT_MACROS
#ifdef _WIN32
//Windows
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
};
#endif
#endif
#include <unistd.h> 
#include "common.h"
using namespace std;
#include "audiodecoder.cpp"
class AudioEncoder
{
  private:
    PaStream *stream;
    static AVPacket pkt;
    static AVFrame *pFrame;
    static AVCodecContext *pCodecCtx;
    static AVCodec *codec;
    static int i;
    PktQueue *pqueue;

  public:
    AudioEncoder(PktQueue *queue);
    int Encode(uint8_t *rawData);
    bool StartEncoder(int);
    static int EncoderCallback(const void *input,
                               void *output,
                               unsigned long frameCount,
                               const PaStreamCallbackTimeInfo *timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void *userData);
};

AudioEncoder::AudioEncoder(PktQueue *queue)
{
    pqueue = queue;
    avcodec_register_all();
    codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (codec == nullptr)
        cout << "avcodec_find_encoder: ERROR\n"
             << endl;
    pCodecCtx = avcodec_alloc_context3(codec);
    pCodecCtx->bit_rate = 64000;
    pCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    pCodecCtx->sample_rate = 44100;
    pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
    pCodecCtx->channels = av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);
    ;
    pCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    //pCodecCtx->profile = FF_PROFILE_AAC_MAIN;
    //pCodecCtx->time_base = (AVRational){1, sampleRate};
    //pCodecCtx->time_base.num = 1;
    //pCodecCtx->time_base.den = 44100;
    av_opt_set(pCodecCtx->priv_data, "tune", "zerolatency", 0);
    int erro = avcodec_open2(pCodecCtx, codec, nullptr);
    if (erro < 0)
        cout << "avcodec_open: ERROR  " << erro << endl;
    pFrame = av_frame_alloc();
    pFrame->nb_samples = pCodecCtx->frame_size;
    pFrame->format = pCodecCtx->sample_fmt;
}
bool AudioEncoder::StartEncoder(int deviceIndex)
{
    PaError err = Pa_Initialize();
    if (err != paNoError)
        cout << "ERROR: Pa_Initialize returned 0x%x\n"
             << endl;
    PaStreamParameters inputParameters;
    if (deviceIndex <= 0)
    {
        deviceIndex = Pa_GetDefaultInputDevice();
    }
    if (deviceIndex == paNoDevice)
    {
        cout << "no defalut device" << endl;
        exit(1);
    }
    inputParameters.device = deviceIndex;
    inputParameters.channelCount = 2;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(deviceIndex)->defaultLowOutputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;
    err = Pa_OpenStream(&stream, &inputParameters, nullptr, 44100, 1024, paClipOff, EncoderCallback, pqueue);
    if (err != paNoError)
    {
        return false;
    }
    err = Pa_StartStream(stream);
    if (err != paNoError)
    {
        return false;
    }

    int cou = 0;
    while (1 == Pa_IsStreamActive(stream))
    {
        cou++;
        sleep(1);
        if (cou == 15)
            Pa_CloseStream(stream);
    }
    Pa_CloseStream(stream);
    return true;
}
int AudioEncoder::EncoderCallback(const void *input,
                                  void *output,
                                  unsigned long frameCount,
                                  const PaStreamCallbackTimeInfo *timeInfo,
                                  PaStreamCallbackFlags statusFlags,
                                  void *userData)
{
    PktQueue *queue = (PktQueue *)userData;
    av_init_packet(&pkt);
    int size = av_samples_get_buffer_size(nullptr, pCodecCtx->channels, pCodecCtx->frame_size, pCodecCtx->sample_fmt, 1);
    uint8_t *frame_buf = (uint8_t *)av_malloc(size);
    avcodec_fill_audio_frame(pFrame, pCodecCtx->channels, pCodecCtx->sample_fmt, frame_buf, size, 0);
    if (input != NULL)
    {
        long *tempI = (long *)input;
        long *temp = (long *)frame_buf;
        for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex)
            *temp++ = *tempI++;
    }
    pFrame->data[0] = frame_buf;
    pFrame->pts = i++;
    int got_frame = 0;
    int ret = avcodec_encode_audio2(pCodecCtx, &pkt, pFrame, &got_frame);
    if (ret < 0)
    {
        cout << "Failed to encode!" << endl;
        return -1;
    }
    if (got_frame == 1)
    {
        cout << "Succeed to encode 1 frame! \tsize:%5d\n"
             << pkt.size << endl;
        if (queue)
        {
            Packet *packet = new Packet;
            packet->size = pkt.size;
            packet->data = new uchar[pkt.size];
            packet->name = nullptr;
            memcpy(packet->data, pkt.data, pkt.size);
            queue->push(packet);
        }
        av_free_packet(&pkt);
    }
    return paContinue;
}
AVPacket AudioEncoder::pkt;
AVFrame *AudioEncoder::pFrame;
AVCodecContext *AudioEncoder::pCodecCtx;
AVCodec *AudioEncoder::codec;
int AudioEncoder::i = 0;
/*
int AudioEncoder::flush()
{
    int got_frame;
    while (1)
    {
        pkt.data = nullptr;
        pkt.size = 0;
        av_init_packet(&pkt);
        ret = avcodec_encode_audio2(pCodecCtx, &pkt,
                                    nullptr, &got_frame);
        av_frame_free(nullptr);
        if (ret < 0)
            break;
        if (!got_frame)
        {
            ret = 0;
            break;
        }
        cout << "Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n"
             << pkt.size << endl;
    }
    avcodec_close(pCodecCtx);
    av_free(pFrame);
    //av_free(frame_buf);
    return 0;
}*/

int main()
{   PktQueue q;
    AudioEncoder aud(&q);
   AudioDecoder aaa(&q);
    aud.StartEncoder(0);
   aaa.StartDecode(0);
    return 0;
}