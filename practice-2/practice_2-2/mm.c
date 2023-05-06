/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  Blocks are never coalesced or reused.  The size of
 * a block is found at the first aligned word before the block (we need
 * it for realloc).
 *
 * This code is correct and blazingly fast, but very bad usage-wise since
 * it never frees anything.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif


/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define SIZE_PTR(p)  ((size_t*)(((char*)(p)) - SIZE_T_SIZE))
// 首先规定startp指向的是heap的第一个位置，endp指向的是heap的最后一个位置
// heap的前四个字节是为了让block pointer 8-aligned的paddinng，所以startp+4才是第一个block的header
// last_header_p指向的是最后一个block的header，目的是为了让extend的时候可以比较快速的知道上一个block的header
// free_header指向的是第一个free block的header，目的是为了让malloc的时候可以比较快速的找到第一个free block
// free list采用的是last in first out 的策略，所以每次一个新的freeblock会被我放到free_header的前面成为新的header
// 然后free_list 采用的是first fit策略，推迟合并，合并的时候看footer的prev_status，如果是1就合并，如果是0就不合并
// 也就是freelist是乱序的，只是用来找free block，但是header-footer相连是地址顺序的，用来合并
// header的内容 block size + prev_status + status，footer 就是header的复制, 其中blocksize是4的倍数，所以占了30位，后面各占一位
// succ 指向的是下个free 块的header，便于快速查看block size，
// 并且为了压缩最小块的限制，succ存为与startp的差值，4byte
#define WSIZE 4
#define SMALLEST_BLOCK 8
#define FREE 0
#define USED 0
#define PREV_FREE 0
#define PREV_USED 2
// succ + footer
// 4+4 = 8
// succ = real succ - startp
// but by putting header and footer together we can manage 8-aligned
// we always hope to allocate blocks of size 8*k
void *last_header_p;
void *startp;// the beginning of the heap, 4-bytes before thr first block's header
void *endp;// the end of the heap. exactly after the last block's footer
void *free_header;//points to the first free block, bp

unsigned int get_word (void *p){
  return *(unsigned int *)p;
}
void put_word (void *p, unsigned int val){
  *(unsigned int *)p = val;
}
void *get_header_pointer(void *block_p){
  return block_p - WSIZE;
}
unsigned int get_size(void *head_p){
  return get_word(head_p)&~0x3;
}
unsigned int get_status(void *head_p){
  // may be 0 or 1
  return get_word(head_p)&0x1;
}
unsigned int get_prev_status(void *head_p){
  // may be 0 or 2
  return get_word(head_p)&0x2;
}
void *get_footer_pointer(void *head_p){
  return head_p + get_size(head_p);
}
void *get_real_succ(void *head_p){
  return get_word(head_p+WSIZE)+startp;
}
void *get_next_header(void *head_p){
  return head_p + 4 + get_size(head_p);
}
unsigned int get_suitable_size(unsigned int size){
  // the block size should be 8*k
  if(size < 8) return 8;
  else if(size%8==0) return size;
  else return (size/8+1)*8;
}
unsigned int pack_header(unsigned int block_size,unsigned int prev_status, unsigned int status){
  // 30 + 1 + 1
  // block size + 2/0 + 1/0
  return (((block_size)|prev_status)|status);
}

/*
 * mm_init - Called when a new trace starts.
 */
int mm_init(void)
{
  // 4 bytes padding before the first block's header
  startp = mem_sbrk(4);//head_listp points to the start of the heap
  endp = startp + 4;//endp points to the end of the heap
  last_header_p = NULL;// always points to the last block's header
  free_header = NULL;
  return 0;
}

