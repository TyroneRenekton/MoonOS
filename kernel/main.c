#include "print.h"
#include "debug.h"
#include "init.h"
void main(void) {
	put_str("I am kernel\n");
	init_all();
	// asm volatile("sti");
	ASSERT(1==2);
	while(1);
}
