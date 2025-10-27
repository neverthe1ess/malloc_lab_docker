#include <stdint.h>
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


extern void *heap_listp;
extern void *pivot;

static inline int in_heap(void *p){
    return (char *) p >= (char *) mem_heap_lo() && (char *)p <= (char *) mem_heap_hi();
}

static inline int aligned(void *p){
    return ((uintptr_t) p % ) == 0;
}

static void print_block(void *bp){
    size_t h = GET(HDRP(bp));
    size_t f = GET(FTRP(bp));
    size_t size = GET_SIZE(HDRP(bp));
    int alloc = GET_ALLOC(HDRP(bp));
    printf("size = [%d], alloc = [%d], HDR = [%p], FTR = [%p]", GET_SIZE(bp), GET_ALLOC(bp), HDRP(bp), FTRP(bp));\
}

static void check_block(void *bp, int line){
    if(!aligned(bp)){
        printf("[line %d] ERROR: payload not aligned : %p\n", line, bp);
    }

    if(!in_heap(bp) || !in_heap(HDRP(bp)) || !in_heap(FTRP(bp))){
        printf("[line %d] ERROR: 블록이 힙 영역을 벗어낫습니다. %p\n", line, bp);
    }

    if(GET(HDRP(bp)) != GET(FTRP(bp))){
        printf("[line %d] ERROR: 헤더와 푸터가 일치하지 않습니다. %p\n", line, bp);
    }

    size_t size = GET_SIZE(HDRP(bp));
    int alloc = GET_ALLOC(HDRP(bp));

    if(size & ~0x7 != 0){
        printf("[line %d] ERROR: 블록의 크기가 8의 배수가 아닙니다. 사이즈 : %zu  주소: %p\n", line, size, bp);
    }

    if(size < 2 * DSIZE){
        printf("[line %d] ERROR: 블록의 크기가 최소 블록 크기보다 작습니다. 사이즈 : %zu, 주소 : %p\n", line, size, bp);
    }

    if(!(alloc == 0 || alloc == 1)){
        printf("[line %d] ERROR: 할당 비트가 유효하지 않습니다. %d %p\n", line, alloc, bp);
    }

    size_t mem_heapsize();



}


void mm_checkheap(int lineno){
    // 0)프롤로그 검사
    // heap_listp는 프롤로그 payload 시작
    // 프롤로그 헤더/푸터는 size = DSIZE, alloc = 1 이어야 함 
    if(!in_heap(heap_listp) || !aligned(heap_listp)){
        printf("[line %d] ERROR: bad heap_listp %p (in_heap = %d aligned = %d)\n",
        lineno, heap_listp, in_heap(heap_listp), aligned(heap_listp));
    }

    size_t pro_h = GET(HDRP(heap_listp));
    size_t pro_f = GET(FTRP(heap_listp));
    size_t pro_sz = GET_SIZE(HDRP(heap_listp));
    size_t pro_al = GET_ALLOC(HDRP(heap_listp));


    // 1) 본문 순회 : 각 블록 검사 + 연속 free 금지
    void *bp = heap_listp;
    size_t blk_cnt = 0, free_cnt = 0;

    for (; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        blk_cnt++;
        check_block(bp, lineno);
    
        int alloc = GET_ALLOC(HDRP(bp));
        if(!alloc){
            free_cnt++;
            void *next_block = NEXT_BLKP(bp);
            if(GET_SIZE(HDRP(next_block)) > 0 && !GET_ALLOC(HDRP(nb))){
                printf("연속된(병합되지 않은) 가용 블록이 존재합니다. (%p and %p)\n", lineno, bp, nb);
                print_block(bp);
                print_block(nb);
            }
        }
    }

    size_t epi_h = GET(HDRP(bp));
    size_t epi_sz = GET_SIZE(HDRP(bp));
    int epi_al = GET_ALLOC(HDRP(bp));
    if(epi_sz != 0 || epi_al != 1){
        printf("ERROR : 에필로그 헤더 오류 발생 ")
    }
    me


    






    // 3) rover(pivot) 간단 점검(선택)
    if (pivot){
        if(!in_heap(pivot) || !aligned(pivot)){
            printf("[line %d] 경고: 피벗이 유효하지 않음(힙 탈출 또는 비정렬): %p\n", lineno, pivot);
        } else if(GET_SIZE(HDRP(pivot)) == 0){
            printf("검색 기준점이 에필로그에 있습니다: %p\n", lineno, pivot);
        }

    }


}