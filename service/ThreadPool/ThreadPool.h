#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <list>
#include <iostream>
#include <pthread.h>
using namespace std;

//������࣬���������Ǽ̳���
class CTask
{
public:
	CTask(){}
	~CTask(){}
	virtual int run()= 0;
};

//�̳߳���
class CThreadPool
{
public:  
	CThreadPool(int maxCount = 100, int waittime = 30);
	int start(int threadcount = 30);			//Ĭ�ϴ���threadcount���߳����ȴ�����
	int addTask(CTask *task);					//��������ӵ��̳߳���  
	int destroy();									//ֹͣ�̳߳�
protected:
	static void* rontine(void * threadData);	//���̵߳��̺߳���
private:
	void createThread();
	void lock();							//����������
	void unlock();						//����������
	int  wait();							//���������������õȴ�
	int  waittime(int sec);				//�������������ȴ�ʱ��
	int  signal();						//�����ź�
	int  broatcast();					//�㲥�����߳��ź�
	list<CTask*> m_taskList;				//�����б�
	pthread_mutex_t m_mutex;				//�߳�ͬ����  
	pthread_cond_t m_cond;				//�߳�ͬ������������
	bool m_bStop;						//�߳�ֹͣ��ʶ
	int m_maxCount;						//�̵߳��������
	int m_waitCount;						//�̵߳ȴ���
	int m_count;								//��ǰ�̳߳��е��߳�����
	int m_waittime;							//�߳�������ĵȴ�ʱ��

}; 

#endif