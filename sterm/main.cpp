#include <cstdlib>
#include <iostream>
#include "k_socket.h"
#include "windows.h"

using namespace std;

int main(int argc, char *argv[])
{
 	k_socket::Initialize();
 	
 	k_socket kx;
 	if (kx.initialize(0)==0) {
	 	kx.set_address("127.0.0.1", 27888);
	 	kx.send("TERM", 5);
	 	Sleep(250);	 	
 		kx.close();
	}

    k_socket::Cleanup();
    return EXIT_SUCCESS;
}
