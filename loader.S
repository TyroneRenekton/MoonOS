%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR
jmp loader_start
GDT_BASE: dd 0x00000000 
	dd 0x00000000
CODE_DESC: dd 0x0000FFFF 
	dd DESC_CODE_HIGH4
DATA_STACK_DESC: dd 0x0000FFFF 
	dd DESC_DATA_HIGH4
VIDEO_DESC: dd 0x80000007 
	dd DESC_VIDEO_HIGH4
GDT_SIZE  equ $ - GDT_BASE
GDT_LIMIT equ GDT_SIZE-1
times 60 dq 0 
total_mem_bytes  dd 0
SELECTOR_CODE equ (0x0001 << 3) + TI_GDT + RPL0
SELECTOR_DATA equ (0x0002 << 3) + TI_GDT + RPL0
SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0
gdt_ptr dw GDT_LIMIT
	 dd GDT_BASE	  
ards_buf times 244 db 0 
ards_nr  dw 0
loadermsg db '2 loader in real.' 
loader_start:
; find max memory area 
	xor ebx, ebx 
	mov edx, 0x534d4150
	mov di, ards_buf
.e820_mem_get_loop:
	mov eax, 0x0000e820
	mov ecx, 20
	int 0x15
	add di, cx
	inc word [ards_nr]
	cmp ebx, 0
	jnz .e820_mem_get_loop

	mov cx, [ards_nr]
	mov ebx, ards_buf
	xor edx, edx
.find_max_mem_area:
	mov eax, [ebx]
	mov eax, [ebx+8]
	add ebx, 20
	cmp edx, eax
	jge .next_ards
	mov edx, eax
.next_ards:
	loop .find_max_mem_area
	jmp .mem_get_ok

.mem_get_ok:
	mov eax, total_mem_bytes
	mov [eax], edx
	
	mov sp, LOADER_BASE_ADDR
	mov bp, loadermsg
	mov cx, 17
	mov ax, 0x1301
	mov bx, 0x001f
	mov dx, 0x1800
	int 0x10
	in al, 0x92
	or al, 0000_0010B
	out 0x92, al
	lgdt [gdt_ptr]
	mov eax, cr0
	or eax, 0x00000001
	mov cr0, eax
	jmp dword SELECTOR_CODE:p_mode_start
[bits 32]
p_mode_start:
	mov ax, SELECTOR_DATA
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov esp, LOADER_STACK_TOP
	mov ax, SELECTOR_VIDEO
	mov gs, ax
	mov byte [gs:160], 'P'

	mov eax, KERNEL_START_SECTOR
	mov ebx, KERNEL_BIN_BASE_ADDR 
	mov ecx, 200

	call rd_disk_m_32

	call setup_page

	sgdt [gdt_ptr]
	mov ebx, [gdt_ptr+2]
	or dword[ebx+0x18+4], 0xc0000000
	add dword [gdt_ptr+2], 0xc0000000
	add esp, 0xc0000000
	mov eax, PAGE_DIR_TABLE_ADDR
	mov cr3, eax
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax
	lgdt [gdt_ptr]

	mov byte [gs:160], 'V'

	jmp $
setup_page:
	mov ecx, 4096
	mov esi, 0
.clear_page_dir:
	mov byte [PAGE_DIR_TABLE_ADDR+esi], 0
	inc esi
	loop .clear_page_dir
create_page_dir_table:
	mov eax, PAGE_DIR_TABLE_ADDR
	add eax, 0x01000
	or eax, PAGE_SYS_LOW12	
	or eax, PAGE_RW_WRITE
	mov [PAGE_DIR_TABLE_ADDR], eax
	mov [PAGE_DIR_TABLE_ADDR+3072], eax 	
	mov ebx, eax
	sub ebx, 0x1000
	mov [PAGE_DIR_TABLE_ADDR+4092], ebx
create_page_table:
	mov ecx, 1024
	mov esi, 0x1000
	mov ebx, 0x100000
	sub eax, 0x101000
.set_page_table_item:
	mov [ebx+esi], eax
	add esi, 0x4
	add eax, 0x1000
	loop .set_page_table_item
	ret
rd_disk_m_32:
	mov esi, eax
	mov di, cx
	; 设置要读取的扇区数
	mov dx, 0x1f2
	mov al, cl
	out dx, al
	mov eax, esi

	mov dx, 0x1f3
	out dx, al

	mov cl, 8
	shr eax, cl
	mov dx, 0x1f4
	out dx, al

	shr eax, cl
	mov dx, 0x1f5
	out dx, al

	shr eax, cl
	and al, 0x0f
	or al, 0xe0
	mov dx, 0x1f6
	out dx, al

	mov dx, 0x1f7
	mov al, 0x20
	out dx, al

.not_ready:
	nop
	in al, dx
	and al, 0x88
	cmp al, 0x08
	jnz .not_ready

	mov ax, di
	mov dx, 256
	mul dx 

	mov cx, ax
	mov dx, 0x1f0
.go_on_read:
	in ax, dx
	mov [ebx], ax
	add ebx, 2
	loop .go_on_read
	ret 
