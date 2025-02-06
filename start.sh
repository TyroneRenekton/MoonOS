#!/bin/sh
rm -rf build/
mkdir build
nasm -I boot/include -o boot/mbr.bin boot/mbr.S
nasm -I boot/include -o boot/loader.bin boot/loader.S
nasm -f elf -o build/print.o lib/kernel/print.S
nasm -f elf -o build/kernel.o kernel/kernel.S
i686-linux-gnu-gcc -I lib/kernel -c -o build/timer.o device/timer.c
i686-linux-gnu-gcc -I lib/kernel -I lib/ -I kernel/ -I device/ -m32 -c -fno-builtin -o build/string.o kernel/string.c
i686-linux-gnu-gcc -I lib/kernel -I lib/ -I kernel/ -m32 -c -fno-builtin -o build/bitmap.o lib/kernel/bitmap.c
i686-linux-gnu-gcc -I lib/kernel -I lib/ -I kernel/ -I device/ -m32 -c -fno-builtin -o build/memory.o kernel/memory.c
i686-linux-gnu-gcc -I lib/kernel -I lib/ -I kernel/ -m32 -c -fno-builtin -o build/debug.o kernel/debug.c
i686-linux-gnu-gcc -I lib/kernel -I lib/ -I kernel/ -m32 -c -fno-builtin -o build/main.o kernel/main.c
i686-linux-gnu-gcc -I lib/kernel -I lib/ -I kernel/ -m32 -c -fno-builtin -o build/interrupt.o kernel/interrupt.c
i686-linux-gnu-gcc -I lib/kernel -I lib/ -I kernel/ -I device/ -m32 -c -fno-builtin -o build/init.o kernel/init.c
i686-linux-gnu-ld -Ttext 0xc0001500 -e main -o build/kernel.bin build/main.o build/print.o build/bitmap.o build/memory.o build/string.o build/interrupt.o build/timer.o build/init.o build/debug.o build/kernel.o 
dd if=boot/loader.bin of=hd60M.img bs=512 count=4 seek=2 conv=notrunc
dd if=boot/mbr.bin of=hd60M.img bs=512 count=1 conv=notrunc
dd if=build/kernel.bin of=hd60M.img bs=512 count=200 seek=9 conv=notrunc
./bochs -f bochsrc.disk
