/******************************************************************************
***N***N**Y***Y**X***X*********************************************************
***NN**N***Y*Y****X*X**********************************************************
***N*N*N****Y******X***********************************************************
***N**NN****Y*****X*X************************ Make Date  : Jul 02, 2007 *******
***N***N****Y****X***X*********************** Last Update: Jul 07, 2007 *******
******************************************************************************/
#include "k_user.h"

//#define PRINT_TRACE

#ifdef PRINT_TRACE
void __cdecl kprintfx(char * arg_0, ...);
void TraceFCN(const char * file, const int line, const char * x){
	kprintfx("%s:%i> %s", file, line,x);
}
	#define TRACE(X) TraceFCN(__FILE__,__LINE__,#X)
#else
	#define TRACE(X)
#endif

k_user::k_userlist k_user::userslist;
k_user::k_gamelist k_user::gameslist;
unsigned short k_user::user_id = 0;
unsigned int k_user::k_game::game_id;

k_user::k_user () {
	TRACE(k_user::k_user);
	id = user_id++;
	sock = new k_message();
	appname[0] = 0;
	username[0] = 0;
	status = 0;
	connection = 0;
	ping = GetTickCount();
	throughput = 0;
	login_time = 0;
	login_timeout = 0;
	game = 0;
	sock_status = sock->initialize(0);
	floodnb = 0;
	floodtime = 0;
}
k_user::~k_user() {
	TRACE(k_user::~k_user);
	delete sock;
	while (incoming_cache.length > 0){
		delete incoming_cache.get(0);
		incoming_cache.remove(0);
	}
	while (incoming_cache.length > 0) {
		delete incoming_cache.get(0);
		incoming_cache.remove(0);
	}
}
int k_user::get_port(){
	return sock->get_port();
}
void k_user::send_instruction(k_instruction * inst){
	sock->send_instruction(inst);
}
bool k_user::step(unsigned int time_) {
	if (sock->has_data()){
		sockaddr_in saddr;
		k_instruction ki;
		if (sock->receive_instruction(&ki, false, &saddr) == 0) {
			switch (ki.type) {
				case INSTRUCTION_USERLEAV:
				{
					ki.load_short();
					char xxx[128];
					ki.load_str(xxx, 128);
					k_instruction exit_not;
					exit_not.type = INSTRUCTION_USERLEAV;
					exit_not.store_short(id);
					exit_not.store_string(xxx);
					exit_not.set_username(username);
					k_user::userslist.send_instruction(&exit_not);
					leave_game();
					kprintf("%s left.", username);
					return true;
				}
				case INSTRUCTION_USERLOGN:
				{
					sock->set_addr(&saddr);
					ki.load_str(appname, 128);
					connection = ki.load_char();
					strcpy(username, ki.user);
					connection = (connection > 0 && connection < 7) ? connection : 2;
					ping = login_time = time_;
					k_instruction kit;
					kit.type = INSTRUCTION_SERVPING;
					for (int i = 0; i < 4; i++) 
						kit.store_int(i);
					sock->send_instruction(&kit);
					break;
				}
				case INSTRUCTION_USERPONG:
				{
					if (status != 0)
						break;
					throughput++;
					if (throughput == 4) {
						unsigned int t = login_time;
						login_time = time_;
						ping = (time_ - t) / 4;
						login_timeout = time_ + 120000;
						throughput = (60 / 1000 * ping / connection) + 1;
						if (setting_min_ping != 0 && ping > setting_min_ping) {
							kprintf("%s has a ping too high (%ims>%ims). ejecting client.", username, ping, setting_min_ping);
							char xxx[200];
							sprintf(xxx, "Rejected: Ping too high (%ims>%ims)", ping, setting_min_ping);
							k_instruction kix;
							kix.type = LOGNSTAT;
							kix.store_short(id);
							kix.store_string(xxx);
							kix.set_username(username);
							sock->send_instruction(& kix);
							return true;
						}
						if (setting_max_conn_set != 0 && connection > setting_max_conn_set) {
							char * connection_type_strarray [] = {"", "LAN", "Excellent", "Good", "Average", "Low", "Bad"};
							char xxx[200];
							kprintf("%s has a connection setting too high (%s>%s). ejecting client.", username, connection_type_strarray[connection], connection_type_strarray[setting_max_conn_set]);
							sprintf(xxx, "Rejected: Connection type too high (%s>%s)", connection_type_strarray[connection], connection_type_strarray[setting_max_conn_set]);
							k_instruction kx;
							kx.type = LOGNSTAT;
							kx.store_short(id);
							kx.store_string(xxx);
							kx.set_username(username);
							sock->send_instruction(& kx);
							return 1;
						}
						userslist.write_login_success(sock);
						status = 1;
						k_instruction kix;
						kix.type = INSTRUCTION_USERJOIN;
						kix.store_short(id);
						kix.store_int(ping);
						kix.store_char(connection);
						kix.set_username(username);
						userslist.send_instruction(& kix);
						if (setting_motd.length > 0) {
							for (int i = 0; i < setting_motd.length; i++){
								char * mtdstr = setting_motd.get(i);
								k_instruction kxz;
								kxz.type = MOTDLINE;
								kxz.store_string(mtdstr);
								kxz.set_username("Server");
								sock->send_instruction(&kxz);
							}
						}
						kprintf("%s connected, id=%i, latency=%i ms.", username, id, ping);
					} else {
						k_instruction kix;
						kix.type = INSTRUCTION_SERVPING;
						for (int x = 0; x < 4; x++) kix.store_int(x);
						sock->send_instruction(&kix);
						ping = time_;
					}
					break;
				}
				case INSTRUCTION_PARTCHAT:
				{
					ki.set_username(username);
					userslist.send_instruction(& ki);
					floodnb++;
					if (floodtime + (var_setting_flood_msg_time * 1000) * 8 < time_) {
						floodnb = 0;
						floodtime = time_;
					}
					if (floodnb > setting_flood_msg_nb) {
						k_instruction kix;
						kix.type = INSTRUCTION_USERLEAV;
						kix.store_short(id);
						kix.store_string("Flood");
						kix.set_username(username);
						userslist.send_instruction(&kix);
						leave_game();
						kprintf("%s flooded! ejecting client.", username);
						return true;
					}
					break;
				}
				case INSTRUCTION_GAMECHAT:
				{
					if (game != 0 && gameslist.posof(game) != 0){
						ki.set_username(username);
						game->players.send_instruction(& ki);
					}
					break;
				}
				case INSTRUCTION_TMOUTRST:
				{
					login_timeout = time_ + 120000;
					break;
				}
				case INSTRUCTION_GAMEMAKE:
				{
					char xxxx[128];
					char xxx[128];
					ki.load_str(xxx, 0x80); //game name
					ki.load_str(xxxx, 0x80); //emu name
					int tid = ki.load_int(); //id

					game = new k_game(xxx, this);
					gameslist.add(game);

					k_instruction kix;
					kix.type = INSTRUCTION_GAMEMAKE;
					kix.store_string(xxx);
					kix.store_string(appname);
					kix.store_int(game->id);
					kix.set_username(username);
					userslist.send_instruction(&kix);
					game->add_user(this);
					kprintf("%s created a new game: %i, %s.", username, game->id, xxx);
					break;
				}
				case INSTRUCTION_GAMRLEAV:
				{
					kprintf("%s left game.", username);
					leave_game();
					break;
				}
				case INSTRUCTION_GAMRJOIN: 
				{
					unsigned int tid = ki.load_int();
					if ((game=gameslist.find_game(tid)) != 0 && game->status==0) {
						game->write_GAMRSLST(sock);
						game->add_user(this);
						kprintf("%s joined game %i.", username, game->players.length);
						game->send_status_update();
					}
					break;
				}
				case INSTRUCTION_GAMRKICK:
				{
					if (game != 0 && gameslist.posof(game) != 0 && game->owner == this){
						k_user * ku;
						if ((ku = userslist.find_user(ki.load_short())) != 0) {
							ku->leave_game();
						}
					}
					break;
				}
				case INSTRUCTION_GAMEBEGN:
				{
					if (game != 0 && gameslist.posof(game) != 0){
						game->start_game();
						kprintf("%s started game.", username);
					}
					break;
				}
				case INSTRUCTION_GAMEDATA:
				{
					char xxx[256];
					//kprintf("DATA - %s", ki.to_string(xxx));
					if (game != 0 && gameslist.posof(game) != 0) {
						data_timeout_count = 0;
						data_timeout_time = time_ + 2000;
						unsigned short DataSize = ki.load_short();								
						ki.load_bytes(xxx, DataSize);//char * bptr = ki.buffer;// + ki.buffer_pos;
						if (frame_size == -1) {
							frame_size = DataSize / connection;
						}
						working_frame.put_data(xxx, DataSize);
						if (incoming_cache.length == 0x100) {
							delete(incoming_cache.get(0));
							incoming_cache.remove(0);
						}
						k_frame * kfx = new k_frame(xxx, DataSize);
						//k_frame * kfx = new k_frame;
						//kfx->put_data(xxx, DataSize);
						incoming_cache.add(kfx);
						//kprintf("-- ICACHELEN: %i, WFPOS: %i", incoming_cache.length, working_frame.pos);
					}
					break;
				}
				case INSTRUCTION_GAMCDATA:
				{
					if (game != 0 && gameslist.posof(game) != 0) {
						data_timeout_count = 0;
						data_timeout_time = time_ + 2000;
						int index = ki.load_char();
						if (index < incoming_cache.length) {
							k_frame * kf = incoming_cache.get(index);
							working_frame.put_data(kf->buffer, kf->pos);
						}
					}
					break;
				}			
				case INSTRUCTION_GAMRDROP:
				{
					drop_game();
					break;
				}
				case INSTRUCTION_GAMRSRDY:
				{
					player_ready = true;
					break;
				}
			}
		}
	} else {
		if (status == 0) {//Logging in
			if (sock_status != 0)
				return true;
			if (time_ > ping + 10000) {
				kprintf("Client timeout while connecting.");
				return true;
			}
		} else if (status == 1) {//Logged in
			if (time_ > login_timeout) {
				kprintf("%s client keepalive timeout. Exiting client.", username);
				k_instruction kix;
				kix.type = INSTRUCTION_USERLEAV;
				kix.store_short(id);
				kix.store_string("Ping timeout");
				kix.set_username(username);
				userslist.send_instruction(&kix);
				leave_game();
				return true;
			}
		} else {//In game
			if (game != 0 && gameslist.posof(game) != 0){
				if (game->status == 1) {
					if (player_ready == 0) {
						if (time_ > netsync_timeout_time) {
							kprintf("%s client netsync timeout #%i", username, netsync_timeout_count);
							sock->resend_message(throughput + 4);
							netsync_timeout_time = userslist.time_ + 10000;
							if (++netsync_timeout_count==24) {
								kprintf("%s client got too many netsync timeouts. leaving game",  username);
								drop_game();
							}
						}
					}
				} else if (game->status == 2) {
					if (time_ > data_timeout_time) {
						kprintf("%s client timeout #%i", username, data_timeout_count);
						sock->resend_message(throughput + 4);
						data_timeout_time = time_ + 2000;
						if (++data_timeout_count >= 3) {
							kprintf("%s client got too many timeouts. leaving game", username);
							drop_game();
						}
					}
				}
			} else {
				leave_game();
			}
		}
	}
	return false;
}
void k_user::k_gamelist::step(){
	bool reiterate = true;
	while (reiterate) {
		reiterate = false;
		for (int i = 0; i < length; i++) {
			reiterate = reiterate || get(i)->step();
		}
	}
}
int k_user::k_gamelist::posof(k_game * game){
	for (int i = 0; i < length; i++) {
		if (game == get(i))
			return i+1;
	}
	return 0;
}
k_user::k_game * k_user::k_gamelist::find_game(unsigned int id_){
	k_game * game;
	for (int i = 0; i < length; i++) {
		if ( (game=get(i))->id == id_)
			return game;
	}
	return 0;
}
void k_user::k_userlist::step(){
	if (length > 0) {
		time_ = GetTickCount();
		for (int i = 0; i < length; i++) {
			k_user * ku;
			if ((ku = get(i))->step(time_)) {
				delete ku;
				remove(i);
				i--;
			}
		}
	}
}
void k_user::k_userlist::send_instruction(k_instruction * instr){
	if (length > 0) {
		for (int x = 0; x < length; x++) {
			get(x)->send_instruction(instr);
		}
	}
}
int k_user::k_userlist::logged_in_users_count(){
	int tt = 0;
	if (length > 0 ) {
		for (int x = 0; x < length; x++) {
			if (get(x)->status != 0)
				tt++;
		}
	}
	return tt;
}
void k_user::k_userlist::write_login_success(k_message * pmsg){
	k_instruction ki;
	k_user * usr;
	int gllen;
	ki.type = LONGSUCC;
	ki.store_int(logged_in_users_count());
	ki.store_int(gllen = gameslist.size());
	if (length > 0) {
		for (int i = 0; i < length; i++){
			if ((usr = get(i))->status != 0) {
				ki.store_string (usr->username);
				ki.store_int    (usr->ping);
				ki.store_char   (usr->status);
				ki.store_short  (usr->id);
				ki.store_char   (usr->connection);
			}
		}
	}
	char Vc[12];
	if (gllen > 0) {
		for (int i = 0; i < gllen; i++) {
			k_game * game = gameslist.get(i);					
			ki.store_string(game->name);
			ki.store_int(game->id);
			ki.store_string(game->owner->appname);
			ki.store_string(game->owner->username);
			sprintf(Vc, "%i/%i", game->players.length, game->maxusers);
			ki.store_string(Vc);
			ki.store_char(game->status);
		}
	}
	pmsg->send_instruction(&ki);
}

