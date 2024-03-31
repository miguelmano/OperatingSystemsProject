#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <string.h>

void *f0(){
    int f = tfs_open("/f1", TFS_O_CREAT);
    int b = (int)tfs_write(f, "234567", 6);
    assert(b == 6);
    assert(tfs_close(f) != -1);
    return NULL;
}

void *f1(){
    char string[6];
    int f = tfs_open("/f1", TFS_O_CREAT);
    int b = (int)tfs_read(f, string, 6);
    assert(b == 6);
    assert(strlen(string) == b);
    assert(tfs_close(f) != -1);
    return NULL;
}

int main(){

    tfs_init();
    pthread_t pthreads[100];
    for (int i = 0; i < 100; i++){
        if ( i % 2 > 0){
            pthread_create(&pthreads[i], NULL, f0, NULL);
        }
        else{
            pthread_create(&pthreads[i], NULL, f1, NULL);
        }
    }
    for (int i = 0; i < 100; i++){
        pthread_join(pthreads[i], NULL);
    }
    printf("Successful Test\n");

    return 0;
}