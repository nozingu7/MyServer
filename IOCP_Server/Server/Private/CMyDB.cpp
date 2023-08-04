#include "CMyDB.h"
#include <iostream>
using namespace std;

#pragma warning(disable:4996)

#define DB_HOST "127.0.0.1"
#define DB_USER "root"
#define DB_PASS "dhkd4120"
#define DB_NAME "chatdb"

CMyDB::CMyDB()
{
}

CMyDB::~CMyDB()
{
}

bool CMyDB::LoginDataCheck(const char* szID, const char* szPassword)
{
	bool bSuccess = false;
	char dbID[20];
	char dbPassword[20];
	char queryStatement[256];
	memset(dbID, 0, sizeof(dbID));
	memset(dbPassword, 0, sizeof(dbPassword));
	memset(queryStatement, 0, sizeof(queryStatement));

	connection = mysql_init(NULL);
	// 포트번호는 기본값 NULL = 3306
	connection = mysql_real_connect(connection, DB_HOST, DB_USER, DB_PASS, DB_NAME, NULL, (char*)NULL, 0);

	if (connection == nullptr)
	{
		return false;
		cout << "DB Connection Failed\n";
	}

	sprintf(queryStatement, "select id, password from idtable where id = \'%s\'", szID);
	query_reslut = mysql_query(connection, queryStatement);

	if (0 != query_reslut)
		return false;

	result = mysql_store_result(connection);
	ULONG iCount = result->field_count;

	if (0 != (row = mysql_fetch_row(result)))
	{
		strcpy(dbID, row[0]);
		strcpy(dbPassword, row[1]);

		if (!strcmp(dbID, szID) && !strcmp(dbPassword, szPassword))
		{
			cout << "로그인 승인\n";
			bSuccess = true;
		}
		else
		{
			cout << "로그인 불가\n";
			bSuccess = false;
		}
	}
	else
	{
		// 유저 정보 없음
		cout << "DB에 일치하는 유저 정보 없음!!\n";
		bSuccess = false;
	}

	// mysql_store_result 함수를 통해 사용한 메모리를 반환해줌 
	mysql_free_result(result);
	mysql_close(connection);

	return bSuccess;
}

char* CMyDB::GetNickName(const char* szID)
{
	bool bSuccess = false;
	char dbNickName[20];
	char queryStatement[256];
	memset(dbNickName, 0, sizeof(dbNickName));
	memset(queryStatement, 0, sizeof(queryStatement));

	connection = mysql_init(NULL);
	// 포트번호는 기본값 NULL = 3306
	connection = mysql_real_connect(connection, DB_HOST, DB_USER, DB_PASS, DB_NAME, NULL, (char*)NULL, 0);

	if (connection == nullptr)
	{
		return dbNickName;
		cout << "DB Connection Failed\n";
	}

	sprintf(queryStatement, "select name from infotable where infotable.idx = (select idx from idtable where id = \'%s\'); ", szID);
	query_reslut = mysql_query(connection, queryStatement);

	if (0 != query_reslut)
		return dbNickName;

	result = mysql_store_result(connection);
	ULONG iCount = result->field_count;

	if (0 != (row = mysql_fetch_row(result)))
		strcpy(dbNickName, row[0]);

	// mysql_store_result 함수를 통해 사용한 메모리를 반환해줌 
	mysql_free_result(result);
	mysql_close(connection);

	return dbNickName;
}

bool CMyDB::DBDubplicateCheck(const char* szStr, int iType)
{
	char dbStr[20];
	char queryStatement[256];
	memset(dbStr, 0, sizeof(dbStr));
	memset(queryStatement, 0, sizeof(queryStatement));

	connection = mysql_init(NULL);
	// 포트번호는 기본값 NULL = 3306
	connection = mysql_real_connect(connection, DB_HOST, DB_USER, DB_PASS, DB_NAME, NULL, (char*)NULL, 0);

	if (connection == nullptr)
	{
		return false;
		cout << "DB Connection Failed\n";
	}

	if(0 == iType)
		sprintf(queryStatement, "select id from idtable where id = \'%s\'", szStr);
	else
		sprintf(queryStatement, "select name from infotable where name = \'%s\'", szStr);
	query_reslut = mysql_query(connection, queryStatement);

	if (0 != query_reslut)
		return false;

	result = mysql_store_result(connection);

	if (nullptr == result)
		return false;

	if (0 != (row = mysql_fetch_row(result)))
	{
		if (!strcmp(szStr, row[0]))
			return false;
	}

	return true;
}

bool CMyDB::SignUp(const char* szID, const char* szPass, const char* szNick, const char* szIP, const int iPort)
{
	char queryStatement[256];
	memset(queryStatement, 0, sizeof(queryStatement));

	connection = mysql_init(NULL);
	// 포트번호는 기본값 NULL = 3306
	connection = mysql_real_connect(connection, DB_HOST, DB_USER, DB_PASS, DB_NAME, NULL, (char*)NULL, 0);

	if (connection == nullptr)
	{
		return false;
		cout << "DB Connection Failed\n";
	}

	sprintf(queryStatement, "insert into idtable values(NULL, \'%s\', \'%s\')", szID, szPass);
	query_reslut = mysql_query(connection, queryStatement);

	if (0 != query_reslut)
		return false;

	sprintf(queryStatement, "insert into infotable values(\'%s\', \'%s\', %d, NOW(), \
		(select idx from idtable where id = \'%s\'))", szNick, szIP, iPort, szID);
	query_reslut = mysql_query(connection, queryStatement);

	if (0 != query_reslut)
		return false;

	return true;
}