void *extend_heap(unsigned int block_size){
  unsigned int prev_status;
  if(last_header_p==NULL) prev_status = PREV_USED;//this is the first block, so the prev should be busy, that is PREV_USED
  else {prev_status=(get_status(last_header_p)==FREE?PREV_FREE:PREV_USED);}
  last_header_p = mem_sbrk(block_size+WSIZE);// this is the new block's header
  endp = last_header_p + block_size + WSIZE;// this is the end of the heap
  unsigned int new_header = pack_header(block_size,prev_status,USED);
  put_word(last_header_p, new_header);
  return last_header_p+WSIZE;
}

void *placement(unsigned int block_size){
    void *header = free_header;
    void *before_header = NULL;// 没记下来pred，用beforehead知道前一个
    while(header != NULL){
      if(get_size(header)>=block_size)break;
      else {
        before_header = header;
        header = get_real_succ(header);
      }
    }
    if(header == NULL){
      // todo coalesce and check again
      return extend_heap(block_size);
    }else{
      //用这块，并且记得把它从freelist里面删掉，并且更新它地址顺序后面那块的prev-status
      if(before_header==NULL)free_header = get_real_succ(header);
      else {
        unsigned int relative_succ =(unsigned int)(get_real_succ(header)-startp);
        put_word((void *)(before_header+4),relative_succ);
      }
      void *next_head_p = get_next_header(header);
        if(next_head_p<endp){
          unsigned int renew = pack_header(get_size(next_head_p),PREV_USED,get_status(next_head_p));
          put_word(next_head_p,renew);
        }
        // todo check split
        return header+WSIZE;
    }
}

void *malloc(size_t size)
{
  // int newsize = ALIGN(size + SIZE_T_SIZE);
  // unsigned char *p = mem_sbrk(newsize);
  // //dbg_printf("malloc %u => %p\n", size, p);

  // if ((long)p < 0)
  //   return NULL;
  // else {
  //   p += SIZE_T_SIZE;
  //   *SIZE_PTR(p) = size;
  //   return p;
  // }

  unsigned int block_size = get_suitable_size(size);
  return placement(block_size);
}

void free(void *ptr){
	// put it into the free list 
  // change this header info
  // and change the prev status of the next header
  if(ptr==NULL)return;
  void *this_header = ptr - WSIZE;
  put_word(ptr,free_header-startp);
  unsigned int header_info = pack_header(get_size(this_header),get_prev_status(this_header),FREE);
  put_word(this_header,header_info);
  void *footer = get_footer_pointer(this_header);
  put_word(footer,header_info);
  void *next_head_p = get_next_header(this_header);
  if(next_head_p<endp){
    unsigned int renew = pack_header(get_size(next_head_p),PREV_FREE,get_status(next_head_p));
    put_word(next_head_p,renew);
  }
}

/*
 * realloc - Change the size of the block by mallocing a new block,
 *      copying its data, and freeing the old block.  I'm too lazy
 *      to do better.
 */
void *realloc(void *oldptr, size_t size)
{
  size_t oldsize;
  void *newptr;
  /* If size == 0 then this is just free, and we return NULL. */
  if(size == 0) {
    free(oldptr);
    return 0;
  }
  /* If oldptr is NULL, then this is just malloc. */
  if(oldptr == NULL) {
    return malloc(size);
  }
  newptr = malloc(size);
  /* If realloc() fails the original block is left untouched  */
  if(!newptr) {
    return 0;
  }
  /* Copy the old data. */
  oldsize = get_size(oldptr-WSIZE);
  if(size < oldsize) oldsize = size;
  memcpy(newptr, oldptr, oldsize);
  /* Free the old block. */
  free(oldptr);
  return newptr;
}

/*
 * calloc - Allocate the block and set it to zero.
 */
void *calloc (size_t nmemb, size_t size)
{
  size_t bytes = nmemb * size;
  void *newptr;
  newptr = malloc(bytes);
  memset(newptr, 0, bytes);
  return newptr;
}

/*
 * mm_checkheap - There are no bugs in my code, so I don't need to check,
 *      so nah!
 */
void mm_checkheap(int verbose){
	/*Get gcc to be quiet. */
	verbose = verbose;
}