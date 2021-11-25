#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define LUNIX_DEVICE "/dev/lunix0-batt"


int main (){

    int fd;
    char read_buff[100];

    fd = open(LUNIX_DEVICE,O_RDONLY);

    if(fd == -1)
    {
        printf("OMG open is wrongggggggggggggggggg\n");
        exit(-1);
    }

    if(read(fd,read_buff, sizeof(read_buff))>0)
        printf("device: %s\n", read_buff);
    else
        printf("Nothing read or ur read is wrong boiii\n");
    

    return 0;
}