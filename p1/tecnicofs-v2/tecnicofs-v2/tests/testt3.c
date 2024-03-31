#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <string.h>
int o = 0;
void *hopen(){
    int f = tfs_open("/f2",TFS_O_CREAT); 
    o++;
    if (f == -1){
        printf("%d\n",o);
        return NULL;
    }
    int b = (int)tfs_write(f,"234567",6);
    assert(b == 6);
    assert(tfs_close(f) == -1);
    return NULL;
}

int main(){

    tfs_init();
    pthread_t pthreads[21];
    for (int i = 0; i < 21; i++){
        pthread_create(&pthreads[i], NULL, hopen, NULL);
    }
    for (int i = 0; i < 21; i++){
        pthread_join(pthreads[i], NULL);
    }
    printf("Successful Test\n");
    return 0;
}