/******************************************************************************
***N***N**Y***Y**X***X*********************************************************
***NN**N***Y*Y****X*X**********************************************************
***N*N*N****Y******X***********************************************************
***N**NN****Y*****X*X************************ Make Date  : Jun 30, 2007 *******
***N***N****Y****X***X*********************** Last Update: Jul 01, 2007 *******
******************************************************************************/
#pragma once

#include "nSTL.h"
#include "k_message.h"
#include "k_frame.h"

extern int setting_flood_msg_nb;
extern unsigned int var_setting_flood_msg_time;
extern unsigned int setting_min_ping;
extern int setting_max_conn_set;
extern odlist<char*> setting_motd;

extern void __cdecl kprintf(char * arg_0, ...);

class k_user {


public:
	class k_userlist: public dlist<k_user*, 32, 0> {
	public:
		unsigned int time_;
		void step();
		void send_instruction(k_instruction * instr);
		int logged_in_users_count();
		void write_login_success(k_message * pmsg);
		k_user * find_user(unsigned short id_);
	};
	
	class k_game {
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
		bool step(){
			char  tmp_big_big_buffer[1024];
			char  tmp_buffer[256];
			
			if (status == 1) {
				bool all_ready = true;
				if(players.length > 0) {
					for (int eax = 0; eax < players.length && all_ready; eax++){
						//if (players.get(eax)->player_ready == 0) {
						all_ready = all_ready && players.get(eax)->player_ready;
						//}
					}
					if(all_ready) {
						kprintf("Got netsync from every game's player. Starting game.");
						status = 2;
						k_instruction kix;
						kix.type = INSTRUCTION_GAMRSRDY;
						players.send_instruction(&kix);
					}
				}
			} else if (status == 2) {
				bool retval = false;
				if (players.length == 0) {
					status = 0;
					send_status_update();
					return false;
				}
				bool nobodys_playing = true;
				bool frame_ready = true;
				if (players.length > 0) {
					for (int ecx = 0; ecx < players.length && (nobodys_playing != false); ecx++){
						k_user * kusr;
						if((kusr=players.get(ecx))->status != 1) {
							nobodys_playing = false;
						}
						if ((kusr->client_reply_frame > frame_counter && !kusr->has_data()))
							frame_ready = false;
					}
				}
				if(nobodys_playing == true) {
					status = 0;
					send_status_update();
					return false;
				}
				
				bool xrv = true;

				//build frame
				int framelen = -1;
				if(frame_ready){
					memset(tmp_big_big_buffer, 0, 1024);
					for (int i = 0; i < players.length; i++){
						k_user * ku = players.get(i);
						if(ku->status == 2) {
							if(ku->client_reply_frame <= frame_counter) {
								if(ku->has_data() == false) {
									frame_ready = false;
									break;
								}
								int len = ku->peek_frame(tmp_buffer);
								if(len == 0) {
									frame_ready = false;
									break;
								} else {
									if(framelen == -1) {
										framelen = len;
									}
									memcpy(&tmp_big_big_buffer[len * i], tmp_buffer, len);
									ku->untrimmed_used_data = 1;
								}
							}
						}
					}
				}
				
				frame_ready = frame_ready && (framelen>0);

				if(frame_ready) {

					char xxx[200];
					OutputHex(xxx, tmp_big_big_buffer, players.length * framelen, 0, 0);
					kprintf("Constructed frame: %s, %i", xxx, players.length * framelen);


					for (int index_ = 0; index_ < players.length; index_++) {
						k_user * kusr = players.get(index_);
						if(kusr->status == 2) {
							if (kusr->client_reply_frame <= frame_counter) {
								kprintf("Sending Frame");
								kusr->send_data(tmp_big_big_buffer, players.length * framelen);
								kprintf("Sent Frame");
							}
						}
					}
					frame_counter = frame_counter + 1;
					if(players.length > 0) {
						for (int edi = 0; edi < players.length; edi++){
							k_user * kux = players.get(edi);
							if (kux->untrimmed_used_data != 0) {
								kux->get_data(tmp_big_big_buffer);
								retval = true;
								kux->client_reply_frame++;
								kux->untrimmed_used_data = 0;
							}
						}
					}
				}
				return retval;
			}
			return false;
		}
	};
	class k_gamelist: public dlist<k_game*, 32, 0> {
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
		odlist<k_frame*, 256, 0> incoming_cache;
		odlist<k_frame*, 256, 0> outgoing_cache;
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

