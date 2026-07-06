/*   第四参数 void *data  <-- 作为第一个参数传递给回调函数的指针   */
#include <sqlite3.h>
#include <stdio.h>

// 回调函数
int callback(void *data, int argc, char **argv, char **azColName) {
    int i;
    if(argv[0][0] == '1')
        printf("%s\n", (char *)data);
    for(i = 0; i < argc; i++) {
        printf("%s\t", argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
 
    return 0;
}
 
int main () {
    sqlite3 *db;  // 数据库指针
    char *zErrMsg = 0; // 存储错误消息的指针
    int rc;
 
    rc = sqlite3_open("../../database/test.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
 
    // 自定义 执行SQLite数据库查询操作
    char *str = "ID\t名字\t年龄"; // 传递自定义数据
    char *sql = "SELECT * from COMPANY"; // SQL查询语句
    rc = sqlite3_exec(db, sql, callback, str, &zErrMsg);
 
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        printf("Operation done successfully\n");
    }
 
    sqlite3_close(db);
    return rc;
}