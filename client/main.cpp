#include "Socket/Socket.h"
#include "ThreadPool/BaseThread.h"
#include "public.h"
#include "Epoll/epoll.h"
#include <stdio.h>
#include  <stdlib.h>
#include <unistd.h>

#define  SEND_SIZE 100
#define  RECV_SIZE sizeof(VideoList_t)*18+sizeof(PackHead_t)
class CPantThr :public CBaseThread
{
public:
	CPantThr(int fd);
	int run();
private:
	int fd;
};

CPantThr::CPantThr(int fd)
{
	this->fd = fd;
}

int CPantThr::run()
{
	PackHead_t head;
	memset(&head,0,sizeof(head));
	PackTail_t tail;
	tail.pack_tail = 55;
	head.func_num = 1001;
	char buf[100] = {0};
	memcpy(buf,&head,sizeof(head));
	memcpy(buf+sizeof(head),&tail,sizeof(tail));
	while (1)
	{
		send(fd,buf,100,0);
		sleep(1);
	}
}


class CMyThr :public CBaseThread
{
public:
	CMyThr(int fd);
	int run();
private:
	CTcpSocket client;
};

CMyThr::CMyThr(int fd)
{
	client.SetSocket(fd);
}



int CMyThr::run()
{
	char buf[SEND_SIZE] = {0};
	memset(buf,0,SEND_SIZE);
	PackHead_t head;
	memset(&head,0,sizeof(head));
	head.func_num = 1002;
	head.counts = 0;
	head.pack_size = 0;
	head.des_fd = 2;
	PackTail_t tail;
	tail.pack_tail = 55;

	//Login_t login;
	//while(1)
	//{
	//	memset(&login,0,sizeof(login));
	//	printf("name : ");
	//	scanf("%s",login.user_name);
	//	printf("passwd : ");
	//	scanf("%s",login.passwd);
	//	memset(&buf,0,sizeof(buf));
	//	head.pack_size = sizeof(Login_t);
	//	head.counts = 1;
	//	memcpy(buf,&head,sizeof(head));
	//	memcpy(buf+sizeof(PackHead_t),&login,sizeof(login));
	//	memcpy(buf+sizeof(PackHead_t)+sizeof(Login_t),&tail,sizeof(PackTail_t));
	//	client.Write(buf,100);
	//}


	memset(buf,0,SEND_SIZE);
	head.func_num = 2001;
	memcpy(buf,&head,sizeof(head));
	memcpy(buf+sizeof(PackHead_t),&tail,sizeof(PackTail_t));
	client.Write(buf,SEND_SIZE);

	memset(buf,0,SEND_SIZE);
	head.func_num = 2002;
	memcpy(buf,&head,sizeof(head));
	memcpy(buf+sizeof(PackHead_t),&tail,sizeof(PackTail_t));
	client.Write(buf,SEND_SIZE);

	memset(buf,0,SEND_SIZE);
	head.func_num = 2003;
	memcpy(buf,&head,sizeof(head));
	memcpy(buf+sizeof(PackHead_t),&tail,sizeof(PackTail_t));
	client.Write(buf,SEND_SIZE);

	memset(buf,0,SEND_SIZE);
	head.func_num = 2004;
	memcpy(buf,&head,sizeof(head));
	memcpy(buf+sizeof(PackHead_t),&tail,sizeof(PackTail_t));
	client.Write(buf,SEND_SIZE);

	//head.func_num = 4001;
	//memcpy(buf,&head,sizeof(head));
	//memcpy(buf+sizeof(PackHead_t),&tail,sizeof(PackTail_t));
	//client.Write(buf,SEND_SIZE);

	//sleep(2);

	//VideoRecord_t time;
	//memset(&time,0,sizeof(time));
	//head.func_num = 3002;
	//head.pack_size = sizeof(VideoRecord_t);
	//time.user_id = 2;
	//time.video_id = 1005;
	//time.video_seek = 999;
	//memcpy(buf,&head,sizeof(head));
	//memcpy(buf+sizeof(head),&time,sizeof(time));
	//memcpy(buf+sizeof(head)+head.pack_size,&tail,sizeof(tail));
	//client.Write(buf,SEND_SIZE);
	//
	//VideoPlay_t play;
	//memset(&play,0,sizeof(play));
	//play.video_id = 1001;
	//head.func_num = 3001;
	//head.counts = 1;
	//head.pack_size = sizeof(VideoPlay_t);
	//memcpy(buf,&head,sizeof(head));
	//memcpy(buf+sizeof(head),&play,sizeof(play));
	//memcpy(buf+sizeof(head)+head.pack_size,&tail,sizeof(tail));
	//client.Write(buf,SEND_SIZE);
	//sleep(2);
	//memset(&head,0,sizeof(head));
	//head.func_num = 4001;
	//head.des_fd = 2;
	//head.pack_size = 0;
	//memcpy(buf,&head,sizeof(head));
	//memcpy(buf+sizeof(PackHead_t),&tail,sizeof(PackTail_t));
	//client.Write(buf,SEND_SIZE);

	return 0;
}


