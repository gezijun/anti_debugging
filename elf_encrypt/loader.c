#include <stdio.h>
#include <dlfcn.h>

int main(int argc, char **argv)
{
        void (*say_hello)();
        void *h;
        char *error;

        h= dlopen("./libdemo.so", RTLD_NOW);
        if (h == NULL) {
                error = dlerror();
                printf("%s\n", error);
                return (-1);
        }

        say_hello = dlsym(h, "say_hello");

        say_hello();
        
        dlclose(h);
        return (0);
}
