#include <stdio.h>
#include "BaseThread.h"
#include "ThreadPool.h"
#include "public.h"
#include "Epoll/epoll.h"
#include "Socket/Socket.h"

#define SEND_SIZE 100
#define RECV_SIZE sizeof(VideoList_t)+sizeof(PackHead_t)+sizeof(PackTail_t)

int connect_count = 0;
int suc_login = 0;
int faild_login = 0;

pthread_mutex_t connect_mutex;
pthread_mutex_t suc_mutex;
pthread_mutex_t faild_mutex;


//发送数据线程
class SendThread : public CBaseThread
{
public:
	SendThread(CTcpSocket *beginSock, int sockCount, bool bSuc);
	~SendThread();
private:
	int run();
	CTcpSocket *m_beginSock;//需要发送数据的socket头指针
	int m_sockCount;		//发送数据的socket的数量
	bool m_bSuc;
};

SendThread::SendThread(CTcpSocket *beginSock, int sockCount, bool bSuc)
	:m_beginSock(beginSock),
	m_sockCount(sockCount),
	m_bSuc(bSuc)
{

}

SendThread::~SendThread()
{

}

int SendThread::run()
{
	PackTail_t tail;
	tail.pack_tail = 55;
	PackHead_t head;
	memset(&head,0,sizeof(head));
	head.func_num = 2004;
	head.pack_size = 0;
	//head.pack_size = sizeof(Login_t);
	head.counts = 1000;
	Login_t login;
	if (m_bSuc)
	{
		//模拟成功的数据
		strcpy(login.user_name,"admin");
		strcpy(login.passwd,"123456");
	}
	else
	{
		//模拟失败
		strcpy(login.user_name,"AAA");
		strcpy(login.passwd,"AAA");
	}
	char buf[SEND_SIZE] = {0};
	memcpy(buf, &head, sizeof(PackHead_t));
	//memcpy(buf + sizeof(PackHead_t), &login, sizeof(Login_t));
	//memcpy(buf + sizeof(PackHead_t)+sizeof(Login_t),&tail,sizeof(PackTail_t));
	memcpy(buf+sizeof(PackHead_t),&tail,sizeof(tail));
	while (1)
	{	
		for (int i = 0; i < m_sockCount; i++)
		{
			int wdsize = (m_beginSock + i)->Write(buf, SEND_SIZE);
		}
		usleep(100000);
	}
}

//接收的线程
class RecvThread : public CBaseThread
{
public:
	RecvThread(CTcpSocket *beginSock, int sockCount);
	~RecvThread();
private:
	int run();
	CTcpSocket *m_beginSock;//需要发送数据的socket头指针
	int m_sockCount;		//发送数据的socket的数量
};

RecvThread::RecvThread(CTcpSocket *beginSock, int sockCount)
	:m_beginSock(beginSock),
	m_sockCount(sockCount)
{

}

RecvThread::~RecvThread()
{

}

int RecvThread::run()
{
	
	int epid = epoll_create(100);
	struct epoll_event event;
	memset(&event, 0, sizeof(event));
	for (int i = 0; i < m_sockCount; i++)
	{
		event.data.fd = (m_beginSock+i)->GetSocket();
		event.events  = EPOLLIN;	//设置事件为EPOLLIN，可读的事件
		epoll_ctl(epid, EPOLL_CTL_ADD, (m_beginSock+i)->GetSocket(), &event); //EPOLL_CTL_ADD添加事件和对应的fd到监听队列
	}

	struct epoll_event return_events[20] = {0};
	char buf[RECV_SIZE] = {0};

	while(1)
	{
		memset(return_events,0,sizeof(return_events));
		int nevent = epoll_wait(epid, return_events, 20, -1);
		for (int i = 0; i < nevent; i++)
		{
			if (return_events[i].events & EPOLLIN)
			{
				memset(buf, 0, RECV_SIZE);
				int rdsize = read(return_events[i].data.fd, buf, RECV_SIZE);
				if (rdsize > 0)
				{
					
					PackHead_t head;
					memcpy(&head, buf, sizeof(PackHead_t));
					if (head.func_num == 1002)
					{
						LoginRet_t ret;
						memcpy(&ret, buf + sizeof(PackHead_t), sizeof(LoginRet_t));
						if (ret.login_ret == 0)
						{
							//成功
							pthread_mutex_lock(&suc_mutex);
							suc_login++;
							pthread_mutex_unlock(&suc_mutex);

						}
						else
						{
							//失败
							pthread_mutex_lock(&faild_mutex);
							faild_login++;
							pthread_mutex_unlock(&faild_mutex);
						}
					}
					else if (head.func_num == 2004)
					{
						pthread_mutex_lock(&suc_mutex);
						suc_login++;
						pthread_mutex_unlock(&suc_mutex);
					}
					else if (head.func_num == 2002)
					{
						pthread_mutex_lock(&suc_mutex);
						suc_login++;
						pthread_mutex_unlock(&suc_mutex);
					}
					else if (head.func_num == 2001)
					{
						pthread_mutex_lock(&suc_mutex);
						suc_login++;
						pthread_mutex_unlock(&suc_mutex);
					}
					else if (head.func_num == 2003)
					{
						pthread_mutex_lock(&suc_mutex);
						suc_login++;
						pthread_mutex_unlock(&suc_mutex);
					}
				}
			
			}
		}
	}
}

//显示线程
class PrintThread : public CBaseThread
{
public:
	PrintThread(){};
	~PrintThread(){}
private:
	int run();
};

int PrintThread::run()
{
	while(1)
	{
		printf("connect : %d, suc %d, faild %d\n", connect_count, suc_login, faild_login);
		sleep(1);
	}
}


int main()
{
	PrintThread printthr;
	printthr.start();

	int counts = 1000;
	CTcpSocket tcpclient[counts];

	for (int i = 0; i < counts; i++)
	{
		tcpclient[i].Create();
		int ret= tcpclient[i].Connect(CHostAddress("127.0.0.1", 5555));
		if (ret == -1)
		{
			printf("canot\n");
		}
		else
			connect_count++;
	}

	printf("start send.................\n");
	//创建10个接收的线程
	for (int i = 0; i < (counts/100); i++)
	{
		RecvThread *recvthr = new RecvThread(&tcpclient[i*100], 100);
		recvthr->start();
	}

	//创建10个发送数据线程
	for(int i = 0; i < (counts/100); i++)
	{
		bool bSuc = true;
		SendThread *sendthr = new SendThread(&tcpclient[i*100], 100, bSuc);
		sendthr->start();
	}

	pause();
	return 0;
}