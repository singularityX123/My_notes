#include <sqlite3.h>
#include <stdio.h>
 
int main(int argc, char* argv[]) {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
 
    /*打开数据库*/
    rc = sqlite3_open("../../database/test.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    } else {
        fprintf(stdout, "Opened database successfully\n");
    }
 
    /*插入SQL语句*/
    char *sql = "INSERT INTO COMPANY (ID,NAME,AGE) VALUES (1, '张三', 32);"
        "INSERT INTO COMPANY (ID,NAME,AGE) VALUES (2, '李四', 33);"
        "INSERT INTO COMPANY (ID,NAME,AGE) VALUES (3, '王五', 30);"
        "INSERT INTO COMPANY (ID,NAME,AGE) VALUES (4, '王博', 32);"
        "INSERT INTO COMPANY (ID,NAME,AGE) VALUES (5, '李为', 33);"
        "INSERT INTO COMPANY (ID,NAME,AGE) VALUES (6, '赵倩', 30);";
 
    /*执行SQL语句*/
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Records created successfully\n");
    }
 
    /*关闭数据库文件*/
    sqlite3_close(db);
    return 0;
}