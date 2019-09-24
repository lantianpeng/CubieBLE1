#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include "errno.h"
#include <misc/printk.h>
#include <zephyr.h>

#define MEM_POOL_BUF_NUM 10
#define MEM_POOL_BUF_SIZE 96
u8_t mem_pool[MEM_POOL_BUF_NUM][MEM_POOL_BUF_SIZE];
struct k_mem_slab wechat_mem;

void mem_init(void)
{
	printk("mem_init\n");

	k_mem_slab_init(&wechat_mem, mem_pool, MEM_POOL_BUF_SIZE, MEM_POOL_BUF_NUM);

	return ;
}

void *mem_malloc(size_t size)
{
	int ret;
	void *b;
	
	printk("mem_malloc, size:%d,", size);
	if (size == 0)
		return NULL;
	
	ret = k_mem_slab_alloc(&wechat_mem, &b, K_NO_WAIT);
	if (ret == 0) {
		printk("buf:%p\n", b);
		return b;
	} else {
		printk("ret:%d\n", ret);
		return NULL;
	}
}

void mem_free(void *buf)
{
	printk("mem_free,buf:%p\n", buf);
	
	if (buf == NULL)
		return ;
	k_mem_slab_free(&wechat_mem, &buf);
}
