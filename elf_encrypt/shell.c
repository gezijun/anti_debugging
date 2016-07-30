#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <elf.h>
#include <sys/mman.h>

#define PAGE_SHIFT   12
#define PAGE_SIZE (1UL << PAGE_SHIFT)

typedef struct {
	Elf64_Addr st_value;
	Elf64_Word st_size;
}func_info;

void say_hello() { /* 被加密函数 */
	puts("hello elf.");
}

void __init() __attribute__((constructor));

static unsigned long get_lib_addr(){
	unsigned long ret = 0;
	char buf[4096], *temp;
	int pid;
	FILE *fp;
	pid = getpid();
	sprintf(buf, "/proc/%d/maps", pid);
	fp = fopen(buf, "r");

	if(fp == NULL) {
		puts("open failed");
		goto _error;
	}

	while(fgets(buf, sizeof(buf), fp)){
		if(strstr(buf, "libdemo.so")){
			temp = strtok(buf, "-");
			ret = strtoul(temp, NULL, 16);
			break;
		}
	}
_error:
	fclose(fp);
	return ret;
}

void __init(){ /* 解密函数 */
	const char target_fun[] = "say_hello";
	func_info info;
	int i;
	unsigned long npage, base = get_lib_addr();
	Elf64_Ehdr *ehdr = (Elf64_Ehdr *)base;

	info.st_value = ehdr->e_shoff;
	info.st_size = ehdr->e_shnum;

	npage = info.st_size / PAGE_SIZE + ((info.st_size % PAGE_SIZE == 0) ? 0 : 1);
	if(mprotect((void *) ((base + info.st_value) / PAGE_SIZE * PAGE_SIZE), npage, PROT_READ | PROT_EXEC | PROT_WRITE) != 0){
		puts("mem privilege change failed");
	}

	for(i = 0; i < info.st_size; i++){
		char *addr = (char*)(base + info.st_value + i);
		*addr = ~(*addr);
	}

	if(mprotect((void *) ((base + info.st_value) / PAGE_SIZE * PAGE_SIZE), npage, PROT_READ | PROT_EXEC) != 0){
		puts("mem privilege change failed");
	}
}

