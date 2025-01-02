#!/bin/sh
nasm -I boot.inc -o mbr.bin mbr.S
nasm -I boot.inc -o loader.bin loader.S
i686-linux-gnu-gcc -m32 -c -o main.o main.c
i686-linux-gnu-ld main.o -Ttext 0xc0001500 -e main -o kernel.bin 
dd if=loader.bin of=hd60M.img bs=512 count=4 seek=2 conv=notrunc
dd if=mbr.bin of=hd60M.img bs=512 count=1 conv=notrunc
dd if=kernel.bin of=hd60M.img bs=512 count=200 seek=9 conv=notrunc
./bochs -f bochsrc.disk
