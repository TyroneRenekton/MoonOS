#!/bin/sh
nasm -I boot.inc -o mbr.bin mbr.S
nasm -I boot.inc -o loader.bin loader.S
dd if=loader.bin of=hd60M.img bs=512 count=4 seek=2 conv=notrunc
dd if=mbr.bin of=hd60M.img bs=512 count=1 conv=notrunc
./bochs -f bochsrc.disk
