#include "print.h"
#include "debug.h"
#include "init.h"
#include "memory.h"
void main(void) {
	put_str("I am kernel\n");
	init_all();
	void* addr = get_kernel_pages(3);
	put_str("\n get_kernel_page start vaddr is ");
	put_int((uint32_t) addr);
	put_str("\n");
	// asm volatile("sti");
	//ASSERT(1==2);
	while(1);
}
