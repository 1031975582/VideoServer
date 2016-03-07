#include "task.h"
#include <string.h>
#include <list>
using namespace std;

CShaMemory *shm1;//�����ڴ�1
CShaMemory *shm2;//�����ڴ�2
CSem *sem;//�ź���

list<Login_t> *login;
list<VideoList_t> *msg;//��Ƶ�б�
list<Pant_t> *pant;//��������

char *channel_buf;
char *msg_buf;
char *type_buf;
char *area_buf;

/*ʵʱ��־�Ļ�����*/
pthread_mutex_t log_mutex;
pthread_mutex_t channel_mutex;
pthread_mutex_t type_mutex;
pthread_mutex_t msg_mutex;
pthread_mutex_t play_mutex;
pthread_mutex_t record_mutex;
pthread_mutex_t send_mutex;
pthread_mutex_t pant_mutex;


void sigint_fun(int sig)
{
	//delete shm1;
	//delete shm2;
	//delete sem;
	delete channel_buf;
	delete msg_buf;
	delete type_buf;
	delete area_buf;
	delete login;
	delete msg;
	delete pant;
	exit(0);
}

/********************************************ǰ���̳߳ؽ������ص�����*******************************************************************************************************/

CAnalyTask::CAnalyTask(char *rd_buf,int fd)
{
	memcpy(buf,rd_buf,BUFSIZE);
	this->des_fd = fd;
}

int CAnalyTask::run()
{
	char sql_temp[128] = {0};
	PackHead_t head;
	memset(&head,0,sizeof(head));
	memcpy(&head,buf,sizeof(head));
	Service_t server;
	memset(&server,0,sizeof(Service_t));
	server.client_fd = this->des_fd;
	server.func_num = head.func_num;
	switch(head.func_num)
	{
	case 1001://������
		{
			CLogThread::pants++;
			list<Pant_t>::iterator it = pant->begin();
			for (;it != pant->end();it++)
			{
				if (server.client_fd == (*it).fd && (*it).flag == false)
				{
					(*it).flag = true;
					break;
				}
			}
			if (it == pant->end())
			{
				Pant_t data;
				data.fd = server.client_fd;
				data.flag = true;
				pthread_mutex_lock(&pant_mutex);
				pant->push_back(data);
				pthread_mutex_unlock(&pant_mutex);
			}
			log_write(1001,this->buf,server.client_fd);
			return 0;
		}
	case 1002://��½�����
		{
			pthread_mutex_lock(&log_mutex);
			CLogThread::logins++;
			pthread_mutex_unlock(&log_mutex);
			Login_t data;
			memset(&data,0,sizeof(login));
			memcpy(&data,buf+sizeof(PackHead_t),sizeof(Login_t));
			list<Login_t>::iterator it = login->begin();
			for (;it != login->end();it++)
			{
				if (strcmp(data.user_name,(*it).user_name) == 0 && strcmp(data.passwd,(*it).passwd) == 0)
				{
					server.user_id = (*it).user_id;
					break;
				}
			}
			if (sem->GetVal(2) == 1)
			{
				log_write(1002,this->buf,server.user_id);
			}
			break;
		}
	case 2001://Ƶ�������
		{
			pthread_mutex_lock(&channel_mutex);
			CLogThread::channels++;
			pthread_mutex_unlock(&channel_mutex);
			if (sem->GetVal(2) == 1)
			{
				log_write(2001,this->buf,0);
			}
			break;
		}
	case 2002://���������
		{
			pthread_mutex_lock(&type_mutex);
			CLogThread::types++;
			pthread_mutex_unlock(&type_mutex);
			if (sem->GetVal(2) == 1)
			{
				log_write(2002,this->buf,0);
			}
			break;
		}
	case 2003://���������
		{
			CLogThread::areas++;
			if (sem->GetVal(2) == 1)
			{
				log_write(2003,this->buf,0);
			}
			break;
		}
	case 2004://��Ƶ�б������
		{
			pthread_mutex_lock(&msg_mutex);
			CLogThread::lists++;
			pthread_mutex_unlock(&msg_mutex);
			if (sem->GetVal(2) == 1)
			{
				log_write(2004,this->buf,0);
			}
			break;
		}
	case 3001://��Ƶ�㲥�����
		{
			pthread_mutex_lock(&play_mutex);
			CLogThread::plays++;
			pthread_mutex_unlock(&play_mutex);
			VideoPlay_t data;
			memcpy(&data,buf+sizeof(PackHead_t),sizeof(VideoPlay_t));
			server.user_id = data.user_id;
			server.video_id = data.video_id;
			if (sem->GetVal(2) == 1)
			{
				log_write(3001,this->buf,data.user_id);
			}
			break;
		}
	case 3002://��Ƶ�㲥ʱ�������
		{
			pthread_mutex_lock(&record_mutex);
			CLogThread::play_times++;
			pthread_mutex_unlock(&record_mutex);
			VideoRecord_t data;
			memcpy(&data,buf+sizeof(PackHead_t),sizeof(VideoRecord_t));
			server.video_id = data.video_id;
			server.user_id = data.user_id;
			server.video_seek = data.video_seek;
			if (sem->GetVal(2) == 1)
			{
				log_write(3002,this->buf,data.user_id);
			}
			break;
		}
	case 4001://���󲥷���ʷ
		{
			server.user_id = head.des_fd;
			log_write(4001,this->buf,server.user_id);
			break;
		}
	}
	//���һ�鹲���ڴ�д��������ҵ������Թ����ڴ�Ķ�д����PV����
	sem->Sem_P(0);
	int counts = shm1->get_head();
	if (counts == SHM1_COUNT)
	{
		while(1)
		{
			sem->Sem_V(0);
			usleep(10);
			sem->Sem_P(0);
			counts = shm1->get_head();
			if (counts < SHM1_COUNT)
			break;	
		}
	}
	shm1->write_block(&server);
	sem->Sem_V(0);
	return 0;
}


