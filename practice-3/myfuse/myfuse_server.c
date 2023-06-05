// implement a file system using fuse, all files are stored in memory

#define FUSE_USE_VERSION 30

#include <fuse.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "socketchat.h"
#include <sys/stat.h>

// the file system is stored in memory

#define MAX_FILE_SIZE 1024

struct myfuse_file {
    char *name;
    char *data;
    int size;
    struct myfuse_file *children;
    struct myfuse_file *parent;
    struct myfuse_file *sibling;
    int is_dir;
    int mode;
    int uid;
    int gid;
};

int change_owner(struct myfuse_file *file, int uid, int gid){
    if(file==NULL){
        return -1;
    }
    file->uid = uid;
    file->gid = gid;
    return 0;
}

int change_mode(struct myfuse_file *file, struct mode_t mode){
    if(file==NULL){
        return -1;
    }
    file->mode = mode;
    return 0;
}

struct myfuse_file* root = NULL;
struct myfuse_file* chat_client = NULL;
int serverfd = -1;
int myfuse_file_num = 0;

// init file system
void* myfuse_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    // init conn cfg
    (void) conn;
    cfg->kernel_cache = 1;
    // init root
    root = (struct myfuse_file*)malloc(sizeof(struct myfuse_file));
    root->name = (char*)malloc(sizeof(char) * 2);
    strcpy(root->name, "/");
    root->data = NULL;
    root->size = 0;
    root->children = NULL;
    root->parent = NULL;
    root->sibling = NULL;
    root->is_dir = 1;
    myfuse_file_num = 1;
    // set mod and own
    int uid, gid;
    struct fuse_context *context = fuse_get_context();
    if(context){
        uid = context->uid;
        gid = context->gid;
    }
    // change own and mode here
    int ret = 0;
    struct stat stbuf;
    // 获取文件/目录的元数据信息
    ret = lstat("/", &stbuf);
    if (ret == -1) {
        printf("ret == -1\n");
        exit(1);
    }
    // 更新用户 ID 和组 ID 值
    stbuf.st_uid = uid;
    stbuf.st_gid = gid;
    // 更新权限位部分
    stbuf.st_mode &= ~0777; // 清除旧权限位
    stbuf.st_mode |= 0755; // 设置新权限位
    // 将更新后的用户 ID 和组 ID 值写入文件/目录的元数据信息中
    ret = change_owner(root, stbuf.st_uid, stbuf.st_gid);
    if (ret == -1) {
        printf("ret == -1\n");
        exit(1);
    }
    // 将修改后的权限位写入文件/目录的元数据信息中
    ret = change_mode(root, stbuf.st_mode);
    if (ret == -1) {
        printf("ret == -1\n");
        exit(1);
    }
    // create chat file with client
    serverfd = server_accept();
    chat_client = (struct myfuse_file*)malloc(sizeof(struct myfuse_file));
    chat_client->name = (char*)malloc(sizeof(char) * 12);
    strcpy(chat_client->name, "chat_client");
    chat_client->data = NULL;
    chat_client->size = 0;
    chat_client->children = NULL;
    chat_client->parent = root;
    chat_client->sibling = NULL;
    chat_client->is_dir = 0;
    root->children = chat_client;
    myfuse_file_num++;
    // set mod and own
    if(context){
        uid = context->uid;
        gid = context->gid;
    }
    // change own and mode here
    // 获取文件/目录的元数据信息
    ret = lstat("/chat_client", &stbuf);
    if (ret == -1) {
        printf("ret == -1\n");
        exit(1);
    }
    // 更新用户 ID 和组 ID 值
    stbuf.st_uid = uid;
    stbuf.st_gid = gid;
    // 更新权限位部分
    stbuf.st_mode &= ~0777; // 清除旧权限位
    stbuf.st_mode |= 0644; // 设置新权限位
    // 将更新后的用户 ID 和组 ID 值写入文件/目录的元数据信息中
    ret = change_owner(chat_client, stbuf.st_uid, stbuf.st_gid);
    if (ret == -1) {
        printf("ret == -1\n");
        exit(1);
    }
    // 将修改后的权限位写入文件/目录的元数据信息中
    ret = change_mode(chat_client, stbuf.st_mode);
    if (ret == -1) {
        printf("ret == -1\n");
        exit(1);
    }
    return NULL;
}

