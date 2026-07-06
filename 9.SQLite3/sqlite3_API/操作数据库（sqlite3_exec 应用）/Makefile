CC = gcc
CFLAGS = -Wall
LIBS = -lsqlite3

TARGETS = create_table insert_data query_data exec_advanced

all: $(TARGETS)

create_table: 1.创建表.c
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

insert_data: 2.向表中插入数据.c
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

query_data: 3.查询数据.c
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

exec_advanced: 4.sqlite3_exec函数高级用法.c
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

clean:
	rm -f $(TARGETS)

.PHONY: all clean