/********************************************�����̳߳�ҵ����ص�����*******************************************************************************************************/

CHandleTask::CHandleTask(char *buf)
{
	memset(&server,0,sizeof(Service_t));
	memcpy(&server,buf,sizeof(Service_t));
}

int CHandleTask::run()
{
	int wr_len = 0;
	char s_buf[LISTSIZE] = {0};
	memset(s_buf,0,sizeof(s_buf));
	PackHead_t head;
	memset(&head,0,sizeof(head));
	switch(server.func_num)
	{
	case 1002:
		{
			LoginRet_t logret;
			memset(&logret,0,sizeof(logret));
			logret.user_id = server.user_id;
			logret.login_ret = (logret.user_id > 0 ? 0:1);
			head.counts = 1;
			head.pack_size = sizeof(LoginRet_t);
			memcpy(s_buf,&head,sizeof(head));
			memcpy(s_buf+sizeof(PackHead_t),&logret,sizeof(LoginRet_t));
			break;
		}
	case 2001:
		{	
			memcpy(s_buf,channel_buf,LISTSIZE);
			memcpy(&head,channel_buf,sizeof(PackHead_t));
			head.des_fd = server.client_fd;
			memcpy(s_buf,&head,sizeof(PackHead_t));
			break;
		}
	case 2002:
		{	
			memcpy(s_buf,type_buf,LISTSIZE);
			memcpy(&head,type_buf,sizeof(PackHead_t));
			head.des_fd = server.client_fd;
			memcpy(s_buf,&head,sizeof(PackHead_t));
			break;
		}
	case 2003:
		{	
			memcpy(s_buf,area_buf,LISTSIZE);
			memcpy(&head,area_buf,sizeof(PackHead_t));
			head.des_fd = server.client_fd;
			memcpy(s_buf,&head,sizeof(PackHead_t));
			break;
		}
	case 2004:
		{	
			memcpy(s_buf,msg_buf,LISTSIZE);
			memcpy(&head,msg_buf,sizeof(PackHead_t));
			head.des_fd = server.client_fd;
			memcpy(s_buf,&head,sizeof(PackHead_t));
			break;
		}
	case 3001:
		{
			if (server.video_id >= 1000 && server.video_id <= 1017)
			{
				char sql[128] = {0};
				sprintf(sql,"update Tbl_video_message set play_counts = play_counts+1 where video_id = %d",server.video_id);
				(DbSingles::GetSingle())->GetData(sql,NULL,NULL);
				list<VideoList_t>::iterator it = msg->begin();
				for (;it != msg->end();it++)
				{
					if ((*it).video_id == server.video_id)
					{
						(*it).play_times++;
						break;
					}
				}
				UpLoad_t ack;
				ack.results = 0;
				head.counts =1;
				head.pack_size = sizeof(UpLoad_t);
				memcpy(s_buf,&head,sizeof(head));
				memcpy(s_buf+sizeof(PackHead_t),&ack,sizeof(UpLoad_t));
				break;
			}
			else
			{
				UpLoad_t ack;
				ack.results = 1;
				head.counts =1;
				head.pack_size = sizeof(UpLoad_t);
				memcpy(s_buf,&head,sizeof(head));
				memcpy(s_buf+sizeof(PackHead_t),&ack,sizeof(UpLoad_t));
				break;
			}		
		}
	case 3002:
		{
			if (server.video_id >= 1000 && server.video_id <= 1017 && server.user_id > 0)
			{
				char sql[256] = {0};
				char tm[6] = {0};
				sprintf(sql,"select play_times from Tbl_user_play where user_id = %d and video_id = %d",server.user_id,server.video_id);
				(DbSingles::GetSingle())->GetData(sql,List_CallBack,(void *)tm);
				if (atoi(tm) == 0)
				{
					char id[6] = {0};
					sprintf(sql,"select count(play_id) as counts from Tbl_user_play");
					(DbSingles::GetSingle())->GetData(sql,List_CallBack,(void *)id);
					int play_id = atoi(id) + 1;
					sprintf(sql,"insert into Tbl_user_play values(%d,%d,%d,%d,'')",play_id,server.user_id,server.video_id,server.video_seek);
					(DbSingles::GetSingle())->GetData(sql,NULL,NULL);
				}
				else
				{
					sprintf(sql,"update  Tbl_user_play set play_times = %d where user_id = %d and video_id = %d",server.video_seek,server.user_id,server.video_id);
					(DbSingles::GetSingle())->GetData(sql,NULL,NULL);
				}
				UpLoad_t ack;
				ack.results = 0;
				head.counts =1;
				head.pack_size = sizeof(UpLoad_t);
				memcpy(s_buf,&head,sizeof(head));
				memcpy(s_buf+sizeof(PackHead_t),&ack,sizeof(UpLoad_t));
				break;
			}
			else
			{
				UpLoad_t ack;
				ack.results = 1;
				head.counts =1;
				head.pack_size = sizeof(UpLoad_t);
				memcpy(s_buf,&head,sizeof(head));
				memcpy(s_buf+sizeof(PackHead_t),&ack,sizeof(UpLoad_t));
				break;
			}	
		}
	case 4001:
		{
			list<PlayHistory_t> his;
			char sql[128] = {0};
			sprintf(sql,"select video_id,play_times from Tbl_user_play where user_id = %d",server.user_id);
			(DbSingles::GetSingle())->GetData(sql,His_CallBack,(void *)&his);
			list<PlayHistory_t>::iterator it = his.begin();
			for (;it != his.end();it++)
			{
				memcpy(s_buf+sizeof(head)+sizeof(PlayHistory_t)*head.counts,&(*it),sizeof(PlayHistory_t));
				head.counts++;
			}
			head.pack_size = head.counts*sizeof(PlayHistory_t);
			memcpy(s_buf,&head,sizeof(head));	
			break;
		}
	}
	wr_len = head.pack_size + sizeof(PackHead_t);
	sem->Sem_P(1);
	int size = shm2->get_head();
	if ((SHM2_COUNT - size) < wr_len)
	{
		while(1)
		{
			sem->Sem_V(1);
			usleep(10);
			sem->Sem_P(1);
			size = shm2->get_head();
			if ((SHM2_COUNT - size) > wr_len)
				break;	
		}
	}
	shm2->write_size(s_buf,wr_len);
	sem->Sem_V(1);
	return 0;
}