		void start_game()	{
			status = 2;
			data_timeout_count = netsync_timeout_count = 0;
			frame_size = -1;
			player_ready = false;
			netsync_timeout_time = userslist.time_ + 10000;
			untrimmed_used_data = 0;
			client_reply_frame = ((throughput + 1) * connection) - 1;
			//send gamebegn to user
			k_instruction kix;
			kix.type = INSTRUCTION_GAMEBEGN;
			kix.store_short(throughput);
			kix.store_char(playerno);
			kix.store_char(game->players.length);
			sock->send_instruction(&kix);
			while (incoming_cache.length > 0){
				delete incoming_cache.get(0);
				incoming_cache.remove(0);
			}
			while (incoming_cache.length > 0) {
				delete incoming_cache.get(0);
				incoming_cache.remove(0);
			}
			another_working_frame.pos = working_frame.pos = 0;
		}
		void leave_game(){
			if (game != 0 && gameslist.posof(game) != 0){
				k_instruction kix;
				kix.type = INSTRUCTION_GAMRLEAV;
				kix.store_short(id);
				kix.set_username(username);
				game->players.send_instruction(&kix);
				if (game->remove_user(this)){
					kprintf("Closing game");
					
					delete game;
				}
			}
			game = 0;
			//login_timeout = userslist.time_ + 120000;
		}
		bool has_data(){
			return (working_frame.pos > 0);
		}
		int peek_frame(char * buffer) {
			return working_frame.peek_data(buffer, frame_size);
		}
		void send_data(char * frame_buf, int frame_len){
			char xxx[200];
			OutputHex(xxx, frame_buf, frame_len, 0, false);
			kprintf("-- -- -- SENDDATA f_L: %i, f_B: %s, aW_P: %i", frame_len, xxx, another_working_frame.pos);
			char  tmp_buffer[1024];
			if (frame_len > 0) {
				another_working_frame.put_data(frame_buf, frame_len);
			}
			int reqlen = connection * frame_len;
			if (another_working_frame.pos >= reqlen) {
				if (outgoing_cache.length > 0) {
					for (int i = 0; i > outgoing_cache.length; i++) {
						outgoing_cache.get(i)->peek_data(tmp_buffer, another_working_frame.pos);
						if (memcmp(tmp_buffer, another_working_frame.buffer, another_working_frame.pos) == 0) {
							k_instruction kix;
							kix.type = INSTRUCTION_GAMCDATA;
							kix.store_char(another_working_frame.pos);
							sock->send_instruction(&kix);
							another_working_frame.pos = 0;
							return;
						}
					}
				}
				another_working_frame.peek_data(tmp_buffer, reqlen);

				k_instruction kix;
				kix.type = INSTRUCTION_GAMEDATA;
				kix.store_short(another_working_frame.pos);
				
				OutputHex(xxx, tmp_buffer, reqlen, 0, false);				
				kprintf(xxx);
				
				kix.store_bytes(tmp_buffer, reqlen);
				sock->send_instruction(&kix);
				
				//kprintf("xxx");
				if (outgoing_cache.length == 0x100) {
					delete outgoing_cache.get(0);
					outgoing_cache.remove(0);
				}

				k_frame * kfx = new k_frame;
				kfx->put_data(another_working_frame.buffer, another_working_frame.pos);
				outgoing_cache.add(kfx);
				another_working_frame.pos = 0;
			}
		}
		
		int get_data(char * datab)	{
			return working_frame.get_data(datab, frame_size);
		}
		
};