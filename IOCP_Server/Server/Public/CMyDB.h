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

public:
	MYSQL* connection = nullptr;
	MYSQL_RES* result = nullptr;
	MYSQL_ROW row = nullptr;
	int query_reslut = 0;
};

