#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "Socket/Socket.h"
#include "public.h"
#include "task.h"

int max_con = 500;
int max_thr = 100;

CEpoll epoll(1024);

void sigpipe_fun(int sig)//socket�Ͽ�,ϵͳ����SIGPIPE�źŴ�����
{
	int counts = CLogThread::links;
	int i = 0;
	int fd = 7;
	for (;i<counts;i++)
	{
		epoll.DelEvent(fd);
		close(fd);
		fd++;
		if (CLogThread::links > 0)
		{
			CLogThread::links--;
		}	
	}
}


int main()
{

	signal(SIGPIPE,sigpipe_fun);//����SIGPIPE�ź�
	signal(SIGINT,sigint_fun);
	init_server();
	init_db();
// 	printf("Ĭ�����ݿ��ļ���Db/mydb.dat\n");
// 	printf("Ĭ�Ϸ������˿ںţ�5555\n");
// 	printf("�������������������1000����");
// 	max_con = put_num(4);
// 	printf("���������߳������ޣ�����100����");
// 	max_thr = put_num(3);
	
	CTcpServer server;
 	server.Create();
 	int b_ret = server.Bind(CHostAddress("0.0.0.0",5555));
 	if (b_ret == -1)
 	{
 		perror("bind error:");
 		exit(-1);
 	}
 	server.Listen(1024);
 	int pid = fork();
 /*ǰ�÷�����*/
 	if (pid > 0)
 	{
		struct epoll_event ev_read[40];
		epoll.AddEvent(server.GetSocket());
 		CReplyThread reply;
 		reply.start();//���м���Ӧ����߳�

		CThreadPool pre_pool(max_thr,30);
 		pre_pool.start(20);//�����̳߳�
 
		CLogThread log;
		log.start();//����ʵʱ��־�߳�

		CPantThread pant(&epoll);//�������������߳�
		pant.start();

		int i;
 		CTcpSocket tcpclient;
 		char rd_buf[BUFSIZE] = {0};//���ݽ��ջ���
		PackHead_t head;
		PackTail_t tail;
 		while(1)
 		{
 			memset(ev_read,0,sizeof(struct epoll_event)*40);
 			int nfds = epoll.Wait(ev_read,40);//epoll����
 			for (i = 0;i < nfds;i++)
 			{
 				int fd = ev_read[i].data.fd;
 				if (ev_read[i].data.fd == server.GetSocket())//�����µ�����
 				{
					if (CLogThread::links == 1010)
					{
						continue;
					}
 					tcpclient = server.Accept();
 					epoll.AddEvent(tcpclient.GetSocket());
					CLogThread::links++;
 				}
 				else if (ev_read[i].events & EPOLLIN)//�ͻ����пɶ��¼�
 				{
 					memset(rd_buf,0,BUFSIZE);
 					int rd_size = recv(fd,rd_buf,BUFSIZE,0);//�ӿͻ���socket��ȡ���ݣ�������֤����������
 					if (rd_size > 0)
 					{
 						memset(&head,0,sizeof(head));
 						memcpy(&head,rd_buf,sizeof(PackHead_t));
 						memcpy(&tail,rd_buf+sizeof(PackHead_t)+head.pack_size,sizeof(PackTail_t));
 						if (tail.pack_tail == 55 )
 						{
							log_enab(head.counts);
 							CLogThread::recv_packs++;//���յ����ݰ�����
 							CAnalyTask *task = new CAnalyTask(rd_buf,ev_read[i].data.fd);
 							pre_pool.addTask(task);	
 						}
 					}
 					else//�ͻ����˳�����
 					{
 						epoll.DelEvent(ev_read[i].data.fd);
 						close(ev_read[i].data.fd);
						if (CLogThread::links > 0)
						{
							CLogThread::links--;
						}				
 					}
 				}
 			}
 		}
 	}
 	/*���÷�����*/
 	else if (pid == 0)
 	{
 		CThreadPool back_pool(max_thr,30);
 		back_pool.start(20);//���к��÷��������̳߳�
		shm1_read(&back_pool); //����ҵ����
 	}
	return 0;
}


