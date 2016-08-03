#include <stdlib.h>
#include <stdio.h>
#include <elf.h>

int main ()
{
	checkBreakPoint ();

	return 0;
}

unsigned long getLibAddr (const char *lib) 
{
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

	base = getLibAddr ("libcmxsecd.so");
	if (base == 0) {
		LOGI ("getLibAddr failed");
		return;
	}

	elfhdr = (Elf32_Ehdr *) base;
	pheader = base + elfhdr->e_phoff;

	for (i = 0; i < elfhdr->e_phnum; i++) {
		ph_t = (Elf32_Phdr*)(pheader + i * sizeof(Elf32_Phdr)); // ±éÀúËùÓÐprogram header

		if ( !(ph_t->p_flags & 1) ) continue;
		offset = base + ph_t->p_vaddr;
		offset += sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr) * elfhdr->e_phnum;
		
		char *p = (char*)offset;
		for (j = 0; j < ph_t->p_memsz; j++) {
			if(*p == 0x10 && *(p+1) == 0xde) {
				LOGI ("Find thumb bpt %X \n", p);
			} else if (*p == 0xf0 && *(p+1) == 0xf7 && *(p+2) == 0x00 && *(p+3) == 0xa0) {
				LOGI ("Find thumb2 bpt %X \n", p);
			} else if (*p == 0x01 && *(p+1) == 0x00 && *(p+2) == 0x9f && *(p+3) == 0xef) {
				LOGI ("Find arm bpt %X \n", p);
			}
			p++;
		}
	}
}
