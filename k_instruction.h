/******************************************************************************
***N***N**Y***Y**X***X*********************************************************
***NN**N***Y*Y****X*X**********************************************************
***N*N*N****Y******X***********************************************************
***N**NN****Y*****X*X************************ Make Date  : Jul 01, 2007 *******
***N***N****Y****X***X*********************** Last Update: Jul 01, 2007 *******
******************************************************************************/
#pragma once
extern void OutputHex(char * outb, const void * inb, int len, int res1, bool usespace);
#include <memory.h>
#ifndef min
#define min(a,b) ((a<b)? a:b)
#endif
#ifndef max
#define max(a,b) ((a>b)? a:b)
#endif
///////////////////////////////////////////////////////////////////////////////
typedef enum INSTRUCTION{
	INVDNONE, USERLEAV, USERJOIN, USERLOGN, LONGSUCC, SERVPING, USERPONG, PARTCHAT,
		GAMECHAT, TMOUTRST, GAMEMAKE, GAMRLEAV, GAMRJOIN, GAMRSLST, GAMESTAT, GAMRKICK,
		GAMESHUT, GAMEBEGN, GAMEDATA, GAMCDATA, GAMRDROP, GAMRSRDY, LOGNSTAT, MOTDLINE
} ;
/////////////////////////////////////////////////////////////////////////////*/
//0x00
#define INSTRUCTION_CLNTSKIP INVDNONE
#define INSTRUCTION_USERLEAV USERLEAV
#define INSTRUCTION_USERJOIN USERJOIN
#define INSTRUCTION_USERLOGN USERLOGN
#define INSTRUCTION_LONGSUCC LONGSUCC
#define INSTRUCTION_SERVPING SERVPING
#define INSTRUCTION_USERPONG USERPONG
#define INSTRUCTION_PARTCHAT PARTCHAT
#define INSTRUCTION_GAMECHAT GAMECHAT
#define INSTRUCTION_TMOUTRST TMOUTRST
#define INSTRUCTION_GAMEMAKE GAMEMAKE
#define INSTRUCTION_GAMRLEAV GAMRLEAV
#define INSTRUCTION_GAMRJOIN GAMRJOIN
#define INSTRUCTION_GAMRSLST GAMRSLST
#define INSTRUCTION_GAMESTAT GAMESTAT
#define INSTRUCTION_GAMRKICK GAMRKICK
#define INSTRUCTION_GAMESHUT GAMESHUT
#define INSTRUCTION_GAMEBEGN GAMEBEGN
#define INSTRUCTION_GAMEDATA GAMEDATA
#define INSTRUCTION_GAMCDATA GAMCDATA
#define INSTRUCTION_GAMRDROP GAMRDROP
#define INSTRUCTION_GAMRSRDY GAMRSRDY
#define INSTRUCTION_LOGNSTAT LOGNSTAT
#define INSTRUCTION_MOTDCHAT MOTDLINE
#define MSTR(X) #X

//内存字节对齐，将当前默认字节入栈，并且将1设置为pack_value默认对齐字节。
#pragma pack(push, 1)
//当前栈顶record（pack_value）为新的packing alignment数值
#pragma pack(pop)


//Programs that use intrinsic functions are faster because they do not have the overhead of function calls 
//but may be larger due to the additional code generated.

//指定参数列表中的函数为内建函数
//The compiler may call the function and not replace the function call with inline instructions, if it will result in better performance.
#pragma intrinsic(memcmp, memcpy, memset, strcat, strcmp, strcpy, strlen)

