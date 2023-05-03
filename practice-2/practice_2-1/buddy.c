#include "buddy.h"
#include "tool.h"
#include <math.h>
#include <stdlib.h>
#define NULL ((void *)0)
struct buddy_manager* b;
void init_node(struct block *root, int order, void *start){
    root->start = start;
    root->order = order;
    root ->next_in_freelist = NULL;
    root ->prev_in_freelist = NULL;
    root->status = FAKE;
    if(order == 0)return;// no left or right child to init
    root->left_child = (struct block *)malloc(sizeof(struct block));
    root->right_child = (struct block *)malloc(sizeof(struct block));
    root->right_child->parent = root;
    root->left_child->parent = root;
    root->left_child->buddy = root->right_child;
    root->right_child->buddy = root->left_child;
    init_node(root->left_child, order-1, start);
    init_node(root->right_child, order-1, start+(int)pow(2,order-1)*4096);
    return;
}
int init_page(void *p, int pgcount){
    //初始化 buddy 页面。你需要管理pgcount个 4K 页，这些 4K 页连续分配并且起始地址是p
    //阶数要可以从0到log(pgcount)+1，那么freelist的空间就应该是log(pgcount)+2
    //一共是pgcount个4k页面，一个页面需要4*1024个byte，所以需要分配出来pgcount*4096个byte 
    b = (struct buddy_manager *)malloc(sizeof(struct buddy_manager));
    b->start = p;
    b->pgcount = pgcount;
    b->maxorder = (int)log2(pgcount);
    b->freelist = (struct free_head_node *)malloc(sizeof(struct free_head_node)*(b->maxorder+1));
    for(int i = 0; i < b->maxorder+1; i++){
        b->freelist[i].order = i;
        b->freelist[i].next = NULL;
        b->freelist[i].block_num = 0;
    }
    b->root_block = (struct block *)malloc(sizeof(struct block));
    init_node(b->root_block, b->maxorder, p);
    b->root_block->parent = NULL;
    b->root_block->buddy = NULL;
    b->root_block->status = FREE;
    b->freelist[b->maxorder].next = b->root_block;
    b->freelist[b->maxorder].block_num = 1;
    // printf("the start address is %lld\n",(long long)b->start);
    // printf("the page count is %d\n",b->pgcount);
    // printf("so the end address is %lld\n",(long long)(b->start+b->pgcount*4096));
    return OK;
}
// void look_inside_tree(struct buddy_manager *b){
//     printf("============================\n");
//     struct block *node = b->root_block;
//     while(node!=NULL){
//         printf("the order is %d\n",node->order);
//         printf("the start address is %lld\n",(long long)node->start);
//         if(node->buddy!=NULL)printf("the buddy address and its address sub result is %lld\n",(long long)node->buddy->start-(long long)node->start);
//         node = node->left_child;
//     }
//     printf("============================\n");
// }
void *alloc_pages(int rank){
    rank--;
    if(rank>b->maxorder||rank<0)return -EINVAL;
    // look_inside_tree(b);
    //分配rank阶的页面，如果没有足够的页面，返回NULL
    int i;
    for(i = rank;i<=b->maxorder;++i){
        if(b->freelist[i].next != NULL)break;
    }
    if(i>b->maxorder){
        //printf("no enough pages\n");
        return -ENOSPC;//没有足够的页面
    }
    //得到应该去rank=i里面拿节点
    //首先把这个节点从freelist里面删掉
    struct block* bl = pop_head_freelist(b,i);// 这个节点拿出来了并且把它变成了FAKE
    //把这个节点分裂成rank阶的节点，期间总是把一半拿出去变成free
    while(bl->order!=rank)bl = split_and_put_one_free(b,bl);
    use_block(b,bl);
    return bl->start;    
}
int return_pages(void *p){
    // printf("the address is %lld\n",(long long)p);
    //释放p指向的页面，如果p不是alloc_pages返回的地址，返回EINVAL
    struct block *bl = find_block_in_buddy(b,p);
    if(bl == NULL||bl->status == FREE) return -EINVAL;
    //merge the blocks
    return_and_merge(b,bl);
    return OK;
}
int query_ranks(void *p){
    struct block *bl = find_block_in_buddy(b,p);
    if(bl == NULL) return -EINVAL;
    else return bl->order+1;
}
int query_page_counts(int rank){
    rank--;
    if(rank>b->maxorder||rank<0)return -EINVAL;
    return b->freelist[rank].block_num;
}