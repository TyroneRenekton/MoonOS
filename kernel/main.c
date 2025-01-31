#include "print.h"
#include "init.h"
void main(void) {
	put_str("I am kernel\n");
	init_all();
	asm volatile("sti");
	put_str("open the interrupt");
	while(1);
}