/********************************************ǰ�÷���������Ӧ����������ڴ�2���߳�*******************************************************************************************************/

int CReplyThread::run()
{
	char buf[SHM2_COUNT] = {0};
	int size = 0;
	CSendThread *send;
	while(1)
	{
		memset(buf,0,sizeof(buf));
		sem->Sem_P(1);
		size = shm2->get_head();
		if (size == 0)
		{
			while(1)
			{
				sem->Sem_V(1);
				usleep(3);
				sem->Sem_P(1);
				size = shm2->get_head();
				if (size > 0)
				break;			
			}
		}
		shm2->read_size(buf);
		sem->Sem_V(1);
		send = new CSendThread(size,buf);
		send->start();
	}
	return 0;
}


CSendThread::CSendThread(int size,char *buf)
{
	this->self = this;
	this->size = size;
	memcpy(this->buf,buf,SHM2_COUNT);
}

int CSendThread::run()
{
	PackHead_t head;
	int i = 0;
	int wr_size,wr_len;
	char s_buf[LISTSIZE] = {0}; 
	while(1)
	{
		memset(&head,0,sizeof(head));
		memcpy(&head,buf+i,sizeof(PackHead_t));
	    memset(s_buf,0,LISTSIZE);
		memcpy(s_buf,buf+i,head.pack_size+sizeof(PackHead_t));
		wr_len = head.pack_size + sizeof(PackHead_t);
		wr_size = send(head.des_fd,s_buf,LISTSIZE,0);
		if (wr_size > 0)
		{
			CLogThread::send_packs++;	
		}
		i = i + wr_len;
		if (i == size)
		{
			break;
		}
	}
	delete self;
	return 0;
}

