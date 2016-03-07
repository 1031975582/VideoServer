#ifndef _SINGLES_H_
#define _SINGLES_H_

#include <stdio.h>
#include <sqlite3.h>
#include <iconv.h>
#include "../public.h"
class DbSingles
{
public:
	static DbSingles *GetSingle();// ��ȡ������ָ��
    int GetData(char *sql,sqlite3_callback pFun,void *pData);// ִ��sql���	
private:
	DbSingles();
	~DbSingles();
	static DbSingles *pS;
    sqlite3 *db;
	friend void ReleaseDb();   
	
};

void ReleaseDb();// �ͷŵ������ر����ݿ�

int List_CallBack(void *pData,int cols,char **colvalu,char **colname);

int Login_CallBack(void *pData,int cols,char **colvalu,char **colname);

int His_CallBack(void *pData,int cols,char **colvalu,char **colname);

#endif