k_user * k_user::k_userlist::find_user(unsigned short id_){
	k_user * ku;
	for (int x = 0; x < length; x++) {
		if ((ku=get(x))->id == id_)
			return ku;
	}
	return 0;
}


k_user::k_game::k_game(char * name_, k_user * owner_) {
	status = 0;
	strncpy(name, name_, 127);
	name[127] = 0;
	owner = owner_;
	maxusers = 2;
	id = game_id++;
}
void k_user::k_game::add_user(k_user * user) {
	players.add(user);	
	k_instruction kix;
	kix.type = INSTRUCTION_GAMRJOIN;
	kix.store_int((int)this);
	kix.store_string(user->username);
	kix.store_int(user->ping);
	kix.store_short(user->id);
	kix.store_char(user->connection);
	players.send_instruction(&kix);
}

void k_user::k_game::send_status_update(){
	k_instruction kix;
	kix.type = INSTRUCTION_GAMESTAT;
	kix.store_int(id);
	kix.store_char(status);
	kix.store_char(players.length);
	kix.store_char(maxusers);
	userslist.send_instruction(&kix);
}
void k_user::k_game::write_GAMRSLST(k_message * pmsg){
	k_instruction kix;
	kix.type = INSTRUCTION_GAMRSLST;
	kix.store_int(players.length);
	for (int esi = 0; esi < players.length; esi++) {
		k_user * kux = players.get(esi);
		kix.store_string(kux->username);
		kix.store_int(kux->ping);
		kix.store_short(kux->id);
		kix.store_char(kux->connection);
	}
	pmsg->send_instruction( & kix);
}
void k_user::k_game::start_game() {
	status = 1;
	frame_counter = 1;
	for (int i = 0; i < players.length; i++){
		k_user * ku = players.get(i);
		ku->playerno = i + 1;
		ku->start_game();
	}
	send_status_update();
}
bool k_user::k_game::remove_user(k_user * player){
	for (int i = 0; i < players.length; i++){
		if(players.get(i) == player) {
			players.remove(i);
			i--;
		}
	}			
	if (player == owner) {
		k_instruction kix;
		kix.type = INSTRUCTION_GAMESHUT;
		kix.store_int(id);
		userslist.send_instruction(&kix);
		int pos = gameslist.posof(this);
		if (pos > 0)
			gameslist.remove(pos-1);
		k_user * ku;
		for (int x = 0; x < userslist.length; x++) {
			if ((ku=userslist.get(x))->game == this && ku != player)
				ku->leave_game();
			//	((ku=get(x))->game == 0)
		}
		return true;
	} else {
		send_status_update();
	}
	return false;
}

