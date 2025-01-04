完成我一直没完成的事情，自制操作系统，记录遇到的问题

arm架构直接编译不行，需要使用下面的命令生成x86-32位文件
i686-linux-gnu-gcc -m32 -c -o main.o main.c
i686-linux-gnu-ld main.o -Ttext 0xc0001500 -e main -o kernel.bin 

1. 字符串搬运指令，movsb、movsw、movsd，其中movs代表move string，b代表byte、w代表word、d代表double word; 这个指令会将DS:[ESI], 搬运到ES:[EDI],
rep 指令用于重复执行指令，使用前需要将ecx赋值
每次执行后自动更新esi与edi，需要使用cld命令，clean direction，该指令会将eflags寄存器中的放行标志位DF置0,这样rep在movs以后，会自动加上搬运所需要的字节数; 而std表示 set direction会将df置为1, 会减掉字节
2. 系统调用传参方式
eax 存放子功能号
ebx、ecx、edx、esi、edi 存放1-5个参数（当参数数量小于等于5时使用）
使用ebx存放内存地址，将参数放在ebx开始的内存中（参赛数量大于5时使用）
猜想
1. 阅读elf文件
根据程序头header，设置选择子以及段基址和内存区域
还没有内存管理，所以先写死代码
