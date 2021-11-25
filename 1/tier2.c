#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include<dlfcn.h>

void setup_tier2()
{/*
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    pid_t pid = getpid();
    long int pidt = (long int) pid;
    int fd = open("/home/adonist/Desktop/riddle-20201014/curiosity", O_RDWR, mode);
    write(fd,&pidt,8);
    mmap((void *) 0x6042000,32768, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
*/
    typedef void* (*arbitrary)(void (*)());
    arbitrary unlock;
    void * handle = dlopen("/home/adonist/Desktop/riddle-20201014/tier3.so",RTLD_LAZY);

    if(!handle) {
        fprintf(stderr,"%s\n", dlerror());
        exit(EXIT_FAILURE);
    }
    printf("%p\n",handle);

    *(void**)(&unlock) = dlsym(handle,"unlock_tier3");

    if(!unlock) {
        fprintf(stderr,"%s\n", dlerror());
        exit(EXIT_FAILURE);
    }
    printf("%p\n", unlock);

    (void) unlock();

}