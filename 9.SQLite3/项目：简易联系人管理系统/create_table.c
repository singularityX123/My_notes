#include <sqlite3.h>
#include <stdio.h>

#define DATABASE_NAME "contacts.db"

int main(int argc, char* argv[]) {
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	/*打开数据库*/
	rc = sqlite3_open(DATABASE_NAME, &db);
	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return 1;
	} else {
		fprintf(stdout, "Opened database successfully\n");
	}

    //TODO 改为标准IO传入
	/*创建表的SQL语句*/
	char *sql = "CREATE TABLE IF NOT EXISTS contact ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"name TEXT NOT NULL,"
		"phone TEXT,"
		"email TEXT"
		");";

	/*执行SQL语句*/
	rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	} else {
		printf("Table created successfully\n");
	}

	/*关闭数据库文件*/
	sqlite3_close(db);
    printf("Database closed successfully\n");

	return 0;
}