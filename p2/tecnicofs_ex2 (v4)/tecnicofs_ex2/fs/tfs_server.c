#include "operations.h"
#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

int session_id=0;
int sessions[10];

int main(int argc, char **argv) {
    int pipeServer, pipeClient = -1;
    char *pipename = argv[1];

    for(int i = 0;i < 10;i++){
        sessions[i]=-1;
    }

    if (argc < 2) {
        printf("Please specify the pathname of the server's pipe.\n");
        return 1;
    }

    if(unlink(pipename)!=0 && errno != ENOENT){printf("[ERR]Unlink Failed\n");}
    if(mkfifo (pipename, 0777) != 0 ){printf("[ERR]Error opening pipe\n");}
    tfs_init();
    printf("Starting TecnicoFS server with pipe called %s\n", pipename);

    /* TO DO */
    
    pipeServer = open (pipename, O_RDWR);
    if (pipeServer == -1){printf("[ERR]: Opening Pipe Failed\n");}
    while(1){
        size_t len;
        char* string;
        char* buffer_read;
        void* pointer;
        char name[40];
        int flags;
        void* fromClient;
        void* toClient;
        int op_code;
        int s_id,fhandle,aux;
        char* pathname;
        int counter=0;
        fromClient=malloc(1024);
        ssize_t size = read(pipeServer, fromClient, 1024);
        if (size == 0) {
            // ret == 0 signals EOF
            printf("[INFO]: pipe closed\n");
            free(fromClient);
            return 1;
        
        }
        else if (size == -1) { 
            printf("[ERR]: read failed\n");
            free(fromClient);
            return -1;
        }
        memcpy(&op_code,fromClient,sizeof(int));
        switch (op_code){
            case 1:
                for(int i = 0;i<10;i++){
                    if (sessions[i]==-1){counter++;}
                }
                if (counter>=1){
                    memcpy(pathname,fromClient+sizeof(int),40*sizeof(char));
                    pipeClient=open(pathname,O_RDWR);
                    if(pipeClient == -1){printf("[ERR]: Opening Pipe Failed\n");}
                    for(int i = 0;i < 10;i++){
                        if(sessions[i]==-1){
                            memcpy(&sessions[i],&pipeClient,sizeof(int));
                            session_id=i;
                            break;
                        }
                    }
                    if(write(pipeClient,&session_id,sizeof(int))<=0){
                        printf("[ERR]Error writing\n");
                    }
                }
                break;
            case 2:
                aux=close(pipeClient);
                if(aux==1){printf("[ERR]: Closing Pipe Failed\n");}
                aux=close(pipeServer);
                if(aux==1){printf("[ERR]: Closing Pipe Failed\n");}
                memcpy(&s_id,fromClient+sizeof(int),sizeof(int));
                sessions[s_id]=-1;
                break;
            case 3:
                memcpy(&s_id,fromClient+sizeof(int),sizeof(int));
                if (sessions[s_id] != -1){
                    pointer=fromClient+sizeof(int)+sizeof(int);
                    memcpy(name,pointer,sizeof(char)*40);
                    pointer+=sizeof(char)*40;
                    memcpy(&flags,pointer,sizeof(int));
                    aux = tfs_open(name,flags);
                    if(write(pipeClient,&aux,sizeof(int))<=0){
                        printf("[ERR]Error writing\n");
                    }
                }
                break;
            case 4:
                memcpy(&s_id,fromClient+sizeof(int),sizeof(int));
                if (sessions[s_id] != -1){
                    pointer=fromClient+sizeof(int)+sizeof(int);
                    memcpy(&fhandle,pointer,sizeof(int));
                    aux = tfs_close(fhandle); 
                    if(write(pipeClient,&aux,sizeof(int))<=0){
                        printf("[ERR]Error writing\n");
                    }
                }
                break;
            case 5:
                memcpy(&s_id,fromClient+sizeof(int),sizeof(int));
                if (sessions[s_id] != -1){
                    pointer=fromClient+sizeof(int)+sizeof(int);
                    memcpy(&fhandle,pointer,sizeof(int));
                    pointer+=sizeof(int);
                    memcpy(&len, pointer, sizeof(size_t));
                    pointer+=sizeof(size_t);
                    string=(char*)malloc(len);
                    memcpy(string, pointer, len);
                    ssize_t aux1 = tfs_write(fhandle,string,len); 
                    if(write(pipeClient,&aux1,sizeof(int))<=0){
                        printf("[ERR]Error writing\n");
                    }
                    free(string);
                }
                break;

            case 6:
                memcpy(&s_id,fromClient+sizeof(int),sizeof(int));
                if (sessions[s_id] != -1){
                    pointer=fromClient+sizeof(int)+sizeof(int);
                    memcpy(&fhandle,&pointer,sizeof(int));
                    pointer+=(int)sizeof(int);
                    memcpy(&len, &pointer, sizeof(size_t));
                    buffer_read=(char*)malloc(len);
                    ssize_t aux2 = tfs_read(fhandle,buffer_read,len);
                    toClient=malloc(sizeof(int)+(size_t)aux2);
                    memcpy(toClient,&aux2,sizeof(int));
                    memcpy(toClient + sizeof(int),buffer_read,(size_t)aux2);
                    if(write(pipeClient,toClient,sizeof(toClient))<=0){
                        printf("[ERR]Error writing\n");
                    }
                    free(buffer_read);
                    free(toClient);
                }
                break;
            case 7:
                memcpy(&s_id,fromClient+sizeof(int),sizeof(int));
                if (sessions[s_id] != -1){
                    aux = tfs_destroy_after_all_closed();
                    if (pipeClient != -1){
                        if(write(pipeClient,&aux,sizeof(int))<=0){
                            printf("[ERR]Error writing\n");
                        }
                    }
                    free(fromClient);
                    return 0;
                }
                break;
            default:
                {printf("[ERR]: Op Code not valid\n");}
                break;
        }    
    }
    
    return 0;
}