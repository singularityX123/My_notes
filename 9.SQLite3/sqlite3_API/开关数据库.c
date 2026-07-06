#include <stdio.h>
#include <sqlite3.h>
 
int main() {
    sqlite3 *db;
    /*打开数据库*/
    int rc = sqlite3_open("../database/mydatabase.db", &db);
    const char *str = sqlite3_db_filename(db, "main");

    if (rc == SQLITE_OK) {
        printf("已成功打开数据库%s\n", str);  
    } else {
        fprintf(stderr, "无法打开数据库: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);       /*无法打开，马上关闭数据库*/
        return 1;
    }

    //TODO
    //数据库操作...


    /*关闭数据库*/
    rc = sqlite3_close(db);
    if (rc == SQLITE_OK) {
        printf("关闭数据库成功！\n");
    } else {
        fprintf(stderr, "无法关闭数据库: %s\n", sqlite3_errmsg(db));
    }
    
    return 0;
}