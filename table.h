#ifndef _PT_TABLE_INCLUED_H_
#define _PT_TABLE_INCLUED_H_

typedef int qboolean;

#define TABLE_NORMAL_COUNT 65535

struct pt_table_node
{
	struct pt_table_node *next;
	uint64_t id;
	void* ptr;
};



struct pt_table
{
    //表的头，一个数组，数组的大小为granularity
	struct pt_table_node **head;
    
    //粒度
	uint32_t granularity;
    
    //表内的数据数量
	uint32_t size;
    
    //这个表是否是new出来的，如果是则free掉
	qboolean alloc;
};

typedef void (*pt_table_enum_cb)(struct pt_table* ptable, uint64_t id,void *ptr, void* user_arg);

/*
    创建一个快速搜索表
 */
struct pt_table *pt_table_new();
/*
    删除一个快速搜索表
 */
void pt_table_free(struct pt_table *ptable);
/*
    清空快速搜索表的内容
 */
void pt_table_clear(struct pt_table *ptable);
/*
    获取表内的数据数量
 */
uint32_t pt_table_size(struct pt_table *ptable);
/*
    添加一个数据到表内
 */
void pt_table_insert(struct pt_table *ptable, uint64_t id, void* ptr);

/*
    从表中移除一个数据
*/
void pt_table_erase(struct pt_table *ptable, uint64_t id);

/*
    从表中查找一个数据
 */
void* pt_table_find(struct pt_table *ptable, uint64_t id);

/*
    枚举整个table
 */
void pt_table_enum(struct pt_table *ptable, pt_table_enum_cb cb, void* user_arg);

#endif
