#include "../head/UDP.h"
#include <ctype.h>
#include <sqlite3.h>

#define DATABASE_NAME "mydatabase.db"

int callback(void *, int, char **, char **);

/* 其他必要的环境已封装好了，只需要实现与客户端的交互即可
 * 这里的fd是服务端的socket，
 * addr是服务端的地址*/
void UDP_main(const int fd, const struct sockaddr_in *addr) {
	int ret = 0, rc;
	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof(client_addr);
	sqlite3 *db;
	char buf[BUFSIZ] = {};
	char *sql_query, *errmsg;

	/* 1.打开数据库 */
	if( (rc = sqlite3_open(DATABASE_NAME, &db) ) ) {
		printf("[%s:%d]无法打开数据库: %s\n", __FUNCTION__, __LINE__, sqlite3_errmsg(db) );
		exit(0);
	}
	/* 2.循环处理客户端数据 */
	while(1) {
		/* 2.1.接收客户端数据 */
		do {
			ret = recvfrom(fd, buf, BUFSIZ, 0, (struct sockaddr *)&client_addr, &addrlen);
		}while(ret < 0 && errno == EINTR);
		if(ret < 0)
			ErrExit("recvfrom");

		printf("[%s:%d]收到的数据: {%s}\n", __FUNCTION__, __LINE__, buf);
		/* 2.2.提取需要翻译的单词 */
		for(ret = 0; isalpha(buf[ret]) || buf[ret] == ' '; ret++);
		buf[ret] = '\0';
		/* 2.3.用SQL语句进行查询 */
		sql_query = sqlite3_mprintf("select * from dictionary where english like '%s'", buf);
		rc = sqlite3_exec(db, sql_query, callback, buf, &errmsg);
		if(rc != SQLITE_OK) {
			sprintf(buf, "查询失败:%s\n", errmsg);
			printf("[%s:%d]%s", __FUNCTION__, __LINE__, buf);
			sendto(fd, buf, strlen(buf) + 1, 0, (struct sockaddr *)&client_addr, addrlen);
			sqlite3_free(errmsg);
			continue;
		}
		sqlite3_free(sql_query);
		printf("[%s:%d]查询结果: {%s}\n", __FUNCTION__, __LINE__, buf);
		sendto(fd, buf, strlen(buf) + 1, 0, (struct sockaddr *)&client_addr, addrlen);
	}
	sqlite3_close(db);
	/* 3.关闭数据库，关闭fd，并且退出程序 */
	close(fd);
}

int callback(void *NotUsed, int argc, char **argv, char **ColName) {
	char *buf = NotUsed;
	if(argc == 2) {
		/* 把查询到的字符串复制给buf */
		strncpy(buf, argv[1], strlen(argv[1]) + 1 );
	} else
		buf[0] = '\0'; //如果失败就将字符串置空
	/* 将换行符替换为'\0' */
	buf[strlen(argv[1])] = '\n';
	buf[strlen(argv[1])+1] = '\0';
	return 0;
}