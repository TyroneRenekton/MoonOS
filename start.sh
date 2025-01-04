#!/bin/sh
nasm -I boot/include -o boot/mbr.bin boot/mbr.S
nasm -I boot/include -o boot/loader.bin boot/loader.S
i686-linux-gnu-gcc -m32 -c -o kernel/main.o kernel/main.c
i686-linux-gnu-ld kernel/main.o -Ttext 0xc0001500 -e main -o kernel/kernel.bin 
dd if=boot/loader.bin of=hd60M.img bs=512 count=4 seek=2 conv=notrunc
dd if=boot/mbr.bin of=hd60M.img bs=512 count=1 conv=notrunc
dd if=kernel/kernel.bin of=hd60M.img bs=512 count=200 seek=9 conv=notrunc
./bochs -f bochsrc.disk
