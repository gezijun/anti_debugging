/*
   detect breakpoint 
 */

#include <stdlib.h>
#include <stdio.h>
#include <elf.h>

unsigned long getLibAddr () 
{
	unsigned long ret = 0;
	char name[] = "libanti_debug.so";
	char buf[4096], *temp;
	int pid;
	FILE *fp;
	pid = getpid();
	sprintf(buf, "/proc/%d/maps", pid);
	fp = fopen(buf, "r");
	if (fp == NULL) {
		puts("open failed");
		goto _error;
	}
	while (fgets(buf, sizeof(buf), fp)) {
		if (strstr(buf, name)) {
			temp = strtok(buf, "-");
			ret = strtoul(temp, NULL, 16);
			break;
		}
	}

_error: fclose(fp);
	return ret;
}



void anti5()
{

	Elf32_Ehdr *elfhdr;
	Elf32_Phdr *pht;
	unsigned int size, base, offset,phtable;
	int n, i,j;
	char *p;

	//从maps中读取elf文件在内存中的起始地址
	base = getLibAddr();
	if(base == 0){
		LOGI("find base error\n");
		return;
	}

	elfhdr = (Elf32_Ehdr *) base;

	phtable = elfhdr->e_phoff + base;

	for(i=0;i<elfhdr->e_phnum;i++){

		pht = (Elf32_Phdr*)(phtable+i*sizeof(Elf32_Phdr));

		if(pht->p_flags&1){
			offset = pht->p_vaddr + base + sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr)*elfhdr->e_phnum;
			LOGI("offset:%X ,len:%X",offset,pht->p_memsz);

			p = (char*)offset;
			size = pht->p_memsz;

			for(j=0,n=0;j<size;++j,++p){

				if(*p == 0x10 && *(p+1) == 0xde){
					n++;
					LOGI("### find thumb bpt %X \n",p);
				}else if(*p == 0xf0 && *(p+1) == 0xf7 && *(p+2) == 0x00 && *(p+3) == 0xa0){
					n++;
					LOGI("### find thumb2 bpt %X \n",p);
				}else if(*p == 0x01 && *(p+1) == 0x00 && *(p+2) == 0x9f && *(p+3) == 0xef){
					n++;
					LOGI("### find arm bpt %X \n",p);
				}

			}
			LOGI("### find breakpoint num: %d\n",n);

		}
	}

}
