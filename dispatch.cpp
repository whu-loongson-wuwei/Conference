#include <sys/socket.h>
#include <sys/types.h>
#include <cstring>
#include <netinet/in.h>
#include <stdio.h>  
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <arpa/inet.h>
#include "common.h"
using namespace std;
class Receiver
{
  public:
    static int s;
    static struct sockaddr_in serv;
    static PktQueue *pqueue;

  public:
    Receiver(PktQueue *queue)
    {
        pqueue = queue;
        s = socket(AF_INET, SOCK_DGRAM, 0);
        bzero(&serv, sizeof(serv));
        serv.sin_family = AF_INET;
        serv.sin_addr.s_addr = htonl(INADDR_ANY);
        serv.sin_port = htons(PORT);
    }
    void Receive()
    {
       
        struct sockaddr_in client;
        struct ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = inet_addr(GROUP);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
            cout << "error in mreq" << endl;
        if (bind(s, (struct sockaddr *)&serv, sizeof(serv)) == -1)
            cout << "bind error" << endl;

        socklen_t addr_len = sizeof(client);
        Packet *pkt;
        uchar *buff = new uchar[PKT_SIZE + 20];
        uchar *merge_buf;
        int len_buf = 0;
        bool is_first_alloc = true;
        uint currentPkt = 0;
        /*-------------------------------------------------------**/
        int u = socket(AF_INET, SOCK_DGRAM, 0);
        struct sockaddr_in usr;
        usr.sin_family = AF_INET;
        usr.sin_addr.s_addr = inet_addr(GROUP);
        usr.sin_port = htons(PORT);
        Header *pkt_id = new Header;
        pkt_id->length = 0;
        pkt_id->id = 2;
        memcpy(pkt_id->name, "loongson", 9);
        sendto(u, pkt_id, 20, 0, (struct sockaddr *)&usr, addr_len);
        usr.sin_addr.s_addr =  inet_addr("127.0.0.1");
        UserName un;
        UserSeq us;
        int port = 26000;
        /*-------------------------------------------------------**/
        while (1)
        {
            int ret = recvfrom(s, buff, PKT_SIZE + 9, 0, (struct sockaddr *)&client, &addr_len);
            Header *h = (Header *)buff;
            uchar id = h->id;
            int l = h->length;
            if (h->length == 0)
            {
                if (un.find(id) == un.end())
                {
                    un[id] = h->name;
                    us[id] = port++;
                    cout << h->name << " join in" << endl;
                    char str[8];
                    sprintf(str, "%d", us[id]);
                    char *execv_str[] = {"receive",str, h->name,NULL};
                    if (fork() == 0)
                        if (execv("./receive", execv_str) < 0)
                            exit(0);
                }
                continue;
            }
            else if (un.find(id) == un.end())
            {
                continue;
            }
            else
            {  
                usr.sin_port = htons(us[id]);
                sendto(u, buff,  PKT_SIZE + 9, 0, (struct sockaddr *)&usr, addr_len);
            }
        }
    }
    void Sclose()
    {
        close(s);
    }
};
int Receiver::s = 0;
PktQueue *Receiver::pqueue = 0;
struct sockaddr_in Receiver::serv;
int main()
{
Receiver d(NULL);
d.Receive();
return 0;
}