/********************************************���÷���������ҵ����������ڴ�1������*******************************************************************************************************/
void shm1_read(CThreadPool *back_pool)
{
	int i;
	char buf[SHM1_SIZE-4] = {0};
	while(1)
	{
		memset(buf,0,sizeof(buf));
		sem->Sem_P(0);
		int counts = shm1->get_head();
		if (counts == 0)
		{
			while(1)
			{
				sem->Sem_V(0);
				usleep(3);
				sem->Sem_P(0);
				counts = shm1->get_head();
				if (counts > 0)
				break;			
			}
		}
		shm1->read_block(buf);
		sem->Sem_V(0);
		char buf1[BLOCKSIZE] = {0};
		for (i = 0;i < counts;i++)
		{
			memset(buf1,0,BLOCKSIZE);
			memcpy(buf1,buf+i*BLOCKSIZE,BLOCKSIZE);
			CHandleTask *task = new CHandleTask(buf1);
			back_pool->addTask(task);
		}
	}
}

/****************************************************���������߳�*******************************************************************************************************/
CPantThread::CPantThread(CEpoll *epoll)
{
	this->epoll = epoll;
}
int CPantThread::run()
{
	list<Pant_t>::iterator it = pant->begin();
	while(1)
	{
		pthread_mutex_lock(&pant_mutex);
		for (it = pant->begin();it != pant->end();)
		{
			if ((*it).flag == true)
			{
				(*it).flag = false;
				it++;
			}
			else if ((*it).flag == false)
			{
				if (CLogThread::links > 0)
				{
						pant->erase(it++);
						continue;
				}
				epoll->DelEvent((*it).fd);
				close((*it).fd);
				if (CLogThread::links > 0)
				{
					CLogThread::links--;
				}			
				pant->erase(it++);
			}
		}
		pthread_mutex_unlock(&pant_mutex);
		sleep(6);
	}
	return 0;
}

/*******************************************ʵʱ��־��ʾ�߳�*******************************************************************************************************/
int CLogThread::log_fd = 0;
int CLogThread::links = 0;
int CLogThread::recv_packs = 0;
int CLogThread::send_packs = 0;
int CLogThread::logins = 0;
int CLogThread::channels = 0;
int CLogThread::types = 0;
int CLogThread::areas = 0;
int CLogThread::lists = 0;
int CLogThread::plays = 0;
int CLogThread::play_times = 0;
int CLogThread::pants = 0;
int CLogThread::run()
{
	while (1)
	{
		sleep(1);
		printf("\\***********************************************************\\\n");
		printf("��Ч��������%d\n",links);
		printf("�������ݰ���%d  �������ݰ���%d  ��������%d\n\n",recv_packs,send_packs,pants);
		printf("�û���������%d\n",logins);
		printf("��ȡ��ƵƵ����%d  ��ȡ��Ƶ���ࣺ%d  ��ȡ��Ƶ������%d\n",channels,types,areas);
		printf("��ȡ��Ƶ�б�%d  �ϴ��������ݣ�%d  �ϴ�����ʱ����%d\n",lists,plays,play_times);
		printf("\\***********************************************************\\\n");
	}
}

/*******************************************��־���ɺ���*******************************************************************************************************/
void log_enab(int num)
{
	if (num == 1000 && sem->GetVal(2) == 1)
	{
		sem->SetVal(2,0);
	}
	else if (num != 1000 && sem->GetVal(2) == 0)
	{
		sem->SetVal(2,1);
	}
}