int main()
{
	CTcpSocket client;
	client.Create();
	int ret = client.Connect(CHostAddress("127.0.0.1",5555));
	if (ret == 0)
	{
		printf("connect success\n");
		CPantThr heart(client.GetSocket());
		heart.start();
	}
	else if (ret == -1)
	{
		perror("connect error");
		exit(-1);
	}
	CEpoll cliepoll(1024);
	struct epoll_event ev_read[20];
	cliepoll.AddEvent(client.GetSocket());

	CMyThr thr(client.GetSocket());
	thr.start();

	int i;
	char buf[RECV_SIZE] = {0};
	PackHead_t head;

	while(1)
	{
		memset(ev_read,0,sizeof(ev_read));
		int nfds = cliepoll.Wait(ev_read,20);

		for (i = 0;i < nfds;i++)
		{
			memset(buf,0,RECV_SIZE);
			int rdsize = client.Read(buf,RECV_SIZE);
			if (rdsize > 0)
			{
				memset(&head,0,sizeof(head));
				memcpy(&head,buf,sizeof(head));

				printf("recv pack head:%d\n",head.func_num);
				int j;
				switch(head.func_num)
				{
				case 1002:
					{
						LoginRet_t data;
						memset(&data,0,sizeof(data));
						memcpy(&data,buf+sizeof(head),sizeof(data));
						if (data.login_ret == 0)
						{
							printf("login success,user_id = %d\n",data.user_id);
						}
						else
							printf("login faided,user_id = %d\n",data.user_id);
											
						break;
					}

				case 2001:
					{
						
						VideoChannel_t data;
						for (j = 0;j < head.counts;j++)
						{
							memset(&data,0,sizeof(data));
							memcpy(&data,buf+sizeof(head)+sizeof(data)*j,sizeof(data));
							printf("%s\n",data.channel_name);
						}
						break;
					}

				case 2002:
					{
						VideoType_t data;
						for (j = 0;j < head.counts;j++)
						{
							memset(&data,0,sizeof(data));
							memcpy(&data,buf+sizeof(head)+sizeof(data)*j,sizeof(data));
							printf("%s\n",data.type_name);
						}
						break;
					}
				case 2003:
					{
						VideoArea_t data;
						for (j = 0;j < head.counts;j++)
						{
							memset(&data,0,sizeof(data));
							memcpy(&data,buf+sizeof(head)+sizeof(data)*j,sizeof(data));
							printf("%s\n",data.area_name);
						}
						break;
					}
				case 2004:
					{
						VideoList_t data;
						for (j = 0;j < head.counts;j++)
						{
							memset(&data,0,sizeof(data));
							memcpy(&data,buf+sizeof(head)+sizeof(data)*j,sizeof(data));
							printf("%s %d\n",data.video_name,data.play_times);
						}
						break;
					}
				case 3001:
				case 3002:
					{
						UpLoad_t ack;
						memcpy(&ack,buf+sizeof(head),sizeof(ack));
						if (ack.results == 0)
						{
							printf("upload data success.\n");
						}
						else
							printf("upload fail\n");
						break;
					}
				case 4001:
					{
						PlayHistory_t data;
						for (j = 0;j < head.counts;j++)
						{
							memset(&data,0,sizeof(data));
							memcpy(&data,buf+sizeof(head)+sizeof(data)*j,sizeof(data));
							printf("%s %dsec\n",data.video_name,data.video_seek);
						}
						break;
					}
				}
				
			}
			else if (rdsize == 0)
			{
				printf("client stop!\n");
				close(client.GetSocket());
				exit(0);
			}
		}
		
	}
	return 0;
}