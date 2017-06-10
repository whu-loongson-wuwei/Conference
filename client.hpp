#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "common.h"
#include <errno.h>
#include <pthread.h>
class Sender
{
  public:
    static int s;
    static PktQueue *pqueue;
    static struct sockaddr_in serv;

  public:
    Sender(PktQueue *queue)
    {
        s = socket(AF_INET, SOCK_DGRAM, 0);
        bzero(&serv, sizeof(serv));
        serv.sin_family = AF_INET;
        serv.sin_addr.s_addr = inet_addr(GROUP); // targe IP
        serv.sin_port = htons(PORT);
        uchar loop = 0;
        //setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
        pqueue = queue;
    }
    void Send()
    {
        pthread_t tid;
        pthread_create(&tid, NULL, send, NULL);
    }
    static void *send(void *)
    {
        //extern int errno;
        uchar *buf = new uchar[PKT_SIZE + 9];
        socklen_t addr_len = sizeof(serv);
        Packet *pkt = pqueue->pop();
        int seq = 1;
        int d;

        Header *pkt_id = new Header;
        pkt_id->length = 0;
        pkt_id->id = 1;
        memcpy(pkt_id->name, "wuwei", 6);

        int beat = 0;
        while (1)
        {
            if (pkt)
            {
                if (beat % 50 == 0)
                {
                    sendto(s, pkt_id, 20, 0, (struct sockaddr *)&serv, addr_len);
                    beat = 0;
                    cout << "beat"<<endl;
                }
                beat++;
                Header *h = (Header *)buf;
                h->length = pkt->size;
                h->seq = seq++;
                h->id = 2;
                cout << "[send]" << h->seq << "----->" << h->length << endl;
                if (pkt->size <= PKT_SIZE)
                {

                    memcpy(buf + 9, pkt->data, pkt->size);
                    h->inner_seq = -pkt->size;
                    sendto(s, buf, pkt->size + 9, 0, (struct sockaddr *)&serv, addr_len);
                }
                else
                {
                    int inner_seq = 1;
                    int c = pkt->size / PKT_SIZE;
                    if (c * PKT_SIZE == pkt->size) //整除最后一个设为-PKT_SIZE
                        c -= 1;
                    for (int i = 0; i < c; ++i)
                    {
                        memcpy(buf + 9, pkt->data + i * PKT_SIZE, PKT_SIZE);
                        h->inner_seq = inner_seq++;
                        sendto(s, buf, PKT_SIZE + 9, 0, (struct sockaddr *)&serv, addr_len);
                    }
                    memcpy(buf + 9, pkt->data + c * PKT_SIZE, pkt->size - c * PKT_SIZE);
                    h->inner_seq = -(pkt->size - c * PKT_SIZE);
                    sendto(s, buf, pkt->size - c * PKT_SIZE + 9, 0, (struct sockaddr *)&serv, addr_len);
                }
            }
            pkt = pqueue->pop();
        }
        delete[] buf;
    }
    void Cclose()
    {
        close(s);
    }
};
int Sender::s = 0;
PktQueue *Sender::pqueue = 0;
struct sockaddr_in Sender::serv;
