/******************************************************************************
***N***N**Y***Y**X***X*********************************************************
***NN**N***Y*Y****X*X**********************************************************
***N*N*N****Y******X***********************************************************
***N**NN****Y*****X*X************************ Make Date  : Jul 01, 2007 *******
***N***N****Y****X***X*********************** Last Update: Jul 01, 2007 *******
******************************************************************************/
#pragma once
#include "k_socket.h"
#include "k_instruction.h"

#pragma pack(push, 1)
//4个字节
typedef struct 
{
    unsigned short serial;
    unsigned short length;
} k_instruction_head;

#pragma pack(pop)
//sizeof(k_instruction_ptr) == 8,
typedef struct 
{
    k_instruction_head head;
    char * body;
} k_instruction_ptr; 


//消息的接受和发送
class k_message : public k_socket
{
    unsigned short      last_sent_instruction;
    unsigned short      last_processed_instruction;
    k_instruction_ptr   out_cache[256] ;  //发送消息从这里取
    int                 out_cache_len;    
    k_instruction_ptr   in_cache[255];    //接受消息存在这里
    int                 in_cache_length;
    int                 default_ipm;
public:
    k_message()
	{
        k_socket();
        out_cache_len = 0;
        in_cache_length = 0;
        last_sent_instruction = 0;
        last_processed_instruction = 0;
        default_ipm = 3;
    }
    ~k_message()
	{
        close();
    }
    void close()
	{
        for (int i = 0; i < in_cache_length; i++)
		{
            free(in_cache[i].body);
        }
        for (int j = 0; j < out_cache_len; j++)
		{
            free(out_cache[j].body);
        }
        k_socket::close();
    }
	//发送一条指令, 将k_instruction中type， user,buffer写到arg_0,并且保存到k_message中out_cache中。
    void send_instruction(k_instruction * arg_0)
	{
        char temp[32768];
        int l = arg_0->write_to_message(temp, 32767);
        k_message::send(temp, l);
    }
	//将buf信息保存到out_cache中
    bool send(char * buf, int len)
	{
        int vx = last_sent_instruction++;
		//对序列last_sent_instruction发送一个default_ipm信息
        if (out_cache_len > 0) 
		{
            for (int eax = 0; eax < out_cache_len; eax++)
			{
                if (out_cache[eax].head.serial == vx) 
				{
					
                    send_message(default_ipm);
                    return 0;
                }
            }
        }
        char * xxx;
		//0x100 = 256d
        if (out_cache_len == 0x100)
		{
			
            xxx = (char *)((len > out_cache[0].head.length) ? realloc(out_cache[0].body, len) : out_cache[0].body);

			//将out_cache上[1,255]移动到[0,254]上。
            memcpy(&out_cache[0], &out_cache[1], 255*8);
            out_cache_len--;
        }
		else
		{
            xxx = (char *) malloc(len);
        }
		
        memcpy(xxx,buf,len);
        out_cache[out_cache_len].head.serial = vx;
        out_cache[out_cache_len].head.length = len;
        out_cache[out_cache_len].body = xxx;
        out_cache_len++;
        send_message(default_ipm);
        return 0;
    }

	//发送缓冲区out_cache中至少limit条信息，从右往左 
    int send_message(int limit)
	{
        char buf[0x8000];
        int len = 1;
        char * buff = buf;
        char max_t = min(out_cache_len, limit);
        *buff++ = max_t;
		//将max_t条消息，放入到buf中。每条消息内容包含一个完整的out_cache(从右往左)
        if(max_t > 0)
		{
            for (int i = 0; i < max_t; i++)
			{
                int cache_index = out_cache_len - i -1;
                *(k_instruction_head*)buff = out_cache[cache_index].head;
                buff += 4;
                int l;
                memcpy(buff, out_cache[cache_index].body, l = out_cache[cache_index].head.length);
                buff += l;
                len += l + 4;
            }
        }
		//向客户端发送消息buf，长度为len.
		k_socket::send(buf, len);
        
        return 0;
    }

	//将当前处理接受到的数据写入到arg_0指令中
    int receive_instruction(k_instruction * arg_0, bool leave_in_queue, sockaddr_in* arg_8) 
	{
        char var_8000[0x8000];
        int var_8004 = 0x8000;
        if (check_recv(var_8000, &var_8004, leave_in_queue, arg_8)==1)
		{
            return 1;
        } 
		else 
		{
            arg_0->read_from_message(var_8000, var_8004);    
            return 0;
        }
    }
	//将收到的数据buffer存放在in_cache中，将last_processed_instruction序号对应消息保存在buf中，
	//删除（last_processed_instruction之前的信息）
    int check_recv (char* buf, int * len, bool leave_in_queue, sockaddr_in* addrp) 
	{
        if (in_cache_length <= 0)
		{
			//将收到的信息buffer存放在incache
            char buff      [0x8000];
            int  bufflen = 0x8000-1;
            memset(buff, 0, 0x8000);
			//
            if (k_socket::check_recv(buff, &bufflen, false, addrp) == 0) 
			{
                char instruction_count = *buff;  //消息的数量
                char* ptr = buff + 1;
                if (instruction_count != 0) 
				{
                    for (int u=0; u<instruction_count; u++) 
					{
                        unsigned short serial = ((k_instruction_head*)ptr)->serial;
                        unsigned short length = ((k_instruction_head*)ptr)->length;
                        if (serial == last_processed_instruction-1)
                            break;
                        if (in_cache_length > 0)
						{
							int v;
                            for ( v = 0; v < in_cache_length; v++)
							{
                                if (in_cache[v].head.serial == serial)
								{
                                    break;
                                }
                            }
                            if (in_cache[v].head.serial == last_processed_instruction)
							{
                                continue;
                            }
                        }
                        void * buffer;
						//0x100 = 256
                        if (in_cache_length == 0x100) 
						{
                            buffer = length <= in_cache[0].head.length? in_cache[0].body : realloc(in_cache[0].body, length);
                            memcpy(&in_cache[0], &in_cache[1], 255 * sizeof(k_instruction_ptr));
                            in_cache_length--;
                        }
						else
						{
                            buffer = malloc(length);
                        }
                        memcpy(buffer, ptr + 4, length);
                        in_cache[in_cache_length].head.serial = serial;
                        in_cache[in_cache_length].head.length = length;
                        in_cache[in_cache_length].body = (char*)buffer;
                        in_cache_length++;
                        ptr += length + 4;
                    }
                }
            }
        }
		//将没处理的消息清掉，in_cache[last_processed_instruction]之前的消息清空
        if (in_cache_length > 0)
		{
            for (int i = 0; i < in_cache_length; i++)
			{
                if (in_cache[i].head.serial == last_processed_instruction)
				{
                    *len = in_cache[i].head.length;
                    memcpy(buf, in_cache[i].body, *len);
                    if(!leave_in_queue)
                        last_processed_instruction++;
                    if(in_cache_length > 0)
					{
                        for (int j=0; j < in_cache_length; j++)
						{
                            if (in_cache[j].head.serial < last_processed_instruction)
							{
                                ::free(in_cache[j].body);
                                memcpy(&in_cache[j], &in_cache[j+1], (in_cache_length - j) * 8 - 8);
                                in_cache_length--;
                                j--;
                            }
                        }
                    }
                    return 0;
                }
            }
        }
        return 1;
    }
    bool has_data()
	{
        if (in_cache_length == 0)
            return has_data_waiting;
        else
            return 1;
    }
    void resend_message(int limit)
	{
        send_message(limit);
    }
};
