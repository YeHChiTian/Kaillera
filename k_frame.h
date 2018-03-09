#pragma once



//ά��һ����СΪsize��buffer������
#include <memory>
class k_frame 
{
public:
	char * buffer;
	int pos;
	int size;
	k_frame(){
		pos = 0;
		buffer = 0;
		size = 0;
	}
	k_frame(char * buf, int len)
	{
		pos=size= len;
		buffer = (char*)malloc(len);
		memcpy(buffer, buf, len);
	}
	~k_frame()
	{
		//kprintf("DISTRUCTOR!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! %x", this);
		if (buffer != 0)
		{
			free(buffer);
			buffer=0;
			pos=0;
			size=0;
		}
	}

	void put_data(void * data, int datalen) 
	{
 		//to_string("put_datab");
		//kprintf("pdlsreq %i", pos+datalen);
		if(datalen > 0) 
		{			
			ensure_sized(pos+datalen);
			memcpy(buffer + pos, data, datalen);
			pos += datalen;
		}
		//kprintf("params l: %i", datalen);
		//to_string("put_data");
	}
	//��������buffer���ݿ�����datab��,��СΪmin(len, pos)
	int peek_data(char * datab, int len)
	{
		int x;
		if ((x = min(len, pos)) > 0) 
		{
			memcpy(datab, buffer, x);
			//to_string("peek_data");
			return x;
		}
		return 0;
	}
	//�ж����ݳ����Ƿ����㣬���������ռ�
	void ensure_sized(int datalen)
	{
		//to_string("ensure_sized");
		if (buffer == 0){
		    //to_string("ensure_sized_naloc");
			size = datalen * 6;
			buffer = (char*)malloc(size);
		} else
		{
		    //to_string("ensure_sized_paloc");
			int ecx = datalen;
			if (datalen > size) 
			{
				size = datalen + 1;
				//to_string("ensure_sized_paloc2");
				buffer = (char*)realloc(buffer, size);
				//to_string("ensure_sized_paloc3");
			}
		}
		//to_string("ensured_sized");
	}
	//�ڻ������п������ݵ�datab,�����ƶ�������buffer��λ��
	int get_data(char * datab, int len)
	{
		int x;
		//to_string("get_datap");
		if ((x = min(len, pos)) > 0)
		{
			memcpy(datab, buffer, x);
			pos -= x;
			memcpy(buffer, buffer+x, pos);
			//to_string("get_data");
			return x;
		}
		return 0;
	}
	void reset()
	{
		pos = 0;
	}
	void to_string(char * pre)
	{
		/*
		char xxx[200];
		OutputHex(xxx, buffer, pos, 0, 0);
		kprintf("k_frame(%x)::%s: %s, si=%i, le=%i", this, pre, xxx, size, pos);*/
	}
};
