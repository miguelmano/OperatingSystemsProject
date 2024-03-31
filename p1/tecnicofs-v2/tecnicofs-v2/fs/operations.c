#include "operations.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tfs_init() {
    state_init();

    /* create root inode */
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }
    return 0;
}

int tfs_destroy() {
    state_destroy();
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}


int tfs_lookup(char const *name) {
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(ROOT_DIR_INUM, name);
}

int tfs_open(char const *name, int flags) {
    int inum;
    size_t offset;


    /* Checks if the path name is valid */
    if (!valid_pathname(name)) {
        return -1;
    }

    inum = tfs_lookup(name);
    if (inum >= 0) {
        /* The file already exists */
        inode_t *inode = inode_get(inum);
        if (inode == NULL) {
            return -1;
        }

        /* Trucate (if requested) */
        if (flags & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                if (data_block_free(inode->i_data_block) == -1) {
                    return -1;
                }
                inode->i_size = 0;
            }
        }
        /* Determine initial offset */
        if (flags & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
    } else if (flags & TFS_O_CREAT) {
        /* The file doesn't exist; the flags specify that it should be created*/
        /* Create inode */
        inum = inode_create(T_FILE);
        if (inum == -1) {
            return -1;
        }
        /* Add entry in the root directory */
        if (add_dir_entry(ROOT_DIR_INUM, inum, name + 1) == -1) {
            inode_delete(inum);
            return -1;
        }
        offset = 0;
    } else {
        return -1;
    }
    /* Finally, add entry to the open file table and
     * return the corresponding handle */
    return add_to_open_file_table(inum, offset);

    /* Note: for simplification, if file was created with TFS_O_CREAT and there
     * is an error adding an entry to the open file table, the file is not
     * opened but it remains created */
}


int tfs_close(int fhandle) { return remove_from_open_file_table(fhandle); }

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write){
    int bufferPos = 0;
    int capacity,block_offset;
    open_file_entry_t *file = get_open_file_entry(fhandle);
    pthread_rwlock_wrlock(&file->rwlock);
    if (file == NULL) {
        pthread_rwlock_unlock(&file->rwlock);
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    pthread_rwlock_wrlock(&inode->rwlock);
    if (inode == NULL) {
        pthread_rwlock_unlock(&file->rwlock);
        pthread_rwlock_unlock(&inode->rwlock);
        return -1;
    }
    int blockIndex = ((int)(file->of_offset + to_write))/1024;
    int currentBlockIndex = ((int)file->of_offset)/1024;
    int *block;
    if (to_write > 0){
        if (currentBlockIndex < blockIndex) {
            /* If empty file, allocate new block */
            for (int i = currentBlockIndex;i<blockIndex && i < 10 ;i++){
                inode->datablocks[i] = data_block_alloc();
            }
            if(blockIndex >= 10){
                if (currentBlockIndex<10){
                    inode->i_data_block = data_block_alloc();
                }
                int *blockPointers = (int*)data_block_get(inode->i_data_block);
                for (int i = currentBlockIndex; i <= blockIndex; i++){

                    blockPointers[i-10]= data_block_alloc();
                }
            }
        }
        for (int i = currentBlockIndex;i<=blockIndex && to_write > 0 ;i++){
            block_offset =(int)( file->of_offset % BLOCK_SIZE);
            if ( i < 10){
                block = data_block_get(inode->datablocks[i]);
            }
            else {
                void *block_i = data_block_get(inode->i_data_block);
                block = data_block_get(((int*)block_i)[i-10]);
            }
            capacity = BLOCK_SIZE - block_offset;
            /* Perform the actual write */
            if(to_write>capacity){
                file->of_offset+=(size_t)capacity;
                bufferPos+=capacity;
                memcpy(block + block_offset, buffer,(size_t)capacity);
                to_write = to_write - (size_t)capacity;
            }
            else{
                file->of_offset+=to_write;
                bufferPos+=(int)to_write;
                memcpy(block + block_offset, buffer,(size_t)to_write);
                to_write = 0;
            }
        }
        
        /* The offset associated with the file handle is
         * incremented accordingly */
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
    }
    pthread_rwlock_unlock(&inode->rwlock);
    pthread_rwlock_unlock(&file->rwlock);
    return (ssize_t)bufferPos;
}


ssize_t tfs_read(int fhandle, void *buffer, size_t len){
    int bufferPos = 0;
    int block_offset;
    void *block;
    open_file_entry_t *file = get_open_file_entry(fhandle);
    pthread_rwlock_rdlock(&file->rwlock);
    if (file == NULL) {
        pthread_rwlock_unlock(&file->rwlock);
        return -1;
    }
    
    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    pthread_rwlock_rdlock(&inode->rwlock);
    if (inode == NULL) {
        pthread_rwlock_unlock(&inode->rwlock);
        pthread_rwlock_unlock(&file->rwlock);
        return -1;
    }

    /* Determine how many bytes to read */
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }
    int blockIndex = ((int)(file->of_offset + to_read))/1024;
    if (to_read > 0 ) {
        for (int i = 0;i <= blockIndex && to_read > 0;i++){
            block_offset = (int)(file->of_offset % BLOCK_SIZE);
            if ( i < 10){
                block = data_block_get(inode->datablocks[i]);
            }
            else{
                void *block_i = data_block_get(inode->i_data_block);
                block = data_block_get(((int*)block_i)[i-10]);
            }
            if (block_offset + (int)to_read <= BLOCK_SIZE){
                memcpy(buffer, block + (size_t)block_offset, to_read);
                file->of_offset+= to_read;
                bufferPos += (int)to_read;
            }
            else{
                memcpy(buffer, block + (size_t)block_offset , BLOCK_SIZE - (size_t)block_offset);
                file->of_offset+= (size_t) (BLOCK_SIZE -block_offset);
                bufferPos += BLOCK_SIZE - block_offset;
                to_read -= (size_t)(BLOCK_SIZE - block_offset);
            }
        }
        
    }
    pthread_rwlock_unlock(&inode->rwlock);
    pthread_rwlock_unlock(&file->rwlock);
    return (ssize_t)bufferPos;
}

int tfs_copy_to_external_fs(char const *source_path, char const *dest_path){
    int fhandle = tfs_open(source_path,0);
    int flag;
    char *buffer;
    FILE *fhandle2;
    open_file_entry_t *file = get_open_file_entry(fhandle);
    inode_t *inode = inode_get(file->of_inumber);
    buffer=(char*)malloc(sizeof(char)*inode->i_size);
    if (fhandle == -1){
        return -1;
    }
    else{
        flag=(int)tfs_read(fhandle, buffer, inode->i_size);
        if (flag==-1){
            return -1;
        }
        else{
            fhandle2 = fopen(dest_path,"w");
            if (fhandle2 == NULL || fhandle == -1){
                return -1;
            }
            flag=(int)fwrite(buffer,sizeof(buffer[0]), inode->i_size, fhandle2);
            if (flag==-1){
                return -1;
            }
        }
    }
    tfs_close(fhandle);
    fclose(fhandle2);
    free(buffer);
    return 0;
}   
