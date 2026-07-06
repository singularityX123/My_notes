#include <sqlite3.h>
#include <stdio.h>

// 回调函数
int callback(void *data, int argc, char **argv, char **azColName) {
    int i;

    printf("callback:\n");

    for(i = 0; i < argc; i++) {
       printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
 
    return 0;
}
 
int main () {
    sqlite3 *db;
    char *zErrMsg = 0; // 存储错误消息的指针
    int rc;
 
    rc = sqlite3_open("../../database/test.db", &db);
    if (rc != SQLITE_OK) {
       fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
       sqlite3_close(db);
       return 1;
    }
 
    const char* sql = "SELECT * from COMPANY";
    rc = sqlite3_exec(db, sql, callback, NULL, &zErrMsg);
 
    if (rc != SQLITE_OK) {
       fprintf(stderr, "SQL error: %s\n", zErrMsg);
       sqlite3_free(zErrMsg);
    } else {
       printf("Operation done successfully\n");
    }
 
    sqlite3_close(db);
    return rc;
}