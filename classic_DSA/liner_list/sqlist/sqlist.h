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
typedef int data_t;
#define N 128

typedef struct {
    data_t data[N];
    int last;
}sqlist, *sqlink; 


sqlink list_create();

int list_clear(sqlink L);

int list_empty(sqlink L);

int list_length(sqlink L);

int list_locate(sqlink L, data_t val);

int list_insert(sqlink L, data_t val, int pos);



