#include "memory.h"
#include "bitmap.h"
#include "stdint.h"
#include "global.h"
#include "debug.h"
#include "print.h"
#include "string.h"

#define PG_SIZE 4096

#define MEM_BITMAP_BASE 0xc009a000

#define PDT_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

// 0c0000000是内核开始的地址，100000可以跨过低端1mb
#define K_HEAP_START 0xc0100000

struct pool {
	struct bitmap pool_bitmap;
	uint32_t phy_addr_start;
	uint32_t pool_size; // 字节容量
};

struct pool kernel_pool, user_pool;

struct virtual_addr kernel_vaddr;

static void mem_pool_init(uint32_t all_mem) {
	put_str("   mem_pool_init start \n");
	// 页表大小256×4K; 内核地址的页表空间
	uint32_t page_table_size = PG_SIZE * 256;
	uint32_t used_mem = page_table_size + 0x100000;
	uint32_t free_mem = all_mem - used_mem;
	uint16_t all_free_pages = free_mem / PG_SIZE;
	uint16_t kernel_free_pages = all_free_pages / 2;
	uint16_t user_free_pages = all_free_pages - kernel_free_pages;
	// 每个位表示1页，kbm位图等于页数/8 
	uint32_t kbm_length = kernel_free_pages / 8;
	uint32_t ubm_length = user_free_pages / 8;
	// 计算起始地址
	uint32_t kp_start = used_mem;
	uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;
	// 初始化pool
	kernel_pool.phy_addr_start = kp_start;
	user_pool.phy_addr_start = up_start;

	kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
	user_pool.pool_size = user_free_pages * PG_SIZE;

	kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
	user_pool.pool_bitmap.btmp_bytes_len = ubm_length;

	kernel_pool.pool_bitmap.bits = (void*) MEM_BITMAP_BASE;
	user_pool.pool_bitmap.bits = (void*) (MEM_BITMAP_BASE + kbm_length);

	put_str("kernel_pool_free_pages="); put_int((uint32_t) kernel_free_pages); 
	put_str("\n");
	put_str("kernel_pool_bitmap_start: "); put_int((int) kernel_pool.pool_bitmap.bits); 
	put_str(" kernel_pool_phy_addr_start: "); put_int(kernel_pool.phy_addr_start);
	put_str("\n");

	put_str("user_pool_free_pages="); put_int((uint32_t) user_free_pages);
	put_str("\n");
	put_str("user_pool_bitmap_start: "); put_int((int) user_pool.pool_bitmap.bits);
	put_str(" user_pool_phy_start: "); put_int((int) user_pool.phy_addr_start);
	put_str("\n");

	bitmap_init(&kernel_pool.pool_bitmap);
	bitmap_init(&user_pool.pool_bitmap);

	kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
	kernel_vaddr.vaddr_bitmap.bits = (void*) (MEM_BITMAP_BASE + kbm_length + ubm_length);
	kernel_vaddr.vaddr_start = K_HEAP_START;
	bitmap_init(&kernel_vaddr.vaddr_bitmap);

	put_str("kernel_vaddr_bitmap_start="); put_int((int)kernel_vaddr.vaddr_bitmap.bits); 
	put_str(" kernel_vaddr_start="); put_int(kernel_vaddr.vaddr_start);
	put_str("\n");

	put_str("mem_pool_init done\n");

}

/**
 * 在内核的虚拟内存池中，申请cnt个内核或者用户页
 */
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt) {
	int vaddr_start = 0, bit_idx_start = -1;
	uint32_t cnt = 0;
	if (pf == PF_KERNEL) {
		bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
		if (bit_idx_start == -1) {
			return NULL;
		} 
		while(cnt < pg_cnt) {
			bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
		}
		vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
	} else {
		// 用户内存池，将来实现用户进程再补充
	}
	return (void*) vaddr_start;
}

// 得到虚拟地址vaddr对应的pte指针
uint32_t* pte_ptr(uint32_t vaddr) {
	return (uint32_t*) (0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);

}

// 得到虚拟地址vaddr对应的pde指针
uint32_t* pde_ptr(uint32_t vaddr) {
	return (uint32_t*) ((0xfffff000) + PDT_IDX(vaddr) * 4);
}

// 在m_pool指向的物理内存池中分配1个物理页
// 成功则返回页框的物理地址，失败则返回NULL
static void* palloc(struct pool* m_pool) {
	int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
	if (bit_idx == -1) {
		return NULL;
	}
	bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
	uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
	return (void*) page_phyaddr;
}
/* 页表中添加虚拟地址_vaddr与物理地址_page_phyaddr的映射 */
static void page_table_add(void* _vaddr, void* _page_phyaddr) {
   	uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
	// 获取虚拟地址的页目录表地址
   	uint32_t* pde = pde_ptr(vaddr);
	// 获取虚拟地址的页表地址
   	uint32_t* pte = pte_ptr(vaddr);
	// 页目录是否已经存在
	if (*pde & 0x00000001) {
		//ASSERT(!(*pte & 0x00000001));
		*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
	} else {
		uint32_t pde_phyaddr = (uint32_t) palloc(&kernel_pool);
		*pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
		memset((void*) ((int) pte & 0xfffff000), 0, PG_SIZE);	
		*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
	}	
}

/* 分配pg—cnt个页空间，成功则返回起始虚拟地址，失败时返回NULL */
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt) {
	ASSERT(pg_cnt > 0 && pg_cnt < 3840);
	// 1. 通过vaddr_get在虚拟内存池中申请虚拟内存
	// 2. 通过palloc在物理内存池中申请物理页
	// 3. 通过page-table-add将以上得到的虚拟地址和物理地址在页表中完成映射
	void* vaddr_start = vaddr_get(pf, pg_cnt);
	if (vaddr_start == NULL) {
		return NULL;
	}
	uint32_t vaddr = (uint32_t) vaddr_start, cnt = pg_cnt;
	struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
	/*
	 * 虚拟地址是连续的，但是物理地址可以是不连续的，所以逐个做映射 
	 */
	while( cnt-- > 0) {
		void* page_phyaddr = palloc(mem_pool);
		if (page_phyaddr == NULL) {
			return NULL;
		}
		page_table_add((void*)vaddr, page_phyaddr);
		vaddr += PG_SIZE;
	}
	return vaddr_start;
}

void* get_kernel_pages(uint32_t pg_cnt) {
	void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
	if (vaddr != NULL) {
		memset(vaddr, 0, pg_cnt * PG_SIZE);
	}
	return vaddr;
}

void mem_init() {
	put_str("mem_init start\n");
	uint32_t mem_bytes_total = (*(uint32_t*)(0xb03));
	mem_pool_init(mem_bytes_total);
	put_str("mem_init done\n");
}
