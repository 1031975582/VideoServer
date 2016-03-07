#ifndef _TASK_H_
#define _TASK_H_

#include <termios.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "public.h"
#include "ShareMemory/sem.h"
#include "ShareMemory/ShareMemory.h"
#include "DbSingles/DbCon.h"
#include "ThreadPool/BaseThread.h"
#include "ThreadPool/ThreadPool.h"
#include "Epoll/epoll.h"

typedef struct Pant//�����ṹ��
{
	int fd; //�ͻ���fd
	bool flag;//����״̬��true-���ߣ�flase-����
}Pant_t;

typedef struct Service//��������ҵ���
{
	int func_num;//�����ܺ�
	int client_fd;//�ͷ���fd
	int user_id;  //�û�ID
	int video_id; //��ƵID
	int video_seek; //����ʱ��
}Service_t;


#define  BUFSIZE 100
#define  LISTSIZE sizeof(VideoList_t)*18+sizeof(PackHead_t)
#define  BLOCKSIZE sizeof(Service_t)
#define  SHM1_SIZE BLOCKSIZE*2000+sizeof(int)
#define  SHM2_SIZE BLOCKSIZE*4000+sizeof(int)
#define  SHM1_COUNT 2000
#define  SHM2_COUNT SHM2_SIZE-sizeof(int)
#define  SHMKEY1 (key_t)0001
#define  SHMKEY2 (key_t)0002
#define  SEMKEY (key_t)0003


class CAnalyTask :public CTask//��������
{
public:
	CAnalyTask(char *rd_buf,int fd);
	int run();
private:
	char buf[BUFSIZE];
	int des_fd;
};


class CHandleTask :public CTask//ҵ����
{
public:
	CHandleTask(char *buf);
	int run();
private:
	Service_t server;
};

class CReplyThread :public CBaseThread//����Ӧ����߳�
{
public:
	int run();
};

class CSendThread :public CBaseThread//����Ӧ����߳�
{
public:
	CSendThread(int size,char *buf);
	int run();
private:
	CSendThread *self;
	int size;
	char buf[SHM2_COUNT];
};

class CLogThread :public CBaseThread//ʵʱ��־�߳�
{
public:
	int run();
	static int log_fd;
	static int links;
	static int recv_packs;
	static int send_packs;
	static int logins;
	static int channels;
	static int types;
	static int areas;
	static int lists;
	static int plays;
	static int play_times;
	static int pants;
};

class CPantThread :public CBaseThread//�����߳�
{
public:
	CPantThread(CEpoll *epoll);
	int run();
private:
	CEpoll *epoll;
};

void log_enab(int num);

void log_write(int num,char *buf,int user_id);//��־���ɺ���

void char_hex(char *buf,char *GetLog);

void init_server();//������������ʼ������

void init_db();

void shm1_read(CThreadPool *back_pool);//����ҵ����ĺ���

int put_num(int counts);//�ն��ַ�������ƺ���

void sigint_fun(int sig);

#endif