// find a file by path
struct myfuse_file* myfuse_find_file(const char *path) {
    if (path == NULL) {
        return NULL;
    }
    if (strcmp(path, "/") == 0) {
        return root;
    }
    struct myfuse_file *cur = root;
    char *path_copy = (char*)malloc(sizeof(char) * (strlen(path) + 1));
    strcpy(path_copy, path);
    char *token = strtok(path_copy, "/");
    while (token != NULL) {
        int found = 0;
        struct myfuse_file *child = cur->children;
        while (child != NULL) {
            if (strcmp(child->name, token) == 0) {
                cur = child;
                found = 1;
                break;
            }
            child = child->sibling;
        }
        if (found == 0) {
            return NULL;
        }
        token = strtok(NULL, "/");
    }
    return cur;
}

// get file attributes
int myfuse_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    struct myfuse_file *file = myfuse_find_file(path);
    if (file == NULL) {
        return -ENOENT;
    }
    if(file==chat_client){
        char *buffer = (char*)malloc(sizeof(char) * (1024 + 1));
        printf("read from server\n");
        int r = server_rw(buffer, 1024, 0, serverfd);// 0 for read
        if(r==-1){}
        else{
            if(file->data==NULL){
                file->data = (char*)malloc(sizeof(char) * r);
            } else {
                file->data = (char*)realloc(file->data, sizeof(char) * r);
            }
            memcpy(file->data, buffer, r);
            file->size = r;
            free(buffer);
        }
    }
    memset(stbuf, 0, sizeof(struct stat));
    if (file->is_dir == 1) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size = file->size;
    }
    return 0;
}

// read directory
int myfuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    struct myfuse_file *file = myfuse_find_file(path);
    if (file == NULL) {
        return -ENOENT;
    }
    if (file->is_dir == 0) {
        return -ENOTDIR;
    }
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    struct myfuse_file *child = file->children;
    while (child != NULL) {
        filler(buf, child->name, NULL, 0, 0);
        child = child->sibling;
    }
    return 0;
}

// create a directory
int myfuse_mkdir(const char *path, mode_t mode) {
    struct myfuse_file *file = myfuse_find_file(path);
    if (file != NULL) {
        return -EEXIST;
    }
    printf("set mod and own\n");
    int uid, gid;
    struct fuse_context *context = fuse_get_context();
    if(context){
        uid = context->uid;
        gid = context->gid;
    }
    // change own and mode here
    int ret = 0;
    struct stat stbuf;
    // 获取文件/目录的元数据信息
    ret = lstat(path, &stbuf);
    if (ret == -1) {
        printf("ret == -1\n");
        exit(1);
    }
    // 更新用户 ID 和组 ID 值
    stbuf.st_uid = uid;
    stbuf.st_gid = gid;
    // 更新权限位部分
    stbuf.st_mode &= ~0777; // 清除旧权限位
    stbuf.st_mode |= mode; // 设置新权限位
    // 将更新后的用户 ID 和组 ID 值写入文件/目录的元数据信息中
    ret = change_owner(file, stbuf.st_uid, stbuf.st_gid);
    if (ret == -1) {
        printf("ret == -1\n");
        exit(1);
    }
    // 将修改后的权限位写入文件/目录的元数据信息中
    ret = change_mode(file, stbuf.st_mode);
    if (ret == -1) {
        printf("ret == -1\n");
        exit(1);
    }
    char *path_copy = (char*)malloc(sizeof(char) * (strlen(path) + 1));
    strcpy(path_copy, path);
    char *token = strtok(path_copy, "/");
    struct myfuse_file *cur = root;
    while (token != NULL) {
        struct myfuse_file *child = cur->children;
        int found = 0;
        while (child != NULL) {
            if (strcmp(child->name, token) == 0) {
                cur = child;
                found = 1;
                break;
            }
            child = child->sibling;
        }
        if (found == 0) {
            break;
        }
        token = strtok(NULL, "/");
    }
    struct myfuse_file *new_file = (struct myfuse_file*)malloc(sizeof(struct myfuse_file));
    new_file->name = (char*)malloc(sizeof(char) * (strlen(token) + 1));
    strcpy(new_file->name, token);
    new_file->data = NULL;
    new_file->size = 0;
    new_file->children = NULL;
    new_file->parent = cur;
    new_file->sibling = NULL;
    new_file->is_dir = 1;
    if (cur->children == NULL) {
        cur->children = new_file;
    } else {
        new_file->sibling = cur->children;
        cur->children = new_file;
    }
    myfuse_file_num++;
    return 0;
}

