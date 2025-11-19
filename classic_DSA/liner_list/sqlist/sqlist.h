/*typedef int data_t;
#define N 128

struct sqlist_t
{
    data_t data[N];
    int last;
};

typedef struct sqlist_t sqlist; // 可使用sqlist L 进行初始化
typedef struct sqlist_t *sqlink; // 可使用sqlink p 进行初始化

下为简化版
*/
// 顺序表的结构体定义
typedef int data_t; // 将数据类型抽象化，便于后续修改维护
#define N 128

/**
* @data_t:数据类型 
* @N:顺序表最大长度
* @last:顺序表最后一个元素的下标 [-1, N-1] ，-1表示空表 N-1表示表满
*/
typedef struct {
    data_t data[N];
    int last;
}sqlist, *sqlink; 

/*****************************/
sqlink list_create();

/**
* @ret 0:成功  -1:失败
*/
int list_clear(sqlink L);

/**
* @ret 1:空表  0:非空表 -1:失败
*/
int list_empty(sqlink L);

int list_length(sqlink L);

/**
* @ret 目标元素第一次出现下标  -1:失败
*/
int list_locate(sqlink L, data_t val);

/** 
* @par positions: 用于获取目标元素所有位置的数组（调用者分配）
* @par max_positions: 数组最大容量
* @ret 目标元素出现的所有次数  -1:失败
*/
int list_locate_all(sqlink L, data_t val, int positions[], int max_positions);

/** 
* @par pos：[0,last+1] 可插入位置
* @par val:要插入的值
*/
int list_insert(sqlink L, data_t val, int pos);

/** 
* @par pos：[0,last] 索引下表
*/
int list_delete(sqlink L, int pos);

int list_purge(sqlink L);

int list_reverse(sqlink L);

//高级操作
int list_merge(sqlink L1, sqlink L2);


int list_clean_repeat(sqlink L);




