#pragma once
#include <mysql.h>

class CMyDB
{
public:
	CMyDB();
	~CMyDB();

public:
	bool LoginDataCheck(const char* szID, const char* szPassword);
	char* GetNickName(const char* szID);
	bool DBDubplicateCheck(const char* szStr, int iType);
	bool SignUp(const char* szID, const char* szPass, const char* szNick, const char* szIP, const int iPort);

public:
	MYSQL* connection = nullptr;
	MYSQL_RES* result = nullptr;
	MYSQL_ROW row = nullptr;
	int query_reslut = 0;
};

