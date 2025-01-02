完成我一直没完成的事情，自制操作系统，记录遇到的问题

arm架构直接编译不行，需要使用下面的命令生成x86-32位文件
i686-linux-gnu-gcc -m32 -c -o main.o main.c
i686-linux-gnu-ld main.o -Ttext 0xc0001500 -e main -o kernel.bin 

