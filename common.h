
#ifndef __COMMON_H__
#define __COMMON_H__
#include <queue>
#include <map>
#include <pthread.h>
#define PORT 24000
#define PKT_SIZE 1000
#define AUDIO 1
#define VIDEO 2
const char GROUP[] = "224.2.2.2";
typedef u_char uchar;
typedef struct Packet
{   
    uchar type;
    char  *name;
    uchar *data;
    ushort size;
} Packet;

typedef struct Header
{
    // uchar type;
    uchar id;
    short length; 
    uint seq;
    short inner_seq;
    char name[11];
} __attribute__((packed)) Header;
class PktQueue
{
  private:
    std::queue<Packet *> q;
    Packet data;
    //pthread_mutex_t mutex;

  public:
    PktQueue()
    {
        data.data = nullptr;
        data.name = nullptr;
        //pthread_mutex_init(&mutex, NULL);
    }
    void push(Packet *u)
    {
        q.push(u);
    }
    int size()
    {
        return q.size();
    }
    Packet *pop()
    {

        if (q.empty())
        {
            return NULL;
        }
       
        delete[] data.data;
        delete[] data.name;
        Packet *temp = q.front();
        data = *temp;
        delete temp;
        q.pop();
        return &data;
    }
};
typedef std::map<uchar,uint> UserSeq;// receive port
typedef std::map<uchar,std::string> UserName;// usr nameStr
#endif
