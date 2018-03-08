/******************************************************************************
***N***N**Y***Y**X***X*********************************************************
***NN**N***Y*Y****X*X**********************************************************
***N*N*N****Y******X***********************************************************
***N**NN****Y*****X*X************************ Make Date  : Jun 29, 2007 *******
***N***N****Y****X***X*********************** Last Update: Jul 06, 2007 *******
******************************************************************************/
#include "settings.h"
#include "nSTL.h"
char setting_server_name[64] = "Unknown serv0r";
int setting_max_users = 50;
int setting_port = 27888;
char setting_location[64] = "Unknown";
int setting_flood_msg_nb = 5;
unsigned int var_setting_flood_msg_time = 3;
unsigned int setting_min_ping = 0;
int setting_max_conn_set = 0;
odlist<char*> setting_motd;

void LoadSettings(int argc, char * args[]){
	//for now we'll hardcode
	setting_motd.add("Nyx server build " __DATE__ " " __TIME__);
	setting_motd.add("Experimental release, contains traces of bugs n stuff...");
}
