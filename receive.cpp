#include <sys/socket.h>
#include <sys/types.h>
#include <cstring>
#include <netinet/in.h>
#include <stdio.h>  
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <arpa/inet.h>
#include "common.h"
#include "decoder.h"
using namespace cv;
using namespace std;
class Receiver
{
  public:
    static int s;
    static struct sockaddr_in serv;
    static PktQueue *pqueue;
    static char*name;
  public:
    Receiver(PktQueue *queue,int port,char* n)
    {
        pqueue = queue;
        s = socket(AF_INET, SOCK_DGRAM, 0);
        bzero(&serv, sizeof(serv));
        serv.sin_family = AF_INET;
        serv.sin_addr.s_addr = inet_addr("127.0.0.1");
        serv.sin_port = htons(port);
        name = new char[11];
        memcpy(name,n,11);
    }
    void Receive()
    {
        pthread_t tid;
        pthread_create(&tid, NULL, receive, NULL);
    }
    static void *
    receive(void *)
    {

        struct sockaddr_in client;
        if (bind(s, (struct sockaddr *)&serv, sizeof(serv)) == -1)
            cout << "bind error" << endl;
        socklen_t addr_len = sizeof(client);
        Packet *pkt;
        uchar *buff = new uchar[PKT_SIZE + 20];
        uchar *merge_buf;
        int len_buf = 0;
        bool is_first_alloc = true;
        uint currentPkt = 0;
        while (1)
        {
            int ret = recvfrom(s, buff, PKT_SIZE + 9, 0, (struct sockaddr *)&client, &addr_len);
            Header *h = (Header *)buff;
            uchar id = h->id;
            int l = h->length;
            int g = h->inner_seq;
            if (currentPkt == 0)
                currentPkt = h->seq;
            if (currentPkt != h->seq) //Packet miss
            {
                cout << "Packet miss" << endl;
                currentPkt = h->seq;
                is_first_alloc = true;
                len_buf = 0;
                /* discard  the incompete   packet**/
            }
            if (is_first_alloc)
            {
                is_first_alloc = false;
                merge_buf = new uchar[l];
            }
            if (g < 0)
            {
                memcpy(merge_buf + l + g, buff + 9, -g);
                len_buf -= g;
            }
            else
            {
                memcpy(merge_buf + (g - 1) * PKT_SIZE, buff + 9, PKT_SIZE);
                len_buf += PKT_SIZE;
            }
            if (len_buf == l)
            {
                currentPkt = 0;
                pkt = new Packet;
                pkt->data = merge_buf;
                pkt->size = l;
                char *na = new char[11];
                memcpy(na, name, 11);
                pkt->name = na;
                pqueue->push(pkt);
                is_first_alloc = true;
                len_buf = 0;
            }
            cout <<"["<<name<<"] "<<"[recv]" << h->seq << "----->";
            if (g < 0)
                cout << "last" << endl;
            else
                cout << g << endl;
        }
        delete[] buff;
    }
    void Sclose()
    {
        close(s);
    }
};
int Receiver::s = 0;
PktQueue *Receiver::pqueue = 0;
struct sockaddr_in Receiver::serv;
char*Receiver::name;
int main(int argc,char *argv[])
{   
    int p;
    sscanf(argv[1],"%d",&p);
    PktQueue queue;
    Receiver *rec = new Receiver(&queue,p,argv[2]);
    Decoder *dec = new Decoder(&queue);
    rec->Receive();
    dec->decode();
    return 0;
}