// remove a directory
int myfuse_rmdir(const char *path) {
    struct myfuse_file *file = myfuse_find_file(path);
    if (file == NULL) {
        return -ENOENT;
    }
    if (file->is_dir == 0) {
        return -ENOTDIR;
    }
    if (file->children != NULL) {
        return -ENOTEMPTY;
    }
    if (file->parent->children == file) {
        file->parent->children = file->sibling;
    } else {
        struct myfuse_file *sibling = file->parent->children;
        while (sibling->sibling != file) {
            sibling = sibling->sibling;
        }
        sibling->sibling = file->sibling;
    }
    if(file->data != NULL)
        free(file->data);
    free(file->name);
    free(file);
    myfuse_file_num--;
    return 0;
}

// rename a file
int myfuse_rename(const char *oldpath, const char *newpath, unsigned int flags) {
    struct myfuse_file *file = myfuse_find_file(oldpath);
    if (file == NULL) {
        return -ENOENT;
    }
    if (myfuse_find_file(newpath) != NULL) {
        return -EEXIST;
    }
    if(file==chat_client){
        return -EBUSY;
    }
    char *newpath_copy = (char*)malloc(sizeof(char) * (strlen(newpath) + 1));
    strcpy(newpath_copy, newpath);
    char *token = strtok(newpath_copy, "/");
    struct myfuse_file *cur = root;
    while (token != NULL) {
        struct myfuse_file *child = cur->children;
        int found = 0;
        while (child != NULL) {
            if (strcmp(child->name, token) == 0) {
                cur = child;
                found = 1;
                break;
            }
            child = child->sibling;
        }
        if (found == 0) {
            break;
        }
        token = strtok(NULL, "/");
    }
    free(file->name);
    file->name = (char*)malloc(sizeof(char) * (strlen(token) + 1));
    strcpy(file->name, token);
    file->parent = cur;
    file->sibling = cur->children;
    cur->children = file;
    return 0;
}

// truncate a file
int myfuse_truncate(const char *path, off_t size, struct fuse_file_info *fi) {
    struct myfuse_file *file = myfuse_find_file(path);
    if (file == NULL) {
        return -ENOENT;
    }
    if (file->is_dir == 1) {
        return -EISDIR;
    }
    if (file->data == NULL) {
        file->data = (char*)malloc(sizeof(char) * size);
        memset(file->data, 0, sizeof(char) * size);
    } else {
        file->data = (char*)realloc(file->data, sizeof(char) * size);
    }
    file->size = size;
    return 0;
}

// change file mode
// change file owner
// open a file
int myfuse_open(const char *path, struct fuse_file_info *fi) {
    struct myfuse_file *file = myfuse_find_file(path);
    if (file == NULL) {
        return -ENOENT;
    }
    if (file->is_dir == 1) {
        return -EISDIR;
    }
    //judge whether truncate file
    if(fi->flags & O_TRUNC){
        if (file->data != NULL) {
            free(file->data);
        }
        file->data = (char*)malloc(sizeof(char) * 0);
        file->size = 0;
    }
    return 0;
}