void log_write(int num,char *buf,int user_id)
{
	char log[2000] = {0};
	char str[100] = {0};
	time_t timep;
	struct tm *t;
	time(&timep);
	t = localtime(&timep);
	sprintf(str,"\r\nʱ�䣺%d-%d-%d %d:%d:%d\r\n",(1900+t->tm_year),(1+t->tm_mon),t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
	strcat(log,str);
	switch (num)
	{
	case 1001:
		strcat(log,"���ܣ���������\r\n");
		break;
	case 1002:
		strcat(log,"���ܣ���¼\r\n");
		break;
	case 2001:
		strcat(log,"���ܣ���ƵƵ����ȡ\r\n");
		break;
	case 2002:
		strcat(log,"���ܣ���Ƶ�����ȡ\r\n");
		break;
	case 2003:
		strcat(log,"���ܣ���Ƶ������ȡ\r\n");
		break;
	case 2004:
		strcat(log,"���ܣ���Ƶ�б��ȡ\r\n");
		break;
	case 3001:
		strcat(log,"���ܣ��ϴ���������\r\n");
		break;
	case 3002:
		strcat(log,"���ܣ��ϴ�����ʱ��\r\n");
		break;
	case 4001:
		strcat(log,"���ܣ�������ʷ��ȡ\r\n");
		break;
	}
	strcat(log,"���ͣ�����\r\n");
	if (num == 1001)
	{
		sprintf(str,"�ͻ���FD��%d\r\n",user_id);
	}
	else
		sprintf(str,"�û�ID��%d\r\n",user_id);
	strcat(log,str);
	strcat(log,"���ݰ���\r\n");
	char_hex(buf,log);
	flock(CLogThread::log_fd,LOCK_EX);
	write(CLogThread::log_fd,log,strlen(log));
	fsync(CLogThread::log_fd);//ˢ��fd���ݵ�����
	flock(CLogThread::log_fd,LOCK_UN);
}


void char_hex(char *buf,char *GetLog)
{
	int i,j;
	int len = 64;
	char acBuf[64] = {0};
	unsigned char tmp[512];
	memset(tmp,0,512);
	memcpy(tmp,buf,len);  
	memset(acBuf, 0x00 ,64);
	for(i=0;i<len/16+1;i++)
	{
		if((i*16) >= len)
		{
			break;
		}
		for(j=0;j<16;j++)
		{
			if((i*16+j) >= len)
			{
				break;
			}
			sprintf(acBuf,"%02x",tmp[i*16+j]);
			strcat(GetLog, acBuf);
			memset(acBuf, 0x00 ,64);

			if(((j+1)%4 != 0 ) && ((i*16+j)<(len-1)))
			{
				strcat(GetLog, " ");
			}
			if((j+1)%4 == 0 )
			{
				strcat(GetLog, " ");
			}
		}
		if(len/16 == i)
		{
			for(j=0;j<((16-(len%16)-1))*3;j++)
			{
				strcat(GetLog, " ");
			}
		}
		strcat(GetLog, "\r\n");
	}
}

/*******************************************��������ʼ������*******************************************************************************************************/
void init_server()
{
	char str[50] = {0};
	time_t timep;
	struct tm *t;
	time(&timep);
	t = localtime(&timep);
	sprintf(str,"Log/%d-%d-%d.log",(1900+t->tm_year),(1+t->tm_mon),t->tm_mday);
	CLogThread::log_fd = open(str,O_WRONLY | O_CREAT | O_APPEND);
	shm1 = new CShaMemory(SHMKEY1,SHM1_SIZE);
	shm2 = new CShaMemory(SHMKEY2,SHM2_SIZE);
	sem = new CSem(SEMKEY,3);
	sem->SetVal(0,1);
	sem->SetVal(1,1);
	sem->SetVal(2,0);
}

void init_db()
{
	int size;
	PackHead_t head;
	memset(&head,0,sizeof(head));
	char sql[256] = {0};
	list<VideoChannel_t> channel;
	strcpy(sql,"select * from Tbl_video_channel");
	(DbSingles::GetSingle())->GetData(sql,List_CallBack,(void *)&channel);
	size = channel.size();
	channel_buf = new char[sizeof(VideoChannel_t)*size+sizeof(PackHead_t)];
	memset(channel_buf,0,sizeof(VideoChannel_t)*size+sizeof(PackHead_t));
	list<VideoChannel_t>::iterator it1 = channel.begin();
	for (;it1 != channel.end();it1++)
	{
		memcpy(channel_buf+sizeof(head)+sizeof(VideoChannel_t)*head.counts,&(*it1),sizeof(VideoChannel_t));
		head.counts++;
	}
	head.pack_size = head.counts*sizeof(VideoChannel_t);
	head.func_num = 2001;
	memcpy(channel_buf,&head,sizeof(head));
	
	memset(&head,0,sizeof(head));
	list<VideoType_t> type;
	strcpy(sql,"select * from Tbl_video_type");
	(DbSingles::GetSingle())->GetData(sql,List_CallBack,(void *)&type);
	size = type.size();
	type_buf = new char[sizeof(VideoType_t)*size+sizeof(PackHead_t)];
	memset(type_buf,0,sizeof(VideoType_t)*size+sizeof(PackHead_t));
	list<VideoType_t>::iterator it2 = type.begin();
	for (;it2 != type.end();it2++)
	{
		memcpy(type_buf+sizeof(head)+sizeof(VideoType_t)*head.counts,&(*it2),sizeof(VideoType_t));
		head.counts++;
	}
	head.pack_size = head.counts*sizeof(VideoType_t);
	head.func_num = 2002;
	memcpy(type_buf,&head,sizeof(head));

	memset(&head,0,sizeof(head));
	list<VideoArea_t> area;
	strcpy(sql,"select * from Tbl_video_area");
	(DbSingles::GetSingle())->GetData(sql,List_CallBack,(void *)&area);
	size = area.size();
	area_buf = new char[sizeof(VideoArea_t)*size+sizeof(PackHead_t)];
	memset(area_buf,0,sizeof(VideoArea_t)*size+sizeof(PackHead_t));
	list<VideoArea_t>::iterator it3 = area.begin();
	for (;it3 != area.end();it3++)
	{
		memcpy(area_buf+sizeof(head)+sizeof(VideoArea_t)*head.counts,&(*it3),sizeof(VideoArea_t));
		head.counts++;
	}
	head.pack_size = head.counts*sizeof(VideoArea_t);
	head.func_num = 2003;
	memcpy(area_buf,&head,sizeof(head));	

	memset(&head,0,sizeof(head));
	msg = new list<VideoList_t>;
	strcpy(sql,"select * from Tbl_video_message");
	(DbSingles::GetSingle())->GetData(sql,List_CallBack,(void *)msg);
	size = msg->size();
	msg_buf = new char[sizeof(VideoList_t)*size+sizeof(PackHead_t)];
	memset(msg_buf,0,sizeof(VideoList_t)*size+sizeof(PackHead_t));
	list<VideoList_t>::iterator it4 = msg->begin();
	for (;it4 != msg->end();it4++)
	{
		memcpy(msg_buf+sizeof(head)+sizeof(VideoList_t)*head.counts,&(*it4),sizeof(VideoList_t));
		head.counts++;
	}
	head.pack_size = head.counts*sizeof(VideoList_t);
	head.func_num = 2004;
	memcpy(msg_buf,&head,sizeof(head));	

	login = new list<Login_t>;
	strcpy(sql,"select * from Tbl_user");
	(DbSingles::GetSingle())->GetData(sql,Login_CallBack,(void *)login);
	pant = new list<Pant_t>;
}

int put_num(int counts)
{
	struct termios old_ter;
	struct termios new_ter;
	tcgetattr(0,&old_ter);
	new_ter=old_ter;
	new_ter.c_lflag &= ~(ICANON|ECHO);
	new_ter.c_cc[VMIN] = 1;
	new_ter.c_cc[VTIME] = 0;
	tcsetattr(0,TCSANOW,&new_ter);

	char num[10] = {0};
	char t;
	int i = 0;
	int valu = 0;
	t = getchar();
	while(1)
	{
		if ( i< counts && t>='0' && t<='9')
		{
			if (i != 0 || t != '0')
			{
				num[i] = t;
				i++;
				printf("%c",t);
			}
		
		}
		else if ( t== 10 && i > 0)
		{
			break;
		}
		else if (i > 0 && t == 127)
		{
			printf("\b \b");
			i--;
		}
		t = getchar();
	}
	num[i] = '\0';
	valu = atoi(num);
	tcsetattr(0,TCSANOW,&old_ter);
	printf("\n");
	return valu;
}