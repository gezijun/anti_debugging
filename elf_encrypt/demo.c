#include <stdio.h>
#include <fcntl.h>
#include <elf.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
        Elf64_Addr st_value;
        Elf64_Word st_size;
}func_info;
Elf64_Ehdr ehdr;

int
find_target_section_addr(const int fd, const char *sec_name){
        lseek(fd, 0, SEEK_SET);
        if(read(fd, &ehdr, sizeof(Elf64_Ehdr)) != sizeof(Elf64_Ehdr)){
                puts("Read ELF header error");
                return (-1);
        }

        return (0);
}

static char get_target_func_info(int fd, const char *func_name, func_info *info){
        char flag = -1, *dynstr;
        int i;
        Elf64_Sym func_sym;
        Elf64_Phdr phdr;
        Elf64_Off dyn_off;
        Elf64_Word dyn_size, dyn_strsz;
        Elf64_Dyn dyn;
        Elf64_Addr dyn_symtab, dyn_strtab;

        lseek(fd, ehdr.e_phoff, SEEK_SET);
        for(i = 0; i < ehdr.e_phnum; i++){
                if(read(fd, &phdr, sizeof(Elf64_Phdr)) != sizeof(Elf64_Phdr)){
                        puts("Read segment failed");
                        return (-1);
                }

                if(phdr.p_type ==  PT_DYNAMIC){
                        dyn_size = phdr.p_filesz;
                        dyn_off = phdr.p_offset;
                        flag = 0;
                        printf("Find section %s, size = 0x%x, addr = 0x%lx\n", ".dynamic", dyn_size, dyn_off);
                        break;
                }
        }

        if(flag) {
                puts("Find .dynamic failed");
                return (-1);
        }

        flag = 0;
        lseek(fd, dyn_off, SEEK_SET);
        for(i=0;i < dyn_size / sizeof(Elf64_Dyn); i++){
                if(read(fd, &dyn, sizeof(Elf64_Dyn)) != sizeof(Elf64_Dyn)){
                        puts("Read .dynamic information failed");
                        return (-1);
                }

                if(dyn.d_tag == DT_SYMTAB){
                        dyn_symtab = dyn.d_un.d_ptr;
                        flag++;
                        printf("Find .dynsym, addr = 0x%lx\n", dyn_symtab);
                }

                if(dyn.d_tag == DT_STRTAB){
                        dyn_strtab = dyn.d_un.d_ptr;
                        flag++;
                        printf("Find .dynstr, addr = 0x%lx\n", dyn_strtab);
                }

                if(dyn.d_tag == DT_STRSZ){
                        dyn_strsz = dyn.d_un.d_val;
                        flag++;
                        printf("Find .dynstr size, size = 0x%x\n", dyn_strsz);
                }

        }
        if(flag != 3){
                puts("Find needed .section failed\n");
                return (-1);
        }

        dynstr = (char*) malloc(dyn_strsz);
        if(dynstr == NULL){
                puts("Malloc .dynstr space failed");
                return (-1);
        }

        lseek(fd, dyn_strtab, SEEK_SET);
        if(read(fd, dynstr, dyn_strsz) != dyn_strsz){
                puts("Read .dynstr failed");
                return (-1);
        }
        lseek(fd, dyn_symtab, SEEK_SET);
        while (1) {
                if(read(fd, &func_sym, sizeof(Elf64_Sym)) != sizeof(Elf64_Sym)){
                        puts("Read func_sym failed");
                        return (-1);
                }

                if(strcmp(dynstr + func_sym.st_name, func_name) == 0){
                        break;
                }
        }

        printf("Find: %s, offset = 0x%lx, size = 0x%lx\n", func_name, func_sym.st_value, func_sym.st_size);
        info->st_value = func_sym.st_value;
        info->st_size = func_sym.st_size;

        ehdr.e_shoff = info->st_value;
        ehdr.e_shnum = info->st_size;

        lseek(fd, 0, SEEK_SET);
        if(write(fd, &ehdr, sizeof(Elf64_Ehdr)) != sizeof(Elf64_Ehdr)){
                puts("Write elf header failed");
                return (-1);
        }

        free(dynstr);
        return 0;
}

int main(int argc, char **argv){
        char sec_name[] = ".text";
        char func_name[] = "say_hello"; /* 被加密函数名 */

        char *content = NULL;
        int fd, i;
        Elf64_Off secOff;
        func_info info;

        if(argc < 2){
                puts("Usage: shell libxxx.so .(section) function");
                return -1;
        }

        fd = open(argv[1], O_RDWR);
        if(fd < 0){
                printf("open %s failed\n", argv[1]);
                return (-1);
        }

        if (find_target_section_addr(fd, sec_name) == -1) {
                printf("Find section %s failed\n", sec_name);
                return (-1);
        }

        if (get_target_func_info(fd, func_name, &info) == -1) {
                printf("Find function %s failed\n", func_name);
                return (-1);
        }

        content = (char*) malloc(info.st_size);
        if(content == NULL){
                puts("Malloc space failed");
                return (-1);
        }

        lseek(fd, info.st_value, SEEK_SET);
        if(read(fd, content, info.st_size) != info.st_size){
                puts("Malloc space failed");
                return (-1);
        }

        for(i = 0; i < info.st_size; i++){
                content[i] = ~content[i];
        }

        lseek(fd, info.st_value, SEEK_SET);
        if(write(fd, content, info.st_size) != info.st_size){
                puts("Write modified content to .so failed");
                return (-1);
        }

        puts("Complete!");

        free(content);
        close(fd);
        return 0;
}