// read data from a file
int myfuse_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    struct myfuse_file *file = myfuse_find_file(path);
    if (file == NULL) {
        return -ENOENT;
    }
    if (file->is_dir == 1) {
        return -EISDIR;
    }
    if(file==chat_client){
        char *buffer = (char*)malloc(sizeof(char) * (1024 + 1));
        printf("read from server\n");
        int r = server_rw(buffer, 1024, 0, serverfd);// 0 for read
        if(r==-1){}
        else{
            if(file->data==NULL){
                file->data = (char*)malloc(sizeof(char) * r);
            } else {
                file->data = (char*)realloc(file->data, sizeof(char) * r);
            }
            memcpy(file->data, buffer, r);
            file->size = r;
            free(buffer);
        }
    }
    if (offset >= file->size) {
        return 0;
    }
    if (offset + size > file->size) {
        size = file->size - offset;
    }
    memcpy(buf, file->data + offset, size);
    return size;
}

// write data to a file
int myfuse_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    struct myfuse_file *file = myfuse_find_file(path);
    if (file == NULL) {
        return -ENOENT;
    }
    if (file->is_dir == 1) {
        return -EISDIR;
    }
    if(file==chat_client){
        char *buffer = (char*)malloc(sizeof(char) * (1024 + 1));
        printf("read from server\n");
        int r = server_rw(buffer, 1024, 0, serverfd);// 0 for read
        if(r==-1){}
        else{
            if(file->data==NULL){
                file->data = (char*)malloc(sizeof(char) * r);
            } else {
                file->data = (char*)realloc(file->data, sizeof(char) * r);
            }
            memcpy(file->data, buffer, r);
            file->size = r;
            free(buffer);
        }
    }
    if (offset + size > MAX_FILE_SIZE) {
        return -EFBIG;
    }
    if (file->data == NULL) {
        file->data = (char*)malloc(sizeof(char) * (offset + size));
        memset(file->data, 0, sizeof(char) * (offset + size));
        file->size = offset + size;
    } else if (offset + size > file->size) {
        file->data = (char*)realloc(file->data, sizeof(char) * (offset + size));
        memset(file->data + file->size, 0, sizeof(char) * (offset + size - file->size));
        file->size = offset + size;
    }
    memcpy(file->data + offset, buf, size);
    if(file==chat_client){
        char *buffer = (char*)malloc(sizeof(char) * (1024 + 1));
        memcpy(buffer, file->data, file->size);
        buffer[file->size] = '\0';
        int w = server_rw(buffer, strlen(buffer), 1, serverfd);// 1 for write
        if(w==-1){
            printf("write to server error\n");
        }
        free(buffer);
    }
    return size;
}

// create a file
int myfuse_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    struct myfuse_file *file = myfuse_find_file(path);
    if (file != NULL) {
        return -EEXIST;
    }
    printf("set mod and own\n");
    int uid, gid;
    struct fuse_context *context = fuse_get_context();
    if(context){
        uid = context->uid;
        gid = context->gid;
    }
    // change own and mode here
    int ret = 0;
    struct stat stbuf;
    // 获取文件/目录的元数据信息
    ret = lstat(path, &stbuf);
    if (ret == -1) {
        printf("ret == -1\n");
        exit(1);
    }
    // 更新用户 ID 和组 ID 值
    stbuf.st_uid = uid;
    stbuf.st_gid = gid;
    // 更新权限位部分
    stbuf.st_mode &= ~0777; // 清除旧权限位
    stbuf.st_mode |= mode; // 设置新权限位
    // 将更新后的用户 ID 和组 ID 值写入文件/目录的元数据信息中
    ret = change_owner(file, stbuf.st_uid, stbuf.st_gid);
    if (ret == -1) {
        printf("ret == -1\n");
        exit(1);
    }
    // 将修改后的权限位写入文件/目录的元数据信息中
    ret = change_mode(file, stbuf.st_mode);
    if (ret == -1) {
        printf("ret == -1\n");
        exit(1);
    }
    char *path_copy = (char*)malloc(sizeof(char) * (strlen(path) + 1));
    strcpy(path_copy, path);
    char *token = strtok(path_copy, "/");
    struct myfuse_file *cur = root;
    while (token != NULL) {
        struct myfuse_file *child = cur->children;
        int found = 0;
        while (child != NULL) {
            if (strcmp(child->name, token) == 0) {
                cur = child;
                found = 1;
                break;
            }
            child = child->sibling;
        }
        if (found == 0) {
            break;
        }
        token = strtok(NULL, "/");
    }
    struct myfuse_file *new_file = (struct myfuse_file*)malloc(sizeof(struct myfuse_file));
    new_file->name = (char*)malloc(sizeof(char) * (strlen(token) + 1));
    strcpy(new_file->name, token);
    new_file->data = NULL;
    new_file->size = 0;
    new_file->children = NULL;
    new_file->parent = cur;
    new_file->sibling = NULL;
    new_file->is_dir = 0;
    if (cur->children == NULL) {
        cur->children = new_file;
    } else {
        new_file->sibling = cur->children;
        cur->children = new_file;
    }
    myfuse_file_num++;
    return 0;
}

