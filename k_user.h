/******************************************************************************
***N***N**Y***Y**X***X*********************************************************
***NN**N***Y*Y****X*X**********************************************************
***N*N*N****Y******X***********************************************************
***N**NN****Y*****X*X************************ Make Date  : Jun 30, 2007 *******
***N***N****Y****X***X*********************** Last Update: Jul 07, 2007 *******
******************************************************************************/
#pragma once
#include <cstdio>
#include "nSTL.h"
#include "k_message.h"
#include "k_frame.h"



extern int setting_flood_msg_nb;
extern unsigned int var_setting_flood_msg_time;
extern unsigned int setting_min_ping;
extern int setting_max_conn_set;
extern odlist<char*> setting_motd;

extern void __cdecl kprintf(char * arg_0, ...);

class k_user 
{

public:
	//派生dlist
	class k_userlist: public dlist<k_user*> 
	{
	public:
		unsigned int time_;
		void step();
		//对所有游戏列表发送指令消息instr,（等于广播）
		void send_instruction(k_instruction * instr);

		int logged_in_users_count();
		void write_login_success(k_message * pmsg);
		k_user * find_user(unsigned short id_);
	};
	//
	class k_game 
	{
	public:
		k_userlist players;
		k_user * owner;
		unsigned int id;
		int status;
		char name[0x80];
		int maxusers;
		unsigned int frame_counter;
		static unsigned int game_id;

		k_game(char * name_, k_user * owner_);
		void add_user(k_user * user);
		void send_status_update();
		void write_GAMRSLST(k_message * pmsg);
		void start_game();
		bool remove_user(k_user * player);
		bool step();
	};
	class k_gamelist: public dlist<k_game*, 32> 
	{
	public:
		void step();
		int posof(k_game * game);
		k_game * find_game(unsigned int id_);
	};

	public:	
		static k_userlist userslist;
		static k_gamelist gameslist;
		static unsigned short user_id;
	public:
        unsigned short id;
        k_message * sock;
        char appname[128];
		char username[32];
        int status;
		char connection;
		unsigned int ping;
		unsigned int throughput;
		unsigned int login_time;
		unsigned int login_timeout;
		k_game * game;
		int sock_status;
		int floodnb;
		unsigned int floodtime;
		bool player_ready;
		int playerno;
		odlist<k_frame*, 256> incoming_cache;
		odlist<k_frame*, 256> outgoing_cache;
		int frame_size;
		k_frame working_frame;
		k_frame another_working_frame;
		unsigned int netsync_timeout_time;
		int netsync_timeout_count;
		unsigned int data_timeout_time;
		int data_timeout_count;
		unsigned int client_reply_frame;   
		bool untrimmed_used_data;
		k_user ();
		~k_user();
		int get_port();
		void send_instruction(k_instruction * inst);
		bool step(unsigned int time_);

		void start_game()
		{
			status = 2;
			data_timeout_count = netsync_timeout_count = 0;
			frame_size = -1;
			player_ready = false;
			netsync_timeout_time = userslist.time_ + 10000;
			untrimmed_used_data = 0;
			client_reply_frame = ((throughput + 1) * connection) - 1;
			//send gamebegn to user

			//写用户信息到k_instruction  kix
			k_instruction kix;
			kix.type = INSTRUCTION_GAMEBEGN;
			kix.store_short(throughput);
			kix.store_char(playerno);
			kix.store_char(game->players.length);

			//将kix的信息保存到sock中的out_cache
			sock->send_instruction(&kix);

			//清空消息
			while (incoming_cache.length > 0)
			{
				delete incoming_cache.get(0);
				incoming_cache.remove(0);
			}
			while (incoming_cache.length > 0)
			{
				delete incoming_cache.get(0);
				incoming_cache.remove(0);
			}
			another_working_frame.pos = working_frame.pos = 0;
		}
		void leave_game()
		{
			if (game != 0 && gameslist.posof(game) != 0)
			{
				k_instruction kix;
				kix.type = INSTRUCTION_GAMRLEAV;
				kix.store_short(id);
				kix.set_username(username);

				//
				game->players.send_instruction(&kix);

				if (game->remove_user(this))
				{
					kprintf("Closing game");
					delete game;
				}
			}
			game = 0;
			//login_timeout = userslist.time_ + 120000;
		}
		void drop_game()
		{
			if (game != 0 && gameslist.posof(game) != 0)
			{
				k_instruction kix;
				kix.type = INSTRUCTION_GAMRDROP;
				kix.store_char(playerno);
				kix.set_username(username);
				game->players.send_instruction(& kix);
				status = 1;
				login_timeout = userslist.time_ + 120000;
			}
		}
		bool has_data()
		{
			return (working_frame.pos > 0);
		}
		int peek_frame(char * buffer)
		{
			return working_frame.peek_data(buffer, frame_size);
		}
		void send_data(char * frame_buf, int frame_len);
		int get_data(char * datab);
};
