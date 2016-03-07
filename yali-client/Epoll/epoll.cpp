#include <string.h>
#include <stdio.h>
#include "../Epoll/epoll.h"

CEpoll::CEpoll(int size)
{
	epfd = epoll_create(size);
}

void CEpoll::AddEvent(int sockfd)
{
	struct epoll_event epevent;
	memset(&epevent,0,sizeof(epevent));
	epevent.data.fd = sockfd;
	epevent.events = EPOLLIN;
	epoll_ctl(epfd,EPOLL_CTL_ADD,sockfd,&epevent);// ��Ӽ����¼���epoll��
}

void CEpoll::DelEvent(int sockfd)
{
	struct epoll_event epevent;
	memset(&epevent,0,sizeof(epevent));
	epevent.data.fd = sockfd;
	epevent.events = EPOLLIN;
	epoll_ctl(epfd,EPOLL_CTL_DEL,sockfd,&epevent);// ��epoll�Ƴ������¼�
}

int CEpoll::Wait(struct epoll_event *ev_read,int counts)
{
	int nfds;
	nfds = epoll_wait(epfd,ev_read,counts,-1);// �����ȴ��¼����������ط����¼��ĸ���
	if (nfds == 0)
	{
		printf("epoll wait timeout\n");
	}
	return nfds;
}

int CEpoll::Getepoll()
{
	return epfd;
}