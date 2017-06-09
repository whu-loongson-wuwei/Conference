#include <portaudio.h>
class AudioDecoder
{
  private:
    PaStream *pa_stream;
    PktQueue *pqueue;
    static AVPacket packet;
    static AVFrame *pFrame;
    static AVCodecContext *pCodec;
    static AVCodec *codec;
    static int i;

  public:
    AudioDecoder(PktQueue *queue);
    bool StartDecode(int);
    static int PlayCallback(const void *input,
                            void *output,
                            unsigned long frameCount,
                            const PaStreamCallbackTimeInfo *timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData);
};
AudioDecoder::AudioDecoder(PktQueue *queue)
{   pqueue =queue;
    avcodec_register_all();
    codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    if (!codec)
        cout << "Codec not found" << endl;
    pCodec = avcodec_alloc_context3(codec);
    if (!pCodec)
        cout << "Could not allocate video codec context" << endl;
    if (avcodec_open2(pCodec, codec, NULL) < 0)
        cout << "Could not open codec\n"
             << endl;
    av_opt_set(pCodec->priv_data, "tune", "zerolatency", 0);
    pFrame = av_frame_alloc();
    av_init_packet(&packet);
}
bool AudioDecoder::StartDecode(int deviceIndex)
{

    PaError err = Pa_Initialize();
    if (err != paNoError)
        cout << "ERROR: Pa_Initialize returned 0x%x\n"
             << endl;
    PaStreamParameters outputParameters;
    if (deviceIndex <= 0)
    {
        deviceIndex = Pa_GetDefaultOutputDevice();
    }
    outputParameters.device = deviceIndex;
    outputParameters.channelCount = 2;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(deviceIndex)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    err = Pa_OpenStream(&pa_stream, &outputParameters, NULL, 44100, 1024, paClipOff, PlayCallback, pqueue);
    if (err != paNoError)
    {
        return false;
    }
    err = Pa_StartStream(pa_stream);
    if (err != paNoError)
    {
        return false;
    }
    int cou = 0;
    while (1 == Pa_IsStreamActive(pa_stream))
    {
        sleep(1);
        cou++;
        if (cou == 15)
            Pa_CloseStream(pa_stream);
    }
    return true;
}
int AudioDecoder::PlayCallback(const void *input,
                                  void *output,
                                  unsigned long frameCount,
                                  const PaStreamCallbackTimeInfo *timeInfo,
                                  PaStreamCallbackFlags statusFlags,
                                  void *userData)
{
    PktQueue *queue = (PktQueue *)userData;
    if (queue)
    {   cout << "fdddgd"<<endl;
        Packet *pkt = queue->pop();
        cout << queue->size()<<endl;
        cout << "fdddgd11"<<endl;
        if (pkt)
        {   
            packet.data = pkt->data;
            packet.size = pkt->size;
            int got_frame = 0;
            int ret = avcodec_decode_audio4(pCodec,pFrame,&got_frame ,&packet);
            if (ret < 0)
            {
                cout << "Failed to decode!" << endl;
                exit(-1);
            }
            long *tempO = (long *)output;
            long *temp = (long *)pFrame->data[0];
            for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex)
                *tempO++ = *temp++;
        }
    }
    return paContinue;
}
AVPacket AudioDecoder::packet;
AVFrame *AudioDecoder::pFrame;
AVCodecContext *AudioDecoder::pCodec;
AVCodec *AudioDecoder::codec;
int AudioDecoder::i = 0;