// remove a file
int myfuse_unlink(const char *path) {
    struct myfuse_file *file = myfuse_find_file(path);
    if (file == NULL) {
        return -ENOENT;
    }
    if (file->is_dir == 1) {
        return -EISDIR;
    }
    if(file==chat_client){
        return -EBUSY;
    }
    if (file->parent->children == file) {
        file->parent->children = file->sibling;
    } else {
        struct myfuse_file *sibling = file->parent->children;
        while (sibling->sibling != file) {
            sibling = sibling->sibling;
        }
        sibling->sibling = file->sibling;
    }
    if(file->data != NULL)
        free(file->data);
    free(file->name);
    free(file);
    myfuse_file_num--;
    return 0;
}

// change file mode
int myfuse_chmod(const char *path, mode_t mode, struct fuse_file_info *fi) {
    struct myfuse_file *file = myfuse_find_file(path);
    if (file == NULL) {
        return -ENOENT;
    } else {
        // change mode here
        int ret = 0;
        struct stat stbuf;
        // 获取文件/目录的元数据信息
        ret = lstat(path, &stbuf);
        if (ret == -1) {
            printf("ret == -1\n");
        exit(1);
        }
        // 更新权限位部分
        stbuf.st_mode &= ~0777; // 清除旧权限位
        stbuf.st_mode |= mode; // 设置新权限位
        // 将修改后的权限位写入文件/目录的元数据信息中
        ret = change_mode(file, stbuf.st_mode);
        if (ret == -1) {
            printf("ret == -1\n");
        exit(1);
        }
    }
    return 0;
}

// change file owner
int myfuse_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi) {
    struct myfuse_file *file = myfuse_find_file(path);
    if (file == NULL) {
        return -ENOENT;
    } else {
        // change owner here
        int ret = 0;
        struct stat stbuf;
        // 获取文件/目录的元数据信息
        ret = lstat(path, &stbuf);
        if (ret == -1) {
            printf("ret == -1\n");
        exit(1);
        }
        // 更新用户 ID 和组 ID 值
        stbuf.st_uid = uid;
        stbuf.st_gid = gid;
        // 将更新后的用户 ID 和组 ID 值写入文件/目录的元数据信息中
        ret = change_owner(file, stbuf.st_uid, stbuf.st_gid);
        if (ret == -1) {
            printf("ret == -1\n");
        exit(1);
        }
    }
    return 0;
}

// file system operations
static struct fuse_operations myfuse_oper = {
    .init = myfuse_init,
    .getattr = myfuse_getattr,
    .readdir = myfuse_readdir,
    .mkdir = myfuse_mkdir,
    .rmdir = myfuse_rmdir,
    .rename = myfuse_rename,
    .truncate = myfuse_truncate,
    .chmod = myfuse_chmod,
    .chown = myfuse_chown,
    .open = myfuse_open,
    .read = myfuse_read,
    .write = myfuse_write,
    .create = myfuse_create,
    .unlink = myfuse_unlink,// equivalence of rm
};

// main function
int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &myfuse_oper, NULL);
}
