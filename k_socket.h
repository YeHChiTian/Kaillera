/******************************************************************************
***N***N**Y***Y**X***X*********************************************************
***NN**N***Y*Y****X*X**********************************************************
***N*N*N****Y******X***********************************************************
***N**NN****Y*****X*X************************ Make Date  : Jul 01, 2007 *******
***N***N****Y****X***X*********************** Last Update: Jul 01, 2007 *******
******************************************************************************/
#pragma once
//extern void __cdecl kprintf(char * arg_0, ...);
//#define FD_SETSIZE 100

/*************************************************
 Dependencies ************************************
*************************************************/
#if defined(linux)

#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SOCKET int
#define INVALID_SOCKET -1
#define TIMEVAL timeval

#else

#include "winsock2.h"

#define ioctl ioctlsocket

#endif

#include "nSTL.h"

/*************************************************
 Sockets Class ***********************************
*************************************************/

class k_socket {

protected:
public:
    unsigned short port;
    SOCKET sock;
    bool has_data_waiting;
    sockaddr_in addr;
    static slist<k_socket*, FD_SETSIZE> list;
    static SOCKET ndfs;      //select 最大的描述符号
    static fd_set sockets;   //描述符集 这个将用来测试有没有一个可用的连接
    static fd_set temp;
public:
    k_socket()
	{
        sock = 0;
        list.add(this);
        has_data_waiting = false;
        if(ndfs == 0) 
		{
            FD_ZERO(&sockets);
            FD_ZERO(&temp);
        }
        port = 0;
    }
    ~k_socket()
	{
        close();
    }
    virtual int clone(k_socket * remote){
        port = remote->port;
        sock = remote->sock;
        return 0;
    }
    void close(){
        if(sock != 0) {
			//The shutdown function disables sends or receives on a socket.  SD_RECEIVE 0, SD_SEND 1, SD_BOTH 2
            shutdown(sock, 2);
            closesocket(sock);
            FD_CLR(sock, &sockets);

			//更新ndfs的值
            if(sock == ndfs) {
                ndfs = 0;
                SOCKET temp = 0;
                k_socket * ks;
                if(list.size() > 0) {
                    for (int i = 0; i < list.size(); i++) {
                        if ((ks = list.get(i)) != this) {
                            if (ks->sock > ndfs){
                                ndfs = ks->sock;
                            }
                        }
                    }
                }
            }
        }
		// 从list中删除this
        if(list.size() > 0) {
            for (int i = 0; i < list.size(); i++) {
                if (list.get(i) == this) {
                    k_socket::list.remove(i--);
                }
            }
        }
    }
	//创建socket并且bind相应的监听地址和信息
    virtual int initialize(int param_port){
        port = param_port;
        sockaddr_in tempaddr;
        memset(&tempaddr, 0, sizeof(tempaddr));
        tempaddr.sin_family = AF_INET;
        tempaddr.sin_port = htons(param_port);
		//IPPROTO_IP : If a value of 0 is specified, the caller does not wish to specify a protocol and the service provider will choose the protocol to use.
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);


