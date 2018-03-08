/******************************************************************************
***N***N**Y***Y**X***X*********************************************************
***NN**N***Y*Y****X*X**********************************************************
***N*N*N****Y******X***********************************************************
***N**NN****Y*****X*X************************ Make Date  : Jul 01, 2007 *******
***N***N****Y****X***X*********************** Last Update: Jul 01, 2007 *******
******************************************************************************/
#pragma once
extern void __cdecl kprintf(char * arg_0, ...);
#define FD_SETSIZE 100

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
    static SOCKET ndfs;
    static fd_set sockets;
    static fd_set temp;
public:
    k_socket(){
        sock = 0;
        list.add(this);
        has_data_waiting = false;
        if(ndfs == 0) {
            FD_ZERO(&sockets);
            FD_ZERO(&temp);
        }
        port = 0;
    }
    ~k_socket(){
        close();
    }
    virtual int clone(k_socket * remote){
        port = remote->port;
        sock = remote->sock;
        return 0;
    }
    void close(){
        if(sock != 0) {
            shutdown(sock, 2);
            closesocket(sock);
            FD_CLR(sock, &sockets);

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
        if(list.size() > 0) {
            for (int i = 0; i < list.size(); i++) {
                if (list.get(i) == this) {
                    k_socket::list.remove(i--);
                }
            }
        }
    }
    virtual int initialize(int param_port){
        port = param_port;
        sockaddr_in tempaddr;
        memset(&tempaddr, 0, sizeof(tempaddr));
        tempaddr.sin_family = AF_INET;
        tempaddr.sin_port = htons(param_port);
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock != SOCKET_ERROR) {
            tempaddr.sin_addr.s_addr = htonl(0);
            if (bind(sock, (sockaddr*)&tempaddr, sizeof(tempaddr))==0) {
                unsigned long ul = 1;
                ioctlsocket (sock, FIONBIO, &ul);

                FD_SET(sock, &sockets);

                if (sock > ndfs)
                    ndfs = sock;
                if (port == 0) {
                    param_port = sizeof(tempaddr);
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
    virtual bool send(char * buf, int len){
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


