/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */
#define DSIZE       8       /* Doubleword size (bytes) */

#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */
// 자주 조금씩 요청하는 것보다, 한 번에 크게 요청하는 게 훨씬 효율적

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))
// alloc -> allocated bit
// PACK(16, 0) = 0x10 | 0x0 = 0x10
// PACK(16, 1) = 0x10 | 0x1 = 0x11

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))
// GET(p): 주소 p의 값을 읽음
// PUT(p, val): 주소 p에 val을 씀

/* Read the size and allocated fields from address p */

#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)


/* Given block ptr bp, compute address of its header and footer */

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Krafton Jungle",
    /* First member's full name */
    "Jiwon Yoon",
    /* First member's email address */
    "include.yoonio.h@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static char* heap_start_pos;
static char* epilogue_header_pos;

void coalesce_with_next(char *bp, char *next_bp){
    size_t size = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(next_bp));
    
    int new_header = PACK(size, 0);
    PUT(HDRP(bp), new_header);

    int new_footer = PACK(size, 0);
    PUT(FTRP(next_bp), new_footer);

    return;
}

void coalesce_with_prev(char *prev_bp, char *bp){
    size_t size = GET_SIZE(HDRP(prev_bp)) + GET_SIZE(HDRP(bp));
        
    int new_header = PACK(size, 0);
    PUT(HDRP(prev_bp), new_header);

    int new_footer = PACK(size, 0);
    PUT(FTRP(bp), new_footer);

    return;
}

void coalesce_with_both(char *prev_bp, char *bp, char *next_bp){
    size_t size = GET_SIZE(HDRP(prev_bp)) +
                GET_SIZE(HDRP(bp)) +
                GET_SIZE(HDRP(next_bp));

    PUT(HDRP(prev_bp), PACK(size, 0));
    PUT(FTRP(next_bp), PACK(size, 0));

    return;
}

void coalesce(char *bp) {
    char* prev_bp = PREV_BLKP(bp);
    char* next_bp = NEXT_BLKP(bp);

    int prev_alloc = GET_ALLOC(FTRP(prev_bp));
    int next_alloc = GET_ALLOC(HDRP(next_bp));

    // [alloc][FREE][FREE]
    if (prev_alloc && !next_alloc)
        coalesce_with_next(bp, next_bp);

    // [FREE][FREE][alloc]
    if (!prev_alloc && next_alloc){
        coalesce_with_prev(prev_bp, bp);
    }

    // [FREE][FREE][FREE]
    // coalesce_case1(bp, next_bp);  // After [FREE][Marged FREE]
    // coalesce_case2(prev_bp, bp);  // After [Marged FREE]
    if (!prev_alloc && !next_alloc){
        coalesce_with_both(prev_bp, bp, next_bp);
    }

    return;
}

/// @return return 1 on success, 0 on error
int extend_heap(char* start_addr, size_t inputed_size) {

    // decide size
    size_t extend_size = MAX(inputed_size, CHUNKSIZE);
    if (extend_size == inputed_size)
        extend_size += CHUNKSIZE;

    // sbrk
    char* result = mem_sbrk(extend_size);

    // except
    if (result == (void *)-1)
        return 0;

    // update epilogue
    epilogue_header_pos = result + extend_size;
    PUT(epilogue_header_pos, PACK(0, 1));

    // Packing header & footer
    int header = PACK(extend_size, 0);
    int footer = header;

    // Put
    PUT(start_addr, header);
    PUT((start_addr + extend_size) - WSIZE, footer);

    coalesce(start_addr + WSIZE);

    return 1;
}

/*
 * mm_init - initialize the malloc package.
 */

// Returns: 0 on success, -1 on error
int mm_init(void)
{ 
    heap_start_pos = mem_sbrk(WSIZE * 4); // padding PH, PF, EH

    if (heap_start_pos == (void *) -1) // except
        return -1;

    // Get Prologue & Epilogue data
    int init_prologue_data = PACK(DSIZE, 1);
    int init_epilogue_data = PACK(0, 1);

    // Prologue
    PUT(heap_start_pos, init_prologue_data);                   // Padding
    PUT(heap_start_pos + (WSIZE * 1), init_prologue_data);     // Header
    PUT(heap_start_pos + (WSIZE * 2), init_prologue_data);     // Footer

    heap_start_pos += DSIZE;

    // Epilogue Header
    PUT(heap_start_pos + (WSIZE * 2), init_epilogue_data);     

    // extend_heap(epilogue_header_pos, CHUNKSIZE);
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment. (8)
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*--
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}