class k_instruction 
{
public:
	INSTRUCTION type: 8;   //指令类型
    char user[32];         //用户名
	char * buffer;
    unsigned int buffer_len;
    unsigned int buffer_pos;
    k_instruction()
	{
        type = INVDNONE;
        user[0] = 0;
        buffer_pos = 0;
        buffer_len = 16;
        buffer = (char*)malloc(buffer_len);
        *buffer = 0x00;
    }
    ~k_instruction()
	{
        free (buffer);
    }
    void clone(k_instruction * arg_0)
	{
        type = arg_0->type;
        strcpy(user, arg_0->user);
        buffer_len = arg_0->buffer_len;
        buffer = (char*)malloc(buffer_len);
        memcpy(buffer, arg_0->buffer, buffer_len);
        buffer_pos = arg_0->buffer_pos;
    }
    void ensure_sized(unsigned int arg_0)
	{
        if (arg_0 > buffer_len) 
		{
            for (;buffer_len < arg_0; buffer_len*=2);
            buffer = (char*)realloc(buffer, buffer_len);
        }
    }
    void set_username(char * arg_0)
	{
        int p;
        strncpy(user, arg_0, (p=min((int)strlen(arg_0), 31)));
        user[p] = 0x00;
    }
	//将arg_0内容加入到缓冲区buffer
    void store_bytes(const void * arg_0, int arg_4)
	{
        ensure_sized(buffer_pos+arg_4);
        memcpy(buffer+buffer_pos, arg_0, arg_4);
        buffer_pos += arg_4;
    }
	//从buffer中加载内容到arg_0,并且buffer自身减去arg_4
    void load_bytes(void * arg_0, unsigned int arg_4)
	{
        if (buffer_pos != 0) 
		{
            int p = min(arg_4, buffer_pos);
            memcpy(arg_0, buffer, p);
            buffer_pos -= p;
            memcpy(buffer, buffer+p, buffer_pos);
        }
    }
    void store_string(const char * arg_0)
	{
        store_bytes(arg_0, (int)strlen(arg_0)+1);
    }
    void load_str(char * arg_0, unsigned int arg_4)
	{
		arg_4 = min(arg_4, strlen(buffer)+1);
        arg_4 = min(arg_4, buffer_pos+1);
		load_bytes(arg_0, arg_4);
		arg_0[arg_4] = 0x00;
    }
    void store_int(const int x)
	{
        store_bytes(&x, 4);
    }
	int load_int()
	{
        int x;
        load_bytes(&x,4);
        return x;
    }
    void store_short(const short x)
	{
        store_bytes(&x, 2);
    }
    void store_char(const char x)
	{
        store_bytes(&x, 1);
    }
    unsigned char load_char()
	{
        unsigned char x;
        load_bytes(&x,1);
        return x;
    }
	//加载2个字节的内容，
    short load_short()
	{
        short x;
        load_bytes(&x,2);
        return x;
    }
	//将type，user，buffer信息写入到arg_0, 然后返回实际写入的长度
    int write_to_message(char * arg_0, const unsigned int max_len)
	{
        *arg_0 = type;
		int eax = (int)strlen(user) + 2;
        strcpy(arg_0 + 1, user);
		int ebx;
        memcpy(arg_0 + eax, buffer, ebx = min(max_len - eax, buffer_pos));
		return eax + ebx;
    }
	//将数据p_buffer写入到buffer中（包含type）
    void read_from_message(char * p_buffer, int p_buffer_len)
	{
        type = *(INSTRUCTION*)p_buffer++;
		
		unsigned int ul = (int)strlen(p_buffer);
		int px = min(ul, 31);
		
		memcpy(user, p_buffer, px+1);
		user[px] = 0;
		p_buffer += ul + 1; 
		
        p_buffer_len -= (ul + 1);
		
        ensure_sized(p_buffer_len);
        memcpy(buffer, p_buffer, p_buffer_len);
		buffer_pos = p_buffer_len;
    }
	//包含了type, user, buffer_len,xxx
	char* to_string(char *buf)
	{
		char * INSTRUCTION_STR[] ={
			MSTR(CLNTSKIP),
				MSTR(USERLEAV),
				MSTR(USERJOIN),
				MSTR(USERLOGN),
				MSTR(LONGSUCC),
				MSTR(SERVPING),
				MSTR(USERPONG),
				MSTR(PARTCHAT),
				MSTR(GAMECHAT),
				MSTR(TMOUTRST),
				MSTR(GAMEMAKE),
				MSTR(GAMRLEAV),
				MSTR(GAMRJOIN),
				MSTR(GAMRSLST),
				MSTR(GAMESTAT),
				MSTR(GAMRKICK),
				MSTR(GAMESHUT),
				MSTR(GAMEBEGN),
				MSTR(GAMEDATA),
				MSTR(GAMCDATA),
				MSTR(GAMRDROP),
				MSTR(GAMRSRDY),
				MSTR(LOGNSTAT),
				MSTR(MOTDCHAT)
		};
		char xxx[2000];
		OutputHex(xxx, buffer, buffer_len, 100, true);
		sprintf(buf, "k_instruction {\n\ttype: %s;\n\tusername: %s;\n\tlen: %i;\n\tcontent:%s;\n};", INSTRUCTION_STR[type], user, buffer_len, xxx);
		return buf;
	}
	void to_string()
	{
		/*
		char * INSTRUCTION_STR[] = {
			MSTR(CLNTSKIP),
				MSTR(USERLEAV),
				MSTR(USERJOIN),
				MSTR(USERLOGN),
				MSTR(LONGSUCC),
				MSTR(SERVPING),
				MSTR(USERPONG),
				MSTR(PARTCHAT),
				MSTR(GAMECHAT),
				MSTR(TMOUTRST),
				MSTR(GAMEMAKE),
				MSTR(GAMRLEAV),
				MSTR(GAMRJOIN),
				MSTR(GAMRSLST),
				MSTR(GAMESTAT),
				MSTR(GAMRKICK),
				MSTR(GAMESHUT),
				MSTR(GAMEBEGN),
				MSTR(GAMEDATA),
				MSTR(GAMCDATA),
				MSTR(GAMRDROP),
				MSTR(GAMRSRDY),
				MSTR(LOGNSTAT),
				MSTR(MOTDCHAT)
		};
		char xxx[2000];
		OutputHex(xxx, buffer, buffer_len, 100, true);
		kprintf("k_instruction \n\ttype: %s;\n\tusername: %s;\n\tlen: %i;\n\tcontent:%s;\n};", INSTRUCTION_STR[type], user, buffer_len, xxx);
		*/
	}
};
