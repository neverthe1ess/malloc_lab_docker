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
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

#define MAX(x, y) ((x) > (y)? (x) : (y))
/* 크기와 할당 비트 통합 */
#define PACK(size, alloc) ((size) | (alloc))
/* 인자 p가 참조하는 워드를 읽거나 써서 리턴 */
#define GET(p) (*(unsigned int *) (p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* 헤더 또는 푸터의 사이즈와 할당 비트를 리턴한다. */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* 각각 블록 헤더와 풋터를 가리키는 포인터를 리턴한다. */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* 다음과 이전 블록의 블록 포인터를 각각 리턴한다. */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ohoh team",
    /* First member's full name */
    "neverthe1ess",
    /* First member's email address */
    "gmldnjs117820@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static char *heap_listp = NULL;

/*
 * extend_heap 함수 - 힙이 초기화 될 때와 mm_malloc이 적당한 fit을 찾지 못했을 때 호출
 */


/* 주변에 빈 블럭이 생겨날 가능성이 있으면 새로 생긴 블록과 병합 */
static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    /* case 1: 이전과 다음 블록이 모두 할당 상태일 때 */
    if (prev_alloc && next_alloc){
        return bp;
    } 

    /* case 2: 이전은 할당, 다음 블록은 가용 상태일 때 */
    else if(prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        // 다음 블록의 푸터 위치(트릭)
        PUT(FTRP(bp), PACK(size, 0));
    }

    /* case 3: 이전은 가용 상태, 다음 블록은 할당 상태일 때 */
    else if(!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp); // 이전 블록이 시작 주소임
    }

    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    /* 크기 결정 : 요청받은 크기를 8바이트 정렬에 맞게 조절 */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1){
        return NULL;
    }

    /* 새로 얻은 가용 블록 초기화 */
    PUT(HDRP(bp), PACK(size, 0)); // Header, 크기와 할당 상태 == 'free'
    PUT(FTRP(bp), PACK(size, 0)); // footer, 헤더와 동일한 정보
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // 새로운 에필로그 헤더 설정
    
    /* 이전 블록이 가용 상태라면, 병합 */
    return coalesce(bp);
}


/*
 * mm_init - 초기 가용 리스트 만들기(힙 초기화)
 */ 
int mm_init(void)
{   
    /* 힙의 기본 구조(패딩 / 프롤로그 헤더 / 프롤로그 푸터 / 에필로그 헤더)를 위한 메모리 확보*/
    if((heap_listp = mem_sbrk(4 * WSIZE)) == (void *) - 1){
        return -1;
    }

    PUT(heap_listp, 0); // Alignment Padding
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); // prologue header
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); // prologue footer
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));// epilogue header
    
    // 실제 블록(페이로드)은 프롤로그 블록 바로 뒤
    heap_listp += (2 * WSIZE);
    
    /* 힙을 확장하여 초기 가용 공간 확보 4KB */
    if(extend_heap(CHUNKSIZE/WSIZE) == NULL){
        return -1;
    }
    return 0;
}

static void *find_fit(size_t asize){
    void *bp;

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            return bp;
        }
    }
    return NULL;
}

static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));

    /* 만약 남는 공간이 최소 블록 크기(16바이트) 이상이면 분할 */
    if ((csize - asize) >= (2 * DSIZE)){
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}


/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 *     사용자가 요청한 size를 담을 수 있는 가용 블록을 힙에서 찾아 할당하고,
 *     그 볼록의 주소를 반환하는 것이다. 
 */
void *mm_malloc(size_t size)
{
    /* 1. 실제 할당할 블록 크기를 계산*/
    size_t asize; 
    size_t extendsize;
    char *bp;

    /* 0바이트 요청은 무시 */
    if(size == 0){
        return NULL;
    }

    if(size <= DSIZE){
        asize = 2 * DSIZE; // 최소 블록 크기는 16바이트
    } else {
        // 8바이트의 오버헤드(헤더/푸터)를 더하고, 8의 배수로 올림
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }

    /* 2. 가용 블록 검색(first-fit)*/
    if((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }

    /* 3. 적합한 블록이 없으면 힙 확장 */
    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize / WSIZE)) == NULL){
        return NULL;
    }

    /* 4. 새로운 확장된 공간에 블록 할당*/
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
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