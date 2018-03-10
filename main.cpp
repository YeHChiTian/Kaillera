

char HEXDIGITS [] = 
{
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};


void OutputHex(char * outb, const void * inb, int len, int res1, bool usespace){
	char * xx = (char*)inb;
	for (int x = 0; x <	len; x++) {
		int dx = *xx++;
		int hib = (dx & 0xF0)>>4;
		int lob = (dx & 0x0F);
		*outb++ = HEXDIGITS[hib];
		*outb++ = HEXDIGITS[lob];
		if (usespace)
			*outb++ = ' ';
	}
	*outb = 0;
}


/******************************************************************************
***N***N**Y***Y**X***X*********************************************************
***NN**N***Y*Y****X*X**********************************************************
***N*N*N****Y******X***********************************************************
***N**NN****Y*****X*X************************ Make Date  : Jun 29, 2007 *******
***N***N****Y****X***X*********************** Last Update: Jul 06, 2007 *******
******************************************************************************/
#include <cstdio>
#include <time.h>
#include <stdarg.h>
#include <signal.h>
#include <stdlib.h>
#include "string.h"
#include "k_socket.h"


void __cdecl kprintfx(char * arg_0, ...) {
	char V8[1024];
	sprintf(V8, "%s\r\n", arg_0);
	va_list args;
	va_start (args, arg_0);
	vprintf (V8, args);
	va_end (args);

}
void __cdecl kprintf(char * arg_0, ...) {
	char V8[1024];

	time_t V4 = time(0);
	tm * ecx = localtime(&V4);
	sprintf(V8, "%02d/%02d/%02d-%02d:%02d:%02d> %s\r\n",ecx->tm_mday,ecx->tm_mon,ecx->tm_year % 0x64,ecx->tm_hour,ecx->tm_min,ecx->tm_sec, arg_0);
	va_list args;
	va_start (args, arg_0);
	vprintf (V8, args);
	va_end (args);

}

#include "nSTL.h"
#include "settings.h"

extern int setting_max_users;
extern int setting_port;

#include "k_user.h"

bool server_run;
void __cdecl Signalhandler(int signal_){
	if (signal_ == SIGINT){
		kprintf("SIGINT - Shutting down...");
		server_run = false;
	} else if (signal_ == SIGTERM) {
		kprintf("SIGTERM - Shutting down...");
		server_run = false;
	} else if (signal_ == SIGABRT) {
		kprintf("SIGABRT - Abnormal termination");
	} else if (signal_ == SIGILL) {
		kprintf("SIGILL - Illegal instruction");
	} else if (signal_ == SIGSEGV) {
		kprintf("SIGSEGV - Illegal storage access");
	}
}


int __cdecl main (int argc, char * args[]){
	printf("*******************************************************************************\n");
	printf("** Open Kaillera server *******************************************************\n");
	printf("*******************************************************************************\n");
	printf("*** BUILD: %s - %s\n",__DATE__, __TIME__);
	printf("*******************************************************************************\n");
	//Load Settings
	LoadSettings(argc, args);
	//Initialize sockets
	k_socket::Initialize();
	//add some signal handlers
	signal(SIGINT, Signalhandler);
	signal(SIGTERM, Signalhandler);
	signal(SIGABRT, Signalhandler);
	signal(SIGILL, Signalhandler);
	signal(SIGSEGV, Signalhandler);
	k_socket main_socket;
	kprintf("Initializing Kaillera communication layer...");
	if (main_socket.initialize(setting_port)==0){
		kprintf("Listening on port %i for incoming connections...", setting_port);
		char buffer[64];
		int bufferl = 0;
		server_run = true;
		while (server_run){
			if(main_socket.has_data() != 0) {
				sockaddr_in incaddr;
				memset(buffer, 0, 64);
				bufferl = 255;
				main_socket.check_recv(buffer, &bufferl, 0, &incaddr);   //检查是否有数据到来，到来的信息保存在incaddr中,数据保存在buffer中
				main_socket.set_addr(&incaddr);                          //将数据保存起来
				if (strcmp("PING", buffer) == 0){
					main_socket.send("PONG", 5);
				} else if(strncmp(buffer, "HELLO", 5) == 0) {
					kprintf("Got connection request from: %s:%i", inet_ntoa(incaddr.sin_addr), ntohs(incaddr.sin_port));
					if(strcmp("0.83", buffer+5) == 0) {
						if (k_user::userslist.size() < setting_max_users) {
							k_user * ku = new k_user;
							k_user::userslist.add(ku);
							sprintf(buffer, "HELLOD00D%i", ku->get_port());
							main_socket.send(buffer, (int)strlen(buffer)+1);
						} else {
							main_socket.send("TOO", 4);
						}
					} else {
						main_socket.send("VER", 4);
					}
				} else if(strncmp(buffer, "TERM", 4) == 0) {
					///////DO NOT FORGET TO REMOVE
					kprintf("Got termination request from: %s:%i", inet_ntoa(incaddr.sin_addr), ntohs(incaddr.sin_port));
					main_socket.close();
					k_socket::Cleanup();
					return 0;
				}
			}
			k_user::userslist.step();
			k_user::gameslist.step();
			k_socket::check_sockets(1, 0);
		}
		kprintf("Cleaning up...");
		main_socket.close();
		k_socket::Cleanup();
		return 0;
	} else {
		kprintf("ERROR: Can't listen on port %i!", setting_port);
		return 1;
	}	
	return 0;
}
