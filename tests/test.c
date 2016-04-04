#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "../mailslot_ioctl.h"

#define NODE_PATH "/tmp/mailslot_test"
#define MAJOR 250
#define MINOR 30

int test_default_iomode(int fd){
    unsigned int is_nonblocking;
    int ret;

    ret = ioctl(fd, MAILSLOT_IOC_IS_NONBLOCKING, &is_nonblocking);
    if(ret){
        printf("Failed to perform ioctl to get block mode: %s\n", strerror(errno));
        return ret;
    }
    if(is_nonblocking){
        printf("Assertion failed: blocking mode not enabled\n");
        return -1;
    }
    return 0;
}

int test_set_nonblocking(int fd, unsigned int want_nonblocking){
    unsigned int is_nonblocking;
    int ret;

    ret = ioctl(fd, MAILSLOT_IOC_SET_NONBLOCKING, &want_nonblocking);
    if(ret){
        printf("Failed to set blocking mode\n");
        return ret;
    }

    ret = ioctl(fd, MAILSLOT_IOC_IS_NONBLOCKING, &is_nonblocking);
    if(ret){
        printf("Failed to perform ioctl to get blocking mode: %s\n", strerror(errno));
        return ret;
    }
    if((want_nonblocking && (!is_nonblocking)) || ((!want_nonblocking) && is_nonblocking)){
        printf("Assertion failed: blocking mode different from what has been requested\n");
        return -1;
    }
    return 0;
}

int test_resize_enlarge(int fd){
    int ret;
    unsigned int size = DEFAULT_SLOT_SIZE << 1;
    char * out[3] = {"ciao1", "ciao2", "ciao3"};
    char in[5];
    int i;

    for(i=0; i<(sizeof(out)/sizeof(out[0])); i++){
        ret = write(fd, out[i], 5);
        if(ret < 0){
            printf("Failed to write: %s\n", strerror(errno));
            return ret;
        }
    }

    ret = ioctl(fd, MAILSLOT_IOC_RESIZE, &size);
    if(ret){
        printf("Failed to perform ioctl to resize mailslot: %s\n:", strerror(errno));
        return ret;
    }

    for(i=0; i<(sizeof(out)/sizeof(out[0])); i++){
        ret = read(fd, in, 5);
        if(ret < 0){
            printf("Failed to read\n");
            return ret;
        }
        ret = strncmp(out[i], in, 5);
        if(ret != 0){
            printf("Different content\n");
            return ret;
        }
    }
    return ret;
}

int test_resize_on_boundaries(int fd){
    int ret;
    const unsigned int default_slot_size = DEFAULT_SLOT_SIZE;
    unsigned int start_size = 16;
    unsigned int final_size = 8;
    char * text_placeholder = "1234";
    char in[4];
    int i;

    ret = ioctl(fd, MAILSLOT_IOC_RESIZE, &start_size);
    if(ret){
        printf("Failed to perform ioctl to shrink mailslot: %s\n:", strerror(errno));
        return ret;
    }

    for(i=0; i<2; i++){
        ret = write(fd, text_placeholder, strlen(text_placeholder));
        if(ret < 0){
            printf("Failed to write: %s\n", strerror(errno));
            return ret;
        }
    }

    for(i=0; i<2; i++){
        ret = read(fd, in, strlen(text_placeholder));
        if(ret < 0){
            printf("Failed to read\n");
            return ret;
        }
    }

    ret = write(fd, text_placeholder, strlen(text_placeholder));
    if(ret < 0){
        printf("Failed to write: %s\n", strerror(errno));
        return ret;
    }


    ret = ioctl(fd, MAILSLOT_IOC_RESIZE, &final_size);
    if(ret){
        printf("Failed to perform ioctl to shrink mailslot: %s\n:", strerror(errno));
        return ret;
    }

    ret = read(fd, in, strlen(text_placeholder));
    if(ret < 0){
        printf("Failed to write: %s\n", strerror(errno));
        return ret;
    }
    if((ret = strncmp(in, text_placeholder, strlen(text_placeholder)))){
        printf("Different content\n");
        return ret;
    }

    ret = ioctl(fd, MAILSLOT_IOC_RESIZE, &default_slot_size);
    if(ret){
        printf("Failed to perform ioctl to shrink mailslot: %s\n:", strerror(errno));
        return ret;
    }

    return 0;
}


int main(int argc, char ** argv){
    int ret;
    int fd;
    char c;

    printf("Creating node\n");
    ret = mknod(NODE_PATH, S_IFCHR, makedev(MAJOR,MINOR));
    if(ret){
        if(errno == EEXIST){
            printf("Device already exists\n");
        }else{
            printf("Failed to create node: %s\n", strerror(errno));
            return ret;
        }
    }

    fd = open(NODE_PATH, O_RDWR);
    if(fd == -1){
        printf("Failed to open device: %s\n", strerror(errno));
        return -1;
    }

    ret = test_default_iomode(fd);
    if(ret){
        printf("Failed test_default_iomode\n");
        return ret;
    }

    ret = test_set_nonblocking(fd, 0);
    if(ret){
        printf("Failed test_set_nonblocking off\n");
        return ret;
    }

    ret = test_set_nonblocking(fd, 1);
    if(ret){
        printf("Failed test_set_nonblocking on\n");
        return ret;
    }

    printf("Should not hang on empty mailslot in nonblocking mode...\n");
    ret = read(fd, &c, 1);
    if(ret != -1 || errno != EAGAIN){
        printf("Failed to read in non blocking mode: ret: %d\n", ret);
        return -1;
    }

    ret = test_resize_enlarge(fd);
    if(ret){
        printf("Failed test_resize_enlarge..\n");
    }

    ret = test_resize_on_boundaries(fd);
    if(ret){
        printf("Failed test_resize_on_boundaries..\n");
    }

    return 0;
}
