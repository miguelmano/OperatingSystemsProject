#include "tecnicofs_client_api.h"
#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int pipeClient,pipeServer;
int session_id;
char pathnameClient[40];


int tfs_mount(char const *client_pipe_path, char const *server_pipe_path) {
    void* toServer;
    int toClient,op_code=1;
    ssize_t aux;
    if (unlink(client_pipe_path) != 0 && errno != ENOENT) {return -1;}
    if (mkfifo (client_pipe_path, 0777) != 0){
        return -1;
    }
    pipeClient = open (client_pipe_path, O_RDWR);
    pipeServer = open (server_pipe_path, O_RDWR);
    if (pipeClient == -1){
        return -1;
        }
    if (pipeServer == -1){
        return -1;
        }
    
    toServer=malloc(sizeof(int)+sizeof(char)*40);
    memcpy(toServer, &op_code, sizeof(int));
    memcpy(toServer+sizeof(int), client_pipe_path, sizeof(char)*40);  
    memcpy(pathnameClient,client_pipe_path,strlen(client_pipe_path));  
    if(write (pipeServer, toServer, sizeof(toServer))<=0){
        free(toServer);
        return -1;
        }
    aux=read(pipeClient, &toClient, sizeof(int));
    if((int)aux==0){
        free(toServer);
        return -1;
        }
    session_id = toClient;     
    free(toServer);       /*LEMBRA TE DE FORMATAR BEM TOCLIENT*/
    return 0;
}

int tfs_unmount() {
    /* TODO: Implement this */
    void* toServer;
    int aux,op_code=2;
    toServer = malloc(2*sizeof(int));
    memcpy(toServer, &op_code, sizeof(int));
    memcpy(toServer+sizeof(int), &session_id, sizeof(int));
    if(write (pipeServer, toServer, sizeof(toServer))<=0){
        free(toServer);
        return -1;
        }
    aux = unlink(pathnameClient);
    if (aux == -1 && errno!= ENOENT){
        free(toServer);
        return -1;
        }
        free(toServer);
    return 0;
}

int tfs_open(char const *name, int flags) {
    /* TODO: Implement this */
    void* pointer;
    void* toServer;        /*maybe needs a fix who knows*/
    int toClient;
    int op_code=3;
    ssize_t aux;
    toServer=malloc(3*sizeof(int)+sizeof(char)*strlen(name));
    memcpy(toServer, &op_code, sizeof(int));
    memcpy(toServer+sizeof(int), &session_id, sizeof(int));
    pointer=toServer+sizeof(int)+sizeof(int);
    memcpy(pointer, name, sizeof(char)*strlen(name));
    pointer+=sizeof(name);
    memcpy(pointer, &flags, sizeof(int));
    if(write (pipeServer, toServer, sizeof(toServer))<=0){
        free(toServer);
        return -1;
        }
    aux=read(pipeClient, &toClient, sizeof(int));
    if ((int)aux==0){
        free(toServer);
        return -1;
        }
    free(toServer);
    return toClient;
}

int tfs_close(int fhandle) {
    /* TODO: Implement this */
    void* toServer;
    int toClient;
    int op_code=4;
    ssize_t aux;
    toServer=malloc(3*sizeof(int));
    memcpy(toServer, &op_code, sizeof(int));
    memcpy(toServer+sizeof(int), &session_id, sizeof(int));
    memcpy(toServer+sizeof(int)+sizeof(int), &fhandle, sizeof(int));
    if(write (pipeServer, toServer, sizeof(toServer))<=0){
        free(toServer);
        return -1;
        }
    aux=read(pipeClient, &toClient, sizeof(int));
    if ((int)aux==0){
        free(toServer);
        return -1;
        }
    free(toServer);
    return toClient;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t len) {
    /* TODO: Implement this */
    void* toServer;
    ssize_t toClient;
    int op_code=5;
    void* pointer;  
    ssize_t aux;  
    toServer=malloc(3*sizeof(int)+sizeof(size_t)+ sizeof(char)*len);          /*maybe is worng who knows*/
    memcpy(toServer, &op_code, sizeof(int));
    memcpy(toServer+sizeof(int), &session_id, sizeof(int));
    memcpy(toServer+sizeof(int)+sizeof(int), &fhandle, sizeof(int));
    pointer=toServer+sizeof(int)+sizeof(int)+sizeof(int);
    memcpy(pointer, &len, sizeof(size_t));
    pointer+=sizeof(size_t);
    memcpy(pointer, buffer, sizeof(char)*len);
    if(write (pipeServer, toServer, sizeof(toServer))<=0){
        free(toServer);
        return -1;
        }
    aux=read(pipeClient, &toClient, 1);
    if ((int)aux==0){
        free(toServer);
        return -1;
        }
    free(toServer);
    return toClient;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    /* TODO: Implement this */
    void* toClient; 
    void* toServer;
    void* pointer;
    int op_code=6;
    ssize_t lenght,aux;
    toServer=malloc(3*sizeof(int)+sizeof(size_t));
    memcpy(toServer, &op_code, sizeof(int));
    memcpy(toServer+sizeof(int), &session_id, sizeof(int));
    memcpy(toServer+sizeof(int)+sizeof(int), &fhandle, sizeof(int));
    pointer=toServer+sizeof(int)+sizeof(int)+sizeof(int);
    memcpy(pointer, &len, sizeof(size_t));
    if(write (pipeServer, toServer, sizeof(toServer))<=0){
        free(toServer);
        return -1;
        }
    toClient=malloc(sizeof(char)*len+sizeof(int));
    aux=read(pipeClient, toClient, sizeof(toClient));
    memcpy(&lenght,toClient,sizeof(int));  
    memcpy(buffer,toClient+sizeof(int),(size_t)lenght* sizeof(char));     
    if((int)aux==0 || (int)lenght==-1){
        free(toServer);
        free(toClient);
        return -1;
        }
    free(toClient);
    free(toServer);
    return lenght;
}

int tfs_shutdown_after_all_closed() {
    /* TODO: Implement this */
    void* toServer;   
    int toClient;
    ssize_t aux;
    int op_code=7;
    toServer=malloc(2*sizeof(int));
    memcpy(toServer, &op_code, sizeof(int));
    memcpy(toServer+sizeof(int), &session_id, sizeof(int));
    if(write (pipeServer, toServer, sizeof(toServer))<=0){
        free(toServer);
        return -1;
        }
    aux=read(pipeClient, &toClient, 1);
    if ((int)aux==0){
        free(toServer);
        return -1;
        }
    free(toServer);
    return toClient;
}
