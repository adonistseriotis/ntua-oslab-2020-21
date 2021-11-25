#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    char buf[6];
    int fd, pid;

    fd = open("/proc/sys/kernel/ns_last_pid", O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        perror("Can't open ns_last_pid");
        return 1;
    }

    if (flock(fd, LOCK_EX)) {
        close(fd);
        printf("Can't lock ns_last_pid\n");
        return 1;
    }

    pid = 32767;
    snprintf(buf, sizeof(buf), "%d", pid - 1);
    printf("%s\n", buf);
    if (write(fd, buf, strlen(buf)) == -1) {
        printf("Can't write to file\n");
        return 1;
    }

    int new_pid;
    new_pid = fork();
    if (new_pid == 0) {
        printf("Starting riddle...\n");
        char* argv[] = {NULL};
        char* envp[] = {NULL};
        if(execve("/home/adonist/Desktop/riddle-20201014/riddle", argv, envp) == -1)
        {
            perror("Could not run riddle\n");
        }
        exit(0);
    } else if (new_pid == pid) {
        return 0;
    } else {
        printf("pid does not match expected one\n");
    }

    if (flock(fd, LOCK_UN)) {
        printf("Can't unlock");
    }

    close(fd);

    return 0;
}