#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
int main(){
    
    pid_t pid;
    // OPEN FILES
    int fd;
    fd = open("test.txt" , O_RDWR | O_CREAT | O_TRUNC);
    if (fd == -1)
    {
        /* code */
        printf("error in opening file");
        return -1;
    }
    //write 'hello fcntl!' to file
    char str[] = "hello fcntl!";
    write(fd, str, sizeof(str)-1);

    // DUPLICATE FD
    // using fcntl to duplicate fd
    int fd2 = fcntl(fd, F_DUPFD, 0);
    if(fd2 == -1){
        printf("error in fcntl");
        return -1;
    }    
    

    pid = fork();

    if(pid < 0){
        // FAILS
        printf("error in fork");
        return 1;
    }
    
    struct flock fl;

    if(pid > 0){
        // PARENT PROCESS
        //set the lock
        if(flock(fd2, LOCK_EX|LOCK_NB)== -1){
            perror("lock file failed\n");
            return -1;
        }
        //append 'b'
        write(fd2, "b", 1);        
        //unlock
        flock(fd2, LOCK_UN);
        sleep(3);
        //read the file using stat
        struct stat st;
        if(fstat(fd2, &st)==-1){
            perror("fstat failed\n");
            return -1;
        };
        char *buf = (char *)malloc(st.st_size);
        lseek(fd2, 0, SEEK_SET);
        if(read(fd2, buf, st.st_size)==-1){
            perror("read failed\n");
            return -1;
        }
        printf("%s", buf); // the feedback should be 'hello fcntl!ba'
        // printf("%s", str); the feedback should be 'hello fcntl!ba'
        exit(0);

    } else {
        // CHILD PROCESS
        sleep(2);
        //get the lock
        if(flock(fd2, LOCK_EX|LOCK_NB)== -1){
            perror("lock file failed\n");
            return -1;
        }
        //append 'a'
        write(fd2, "a", 1);
        flock(fd2, LOCK_UN);
        exit(0);
    }
    close(fd);
    return 0;
}