        if (sock != SOCKET_ERROR)
		{
            tempaddr.sin_addr.s_addr = htonl(0);
            if (bind(sock, (sockaddr*)&tempaddr, sizeof(tempaddr))==0)
			{
                unsigned long ul = 1;
				//The ioctlsocket function controls the I / O mode of a socket.
				//ul =0, blocking is enabled,  !=0, non-blocking mode is enabled
                ioctlsocket (sock, FIONBIO, &ul);

				//把sock放入要测试的描述符集 就是说把sock放入了sockets里面 这样下一步调用select对sockets进行测试的时候就会测试sock了(因为我们将sock放入的socket) 一个描述符集可以包含多个被测试的描述符,
                FD_SET(sock, &sockets);

                if (sock > ndfs)
                    ndfs = sock;
                if (port == 0) {
                    param_port = sizeof(tempaddr);
					//The getsockname function retrieves the local name for a socket.
                    getsockname(sock, (sockaddr*)&tempaddr, &param_port);
                    port = ntohs(tempaddr.sin_port);
                }
                return 0;            
            } else {
                return 1;
            }
        } else {
            //printf("uh oh. socket() returned -1!\n");
            return 1;
        }
    }
	//设置成员addr的IP和端口
    virtual int set_address(const char * cp, const u_short hostshort){
        memset(&addr, 0, 16);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(hostshort);
        if (addr.sin_port != 0) {
            addr.sin_addr.s_addr = inet_addr(cp);
            if (addr.sin_addr.s_addr == -1) {
                hostent * h = gethostbyname(cp);
                if (h==0) {
                    return 1;
                } else {
                    memcpy(&addr.sin_addr,&h->h_addr_list[0],h->h_length);
                }
            }
            return 0;
        } else return 1;
    }
    virtual void set_addr(sockaddr_in * arg_addr) {
        memcpy(&addr, arg_addr, sizeof(addr));
    }
    virtual bool set_aport(int port){
        return ((addr.sin_port = htons(port)) != 0);
    }
    virtual bool send(char * buf, int len)
	{
        return (sendto(sock, buf, len, 0, (sockaddr*)&addr, 16) == -1 );
    }
    virtual int check_recv (char* buf, int * len, bool leave_in_queue, sockaddr_in* addrp)  {
        struct sockaddr saa;
        int V4 = sizeof(saa);
        has_data_waiting = 0;
        int  lenn = 0;
        if ((lenn = recvfrom(sock, buf, *len, leave_in_queue? MSG_PEEK:0, &saa, & V4)) <= 0) {
            return 1;
        } else {
            *len = lenn;
            if(lenn != 0) {
                memcpy(addrp, &saa, sizeof(saa));
            }
            return 0;
        }
    }
    virtual bool has_data(){
        return has_data_waiting;
    }
    virtual int send_nothing(){
        char buf[16];
        return sendto(sock, buf, 0, 0, (sockaddr*)&addr, sizeof(addr));
    }
    virtual int get_port(){
        return port;
    }
	
	char* to_string(char *buf){
		sprintf(buf, "k_socket {\n\tsock: %u;\n\tport: %u;\n\thas_data: %i;\n};", sock, port, has_data_waiting);
		return buf;
	}
	/*
	有三种情况：
	timeval
	timeout == NULL  等待无限长的时间。等待可以被一个信号中断。当有一个描述符做好准备或者是捕获到一个信号时函数会返回。如果捕获到一个信号， select函数将返回 -1,并将变量 erro设为 EINTR。
	timeout->tv_sec == 0 &&timeout->tv_usec == 0不等待，直接返回。加入描述符集的描述符都会被测试，并且返回满足要求的描述符的个数。这种方法通过轮询，无阻塞地获得了多个文件描述符状态。
	timeout->tv_sec !=0 ||timeout->tv_usec!= 0 等待指定的时间。当有描述符符合条件或者超过超时时间的话，函数返回。在超时时间即将用完但又没有描述符合条件的话，返回 0。对于第一种情况，等待也会被信号所中断。
	*/
    static bool check_sockets(int secs, int ms){
        timeval tv;
        tv.tv_sec = secs;
        tv.tv_usec = ms * 1000;
        memcpy(&temp, &sockets, sizeof(temp));
        if(select((int)(ndfs + 1), &temp, 0, 0, &tv) != 0) {
            if(list.size() > 0) {
                for (int i = 0; i < list.size(); i++){
                    k_socket * k = list.get(i);
                    if (FD_ISSET(k->sock, &temp)!=0){
						//char xxx[200];
                        k->has_data_waiting = true;
						//kprintf("data on: %s", k->to_string(xxx));
                    }
                }
            }
            return true;
        }
        return false;
    }
    static bool Initialize(){
		#if !defined(linux)
        WSAData ws;
        return (WSAStartup(0x0101, &ws)==0);
		#else
		return true;
		#endif

    }
    static void Cleanup(){
		#if !defined(linux)
        WSACleanup();
		#endif
    }
};


