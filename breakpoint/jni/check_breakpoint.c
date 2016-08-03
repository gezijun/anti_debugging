#include <stdlib.h>
#include <stdio.h>
#include <elf.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

void checkBreakPoint ();
unsigned long getLibAddr (const char *lib);

#define LOG_TAG "ANTIDBG_DEMO"

#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

int main ()
{
	dlopen ("./libdemo.so", RTLD_NOW);
puts ("After dlopen");
	sleep (60);
	checkBreakPoint ();

	return 0;
}

unsigned long getLibAddr (const char *lib) 
{
	puts ("Enter getLibAddr");
	unsigned long addr = 0;
	char lineBuf[256];
	
	snprintf (lineBuf, 256-1, "/proc/%d/maps", getpid ());
	FILE *fp = fopen (lineBuf, "r");
	if (fp == NULL) {
		perror ("fopen failed");
		goto bail;
	}
	while (fgets (lineBuf, sizeof(lineBuf), fp)) {
		if (strstr (lineBuf, lib)) {
			char *temp = strtok (lineBuf, "-");
			addr = strtoul (temp, NULL, 16);
			break;
		}
	}

bail: 
	fclose(fp);
	return addr;
}



void checkBreakPoint ()
{
	int i, j;
	unsigned int base, offset, pheader;
	Elf32_Ehdr *elfhdr;
	Elf32_Phdr *ph_t;

	base = getLibAddr ("libdemo.so");
	if (base == 0) {
		LOGI ("getLibAddr failed");
		return;
	}

	elfhdr = (Elf32_Ehdr *) base;
	pheader = base + elfhdr->e_phoff;

printf ("pheader count : %d\n", elfhdr->e_phnum);

	for (i = 0; i < elfhdr->e_phnum; i++) {
puts ("hello");
		ph_t = (Elf32_Phdr*)(pheader + i * sizeof(Elf32_Phdr)); // traverse program header

		if ( !(ph_t->p_flags & 1) ) continue;
		offset = base + ph_t->p_vaddr;
		offset += sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr) * elfhdr->e_phnum;
		
		char *p = (char*)offset;
		for (j = 0; j < ph_t->p_memsz; j++) {
	printf ("check one : total size %d, location %p\n", ph_t->p_memsz, p);
			if(*p == 0x01 && *(p+1) == 0xde) {
				printf ("Find thumb bpt %p\n", p);
			} else if (*p == 0xf0 && *(p+1) == 0xf7 && *(p+2) == 0x00 && *(p+3) == 0xa0) {
				printf ("Find thumb2 bpt %p\n", p);
			} else if (*p == 0x01 && *(p+1) == 0x00 && *(p+2) == 0x9f && *(p+3) == 0xef) {
				printf ("Find arm bpt %p\n", p);
			}
			p++;
		}
		puts ("endof for loop2");
	}
	puts ("endof for loop1");
}
