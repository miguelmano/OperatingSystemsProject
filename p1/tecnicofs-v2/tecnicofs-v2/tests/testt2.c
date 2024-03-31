#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <string.h>

void *hwrite(){
    int f = tfs_open("/f2",TFS_O_CREAT);
    int b = (int)tfs_write(f,"234567",6);
    assert(b == 6);
    assert(tfs_close(f) != -1);
    return NULL;
}

int main(){

    tfs_init();
    char string[600];
    pthread_t pthreads[100];
    for (int i = 0; i < 100; i++){
        pthread_create(&pthreads[i], NULL, hwrite, NULL);
    }
    for (int i = 0; i < 100; i++){
        pthread_join(pthreads[i], NULL);
    }
    int f = tfs_open("/f1", TFS_O_CREAT);
    int b = (int)tfs_read(f, string, 600);
    assert( b == 600);
    assert(tfs_close(f) != -1);
    printf("Successful Test\n");
    return 0;
}