bool k_user::k_game::step(){
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
		if (players.length > 0) {
			for (int ecx = 0; ecx < players.length && (nobodys_playing != false); ecx++){
				if(players.get(ecx)->status != 1) {
					nobodys_playing = false;
				}
			}
		}
		if(nobodys_playing == true) {
			status = 0;
			send_status_update();
			return false;
		}
		bool xrv = true;
		if(players.length > 0) {
			for (int index_ = 0; index_ < players.length; index_++) {
				k_user * kusr = players.get(index_);
				if(kusr->status == 2) {
					if (kusr->client_reply_frame <= frame_counter && kusr->untrimmed_used_data == 0) {
						if(kusr->has_data() == 0) {
							xrv = 0;
						} else {
							memset(tmp_big_big_buffer, 0, 1024);
							int framelen = -1;
							bool skip = false;
							bool pack_success = false;
							if (players.length> 0) {
								for (int i = 0; i < players.length; i++){
									k_user * ku = players.get(i);
									if(ku->status == 2) {
										if(ku->client_reply_frame <= frame_counter) {
											if(ku->has_data() == 0) {
												xrv = false;
												skip = true;
												break;
											}
											int len = ku->peek_frame(tmp_buffer);
											if(len == 0) {
												xrv = false;
												skip = true;
												break;
											} else {
												if(framelen == -1) {
													framelen = len;
												}
												memcpy(&tmp_big_big_buffer[len * i], tmp_buffer, len);
												pack_success = true;
											}
										}
									}
								}
								if (skip)
									continue;
								if (pack_success) {
									kusr->send_data(tmp_big_big_buffer, players.length * framelen);
									kusr->untrimmed_used_data = 1;
								}
							}
						}
					}
				}
			}
			if(xrv != false) {
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
		}
		return retval;
	}
	return false;
}
void k_user::send_data(char * frame_buf, int frame_len){
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
		kix.store_bytes(tmp_buffer, reqlen);
		sock->send_instruction(&kix);
		if (outgoing_cache.length == 0x100) {
			delete outgoing_cache.get(0);
			outgoing_cache.remove(0);
		}
		k_frame * kfx = new k_frame(another_working_frame.buffer, another_working_frame.pos);
		outgoing_cache.add(kfx);
		another_working_frame.pos = 0;
	}
}
int k_user::get_data(char * datab)	{
	int ret = working_frame.get_data(datab, frame_size);
	return ret;
}
