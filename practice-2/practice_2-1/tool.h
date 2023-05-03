#ifndef TOOL_H
#define TOOL_H
#define NULL 0
#define FREE 1
#define USED 2
#define FAKE 3

//吊在每一阶free_list上的节点
// 同时也是在地址树上面的节点
struct block{
    //起始地址
    void *start;
    //还有这个节点的阶数，表示的是这个节点中页的大小，可以用来计算结束地址
    int order;
    //还有一个指向下一个节点的指针
    struct block *next_in_freelist;
    struct block *prev_in_freelist;
    struct block *left_child;
    struct block *right_child;
    struct block *parent;
    struct block *buddy;
    int status;//表示在树上的节点状态，是free还是used还是fake
    //free表示的是在freelist里面并且可以分配，used表示exactly可以使用，fake表示的只是在树上当一个假的节点并不能用来分配或者归还
};
struct free_head_node{
    int order;//表示这一个表头的阶数
    int block_num;
    struct block *next;//指向这个表头的第一个节点,一开始会把它初始化成NULL
};
struct buddy_manager{
    struct free_head_node *freelist;//指向一个freelist的指针
    void *start;//指向这个buddy管理的内存的起始地址
    int pgcount;//表示这个buddy管理的内存的页数
    int maxorder;//表示这个buddy管理的内存的最大阶数，也就是freelist的最大阶数，log(pgcount)+1
    struct block *root_block;
};

void add_to_freelist(struct buddy_manager *b, struct block *block_to_add){
    //首先要找到这个block的阶数
    int order = block_to_add->order;
    //然后把这个block加到这个阶数的freelist的最前面
    block_to_add->next_in_freelist = b->freelist[order].next;
    block_to_add->prev_in_freelist = NULL;
    if(b->freelist[order].next != NULL){
        b->freelist[order].next->prev_in_freelist = block_to_add;
    }
    b->freelist[order].next = block_to_add;
    //然后应该把这个block的status改成free
    block_to_add->status = FREE;
    b->freelist[order].block_num++;
    return;
}

void delete_from_freelist(struct buddy_manager *b, struct block* block_to_delete){
    // 首先要找到这个block的阶数
    int order = block_to_delete->order;
    //然后把这个block从这个阶数的freelist里面拿出来
    if(block_to_delete==b->freelist[order].next){
        b->freelist[order].next = block_to_delete->next_in_freelist;
    } else {
        block_to_delete->prev_in_freelist->next_in_freelist = block_to_delete->next_in_freelist;
        if(block_to_delete->next_in_freelist != NULL){
            block_to_delete->next_in_freelist->prev_in_freelist = block_to_delete->prev_in_freelist;
        }
    }
    block_to_delete->next_in_freelist = NULL;
    block_to_delete->prev_in_freelist = NULL;
    block_to_delete->status = FAKE;
    b->freelist[order].block_num--;
}

struct block *pop_head_freelist(struct buddy_manager *b,int rank){
    //首先要找到这个rank的freelist的头节点
    struct block *head = b->freelist[rank].next;
    //然后把这个头节点从freelist里面拿出来
    if(head != NULL){
        b->freelist[rank].next = head->next_in_freelist;
        if(head->next_in_freelist != NULL){
            head->next_in_freelist->prev_in_freelist = NULL;
        }
        head->next_in_freelist = NULL;
        head->prev_in_freelist = NULL;
        head->status = FAKE;
    }
    b->freelist[rank].block_num--;
    return head;
}

struct block* split_and_put_one_free(struct buddy_manager *b, struct block* to_split){
    to_split->status = FAKE;
    // 然后把它的左孩子返回出去，右孩子记成free
    add_to_freelist(b,to_split->right_child);
    return to_split->left_child;
}

void use_block(struct buddy_manager *b, struct block *to_use){
    to_use->status = USED;
}

struct block *find_block_on_tree(struct block *root, void *p){
    if(root->start == p&&root->status != FAKE){
        return root;
    }else {
        // printf("root->start:%lld\n",(long long)root->start);
        // printf("p:%lld\n",(long long)p);
        if(root->left_child==NULL)return NULL;
        else if(p<root->right_child->start)return find_block_on_tree(root->left_child,p);
        else return find_block_on_tree(root->right_child,p);
    }
}

struct block *find_block_in_buddy(struct buddy_manager *b, void *p){
    if((p-b->start)%4096!=0)return NULL;
    return find_block_on_tree(b->root_block,p);
}

void return_and_merge(struct buddy_manager *b, struct block * bl){
    bl->status = FREE;
    while(bl->buddy->status == FREE){
        bl->status = FAKE;
        bl->buddy->status = FAKE;
        delete_from_freelist(b,bl->buddy);
        bl->parent->status = FREE;
        bl = bl->parent;
        if(bl->buddy == NULL)break;
    }
    add_to_freelist(b,bl);
    return;
}
#endif