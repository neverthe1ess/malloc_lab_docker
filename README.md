# CSAPP MALLOC LAB(동적 메모리 할당기 구현)

C언어 표준 라이브러리에서 제공하는 `malloc` , `free`, `realloc`을 직접 구현하는 과제이다.


## 1. 목표
> 자신만의 `malloc`, `realloc`. `free`를 구현해보자!
>  

## 2. 구현 규칙
- `mm.c` 만 수정하여 테스트를 통과하여야 한다.
- `make clean` > `make` > `./mdriver -V` 과정을 통해 throughput/memory utilzation을 검증한다. 

---
## 3. 구현할 가용 리스트 전략

- 묵시적 가용 리스트(implicit free list)
- 명시적 가용 리스트(explicit free list)
- 분리 가용 리스트(Segregated free list)


---

## 4. 동적 메모리 할당(Dynamic Memory Allocation)

> 프로그래머는 **동적 메모리 할당기**를 통해 **런타임**에 추가적인 가상 메모리를 흭득한다.
>

💡 동적 메모리 할당기는 **힙**이라고 불리는 하나의 프로세스 가상 메모리 영역을 관리한다.



### ✅ 동적 메모리 할당기의 특징

- 할당기는 힙을 **가변 크기의 블록**의 모음으로 관리한다.
- 가변 크기의 블록의 상태를 **Allocated** 와 **Free** 두 가지 상태로 관리한다.

### 📌 동적 메모리 할당기의 유형
- **Explicit Allocator(명시적 할당기)**
   > Application(즉, 프로그래머)이 할당과 해제(반환)의 책임을 가지고 있다.
   >
   예시 : C언어에서의 malloc() 와 free() 

- **implicit Allocator(암시적 할당기)**
   > Application이 할당의 책임을 가지나, 해제의 책임을 가지지 않는다.
   > 
   예시 : garbage collection을 지원하는 JAVA, PYTHON, ML 등 

---

## 5. 동적 메모리 할당기의 제약조건(Constraints)

- **할당기를 사용하는 프로그램 입장**
   - 멋대로 malloc()과 free()를 호출할 수 있다.(임의의 순서)
   - free() 함수는 반드시 malloc()으로 할당된 블록에 한하여 사용

- **동적 메모리 할당기 입장**
   - **할당된 블록**의 개수와 사이즈를 조작해서는 안된다.
   - malloc() 함수 호출에 즉시 반응하여야 한다.(순서 변경 및 요청 버퍼 사용 금지)
   - 반드시 가용 메모리에 속하는 메모리만 제공하여야 한다.
   - 모든 데이터 타입을 호환하기 위해 **8 ~ 16바이트 정렬 요건**을 준수하여야 한다.

---


## 6. 동적 메모리 할당기의 평가지표(Performance Goal)

> **처리율의 극대화 및 메모리 이용률 최대화**
- 그러나 이 두가지 지표는 Trade-off 관계이다.

- **처리량(Throughput)**
   - 단위 시간 당 malloc()과 free()가 처리가 완료된 횟수
   - 5000 개의 free() 호출, 5000개의 malloc() 호출을 10초로 처리하였다면 `1,000 operations/sec` 라 한다.

- **메모리 이용률 최대화(Peak Memory Utilization)**
   - `모든 페이로드(malloc()으로 요청한 사이즈)의 합계 / 힙 사이즈`
   - 내부 단편화와 외부 단편화에 깊이 연관되어 있다.


## 7. 내부 단편화와 외부 단편화(Fragmentation)
### 📌 단편화의 유형
- **Internal Fragmentation(내부 단편화)**
   > 블록을 할당하였을 때, 요청한 사이즈(payload)보다 블록 사이즈가 더 커서 사용되지 않는 hole이 발생하는 것을 말한다.
   >
   - 힙 자료구조 관리, 정렬 목적의 패딩, 명시적 정책 결정에 의해 발생한다.
   - 이전 요청에 의존하는 경향이 있다. 즉, 이전 요청 조사를 통해 내부 단편화를 예측할 수 있다. 

- **External Fragmentation(외부 단편화)**
   > 실제로 총 가용 블록의 합계 크기는 충분하나, 연속된 하나의 블록의 크기보다 더 큰 블록 요청이 들어올 때 발생하는 것을 말한다.
   > 
   - 전술된 내부 단편화에 비해서 미래의 요청에 영향을 크게 받아 예측하기 매우 어렵다.
   - ex. 4KB 가용 블록 2개 -> 실제 요청 5KB 1개 (할당 불가)
   - 그렇기 떄문에, 최대한 적은 개수의 큰 블록을 둘 수 있도록 해야 한다.

## 9. 구현할 가용 리스트 전략
> 각 전략은 가용 블록(Free blocks)을 어떻게 추적하고 관리해야 할지에 따라 구분되어 있다.
>

- 묵시적 가용 리스트(implicit free list) 
- 명시적 가용 리스트(explicit free list)
- 분리 가용 리스트(Segregated free list)

## 10. 묵시적 가용 리스트 Implicit Free Lists
> 모든 블록에 사이즈와 할당 여부를 기록하는 방법이며, 각 블록 경계를 헤더로 구분한다.
> 

- **사이즈와 할당 여부를 기록하는 방법**

   블록들은 어차피 8배수로 정렬되어 있기 때문에, 하위 주소의 3비트는 무조건 0이다. 묵시적 가용 리스트에서는 블록 **맨 앞에 워드 하나를 헤더**로 사용한다. 헤더에는 **사이즈와 할당 여부**를 같이 기록한다. 

   하나의 워드에 같이 기록하려면 사이즈에서 항상 3비트가 0으로 저장되어 있기 때문에, 이 곳으로 할당/해제 비트로 사용한다. 그렇기 때문에 사이즈를 읽어야 할 때, 하위 3비트는 비트 마스크 연산이 필요하다.  

- **가용 블록을 찾는 방법**

   - **First Fit**
   
      ```
      p = 블록의 시작 주소(페이로드)
      while(p가 끝이 아니면서 p가 할당되어 있으면서 p가 원하는 블록의 크기보다 작을 때면 반복해줘){
         p = 다음 블록의 시작 포인터
      }  
      ```
      - 블록의 총 개수에 탐색시간이 비례한다.(할당여부와 관계없이)
      - 뒤로 갈수록 개수가 작고 큰 블록이 많아진다. 
   - **Next Fit**
      - first-fit과 유사하지만, 과거 탐색을 끝난 위치에서 탐색을 시작한다.
      - 할당된 블록을 다시 스캔할 필요가 없어, first-fit에 비해 탐색속도가 빠르다.
      - 그러나, utilization 관점(단편화)에서는 first fit보다 나쁘다는 연구 결과가 있다.

   - **Best Fit**
      - 리스트 전체를 탐색하여, 요청하는 페이로드 크기와 가장 딱 맞는 블록을 찾는다.
      - 딱 맞는 블록을 선택하기 때문에, 단편화가 줄어들지만 탐색비용이 first-fit보다 크다.

- **내부 단편화를 막기 위한 분할 기법**
   - 할당된 공간(페이로드)이 가용된 공간보다 작다면 블록을 분할 할 수 있다.  
      
      ```c
      void addblock(ptr p, int len) {
         int newsize = ((len + 1) >> 1) << 1;  // round up to even
         int oldsize = *p & -2;                // mask out low bit
         *p = newsize | 1;                     // set new length
         if (newsize < oldsize)
         *(p+newsize) = oldsize - newsize;     // set length in remaining
      }                                        // part of block
   
     ```

- **할당 해제 구현과 해제에서 발생하는 오류 단편화**
   - 단순하게 구현한다면, 할당 비트를 0으로 `Clear` 하면 된다.
   - 그러나 이것은 해제된 블록과 기존 가용 블록 간 단편화가 발생할 수 있다. 
   - 오류 단편화는 실제 가용 블록의 크기는 충분한데, 서로 경계 헤더로 분리되어 있어 하나의 연속된 블록으로 할당할 수 없는 현상을 말한다. 
   - 이를 해결하기 위해, 인접한 두 가용 블록 간 병합(coalesce)이 필요하다.

- **오류 단편화를 막기 위한 병합 기법**
   - 이전 블록과 다음 블록이 인접하면서, 가용 상태라면 블록 간 조인을 수행한다. 
      ```c
      void free_block(ptr p) {    
         *p = *p & -2;           // clear allocated flag
         next = p + *p;          // find next block
         if ((*next & 1) == 0)      
         *p = *p + *next;        // add to this block if
      }                          //    not allocated
      
      ```
   - 그러나 이전 블록의 태그의 사이즈와 할당 여부를 알 수 없어 처음부터 다시 순회해야 한다.

- **이전 태그의 사이즈와 할당 여부를 알기 위한 경계 태그 기법**
   - 블록 헤더의 값(사이즈 / 할당 여부 비트)워드를 그대로 페이로드 뒤, 즉 블록의 끝에 그대로 복사한다.
   - 이전 블록의 데이터를 찾기 위해, 처음부터 다시 순회할 필요가 없다.

- **상수 시간 병합(Constant Time Coalescing)**
   - 전술된 경계 태그 기법으로 이전 태그의 할당 여부를 상숙시간에 알 수 있게 되었다. 
   - 즉, 상수 시간 내 인접 블록을 병합 할 수 있다.
   - 로직은 헤더, 푸터의 사이즈를 케이스에 따라 갱신하면 된다. 
   - `케이스 1` : 이전, 다음 블록 모두 할당되어 있음(Allocated, Free, Allocated) -> 유지
   - `케이스 2` : 이전 블록은 할당, 다음 블록은 가용(Allocated, Free, Free) -> 현재 블록의 헤더와 다음의 블록의 푸터에 통합된 사이즈 반영
   - `케이스 3` : 이전 블록은 가용, 다음 블록은 할당(Free, Free, Allocated) -> 이전 블록의 헤더와 현재 블록의 푸터에 통합된 사이즈 반영
   - `케이스 4` : 이전, 다음 블록 모두 가용(Free, Free, Free) -> 이전 블록의 헤더와 다음 블록의 푸터에 통합된 사이즈 반영
   

## 11. 할당기 정책의 핵심 요약 KeyAllocator Policies 
- **배치 정책(가용 블록 선택)**
   - First-fit, next-fit, best-fit 등이 있다.
   - 처리량과 메모리 효율에 상충 관계를 잘 고려해야 한다.
   - `흥미로운 점`: Segregated Free list는 전체 리스트를 순회하지 않아도 best-fit에 수렴한다.

- **분할 정책**
   - 전술된 경계 태그 기법으로 이전 태그의 할당 여부를 상숙시간에 알 수 있게 되었다. 
   - 즉, 상수 시간 내 인접 블록을 병합 할 수 있다.
   - 로직은 헤더, 푸터의 사이즈를 케이스에 따라 갱신하면 된다. 
   - `케이스 1` : 이전, 다음 블록 모두 할당되어 있음(Allocated, Free, Allocated) -> 유지
   - `케이스 2` : 이전 블록은 할당, 다음 블록은 가용(Allocated, Free, Free) -> 현재 블록의 헤더와 다음의 블록의 푸터에 통합된 사이즈 반영
   - `케이스 3` : 이전 블록은 가용, 다음 블록은 할당(Free, Free, Allocated) -> 이전 블록의 헤더와 현재 블록의 푸터에 통합된 사이즈 반영
   - `케이스 4` : 이전, 다음 블록 모두 가용(Free, Free, Free) -> 이전 블록의 헤더와 다음 블록의 푸터에 통합된 사이즈 반영

## 12. 명시적 가용 리스트 Explicit Free Lists
> 가용 상태인 블록만 추적하면 안될까? >> 명시적 가용 리스트는 가용 블록끼리 이중 연결 리스트로 연결하여 관리한다. 
- 가용 상태인 블록은 payload가 비어 있다. 즉, 데이터가 필요가 없다.
- 이 때 이 payload에 이전 가용 블록의 포인터와 다음 가용 블록의 주소를 같이 관리하면?
- 모든 블록을 굳이 다 순회할 필요가 사라진다.

💡 하나의 이중 연결 리스트를 만들어 가용 블록들을 연결한다.

   - **삽입 정책(Insertion Policy)**
      > 새로운 블록을 가용 리스트에 삽입하려면 어떻게 해야 할까? 
      >

      **LIFO(last-in-first-out) Policy**
      - 삽입할 블록을 무조건 리스트에 처음에 삽입한다.
      - 장점 : 구현이 간단하고 상수 시간에 처리 가능하다.
      - 단점 : Address-Ordered 방식에 비해 단편화 관점에서 좋지 않다는 연구 결과가 있다.

      **Address-Ordered Policy**
      - 삽입할 가용 블록을 주소 순서 대로 삽입한다.
      - 장점 : 삽입을 위해 리스트 탐색이 필요하다. 
      - 단점 : LIFO에 비해 단편화 관점에서 더 좋은 결과를 보인다는 연구 결과가 있다.

   - **요약(Summary)**
      - 묵시적 리스트와 비교했을 때, 모든 블록 대신 가용 블록의 개수에만 선형 시간으로 할당할 수 있다.
      - 블록의 분할이나 병합 시 리스트 조작 구현이 복잡하다.
      - 두 개의 주소 포인터를 위한 별도의 공간이 필요하다.


## 13. 분할 가용 리스트 Segregated Free Lists(Seglist)
> 모든 가용 블록들을 크기별로 클래스로 나누어 여러 개의 Free list에 저장한다.
- 큰 블록 요청을 위해 2의 제곱수를 기준으로 클래스를 구분한다.
- Allocation 시 요청 크기에 맞는 Free list에서 가용 블록을 찾기 때문에 탐색이 줄어든다.


### 요약
- **간단한 분리 저장장치 Simple Segregated Storage**
    
    > 각 size class에 대한 free list 안의 **free 블록들이 동일한 크기**를 갖는다.
    > 
    - Free 블록들의 크기는 size class의 가장 큰 사이즈로 한다.
    - 각 free 블록들은 할당되어도 **절대로 분할되지 않는다.**
    - 블록의 **할당과 반환이 상수 시간**에 이루어지지만 당연히 **내부, 외부 단편화에 취약**하다.
- **분리 맞춤 Segregated Fit**
    
    > 각 size class에 대한 free list들 안에 **서로 다른 크기의 free 블록들의 배열**들을 저장한다.
    > 
    - 각 Free list 안의 가용 블록들은 서로 다른 크기를 갖지만, 해당 size class의 크기 범위보다 **작거나 크면 안 된다.**


- **사이즈 N의 블록 할당 시**
   - 요청한 사이즈에 맞는 클래스를 검색한다.
   - 적절한 블록을 찾으면 -> 블록을 잘라서 나머지를 다시 적절한 클래스에 해당하는 리스트에 넣을 수 있다.(선택)
   - 해당 클래스 내에 블록을 찾지 못하면, 그 다음 큰 클래스의 리스트로 넘어간다.
   - 이 블록을 찾을 때까지 이 과정을 반복한다.


## 14. 구현 전략과 선택한 이유


| 구분 |  설명 | 이유 
|------|------------------------------|-------|
| 할당자 종류 | 명시적 가용 리스트 중 Segregated(Fit) Free list 방식 사용 |크기 별로 리스트를 나누어서 탐색 범위를 줄여서 속도 향상|
| 클래스 수 | #define NUM_CLASSES 16 | 16개의 크기별 분리 리스트 존재, 2의 배수로 구분|
| 삽입 정책 | LIFO(LAST-IN-FIRST-OUT) |구현 난이도가 주소 정렬이 비해 간단하고, 삽입 비용이 O(1)으로 저렴|
| 병합 정책 | 즉시 병합(Immediate coalescing) | 테스트 케이스 중 제자리 확장이 많음, 일정 thru 달성 시 util 우선으로 trade-off 고려, 나중 병합에 비해 외부 단편화 적음|
| 재할당 정책 | 재할당 패딩 도입으로 복사 없이 제자리 확장(in-place) |제자리 확장으로, util 극대화(테스트 케이스 9, 10 맞춤 정책)|

## 15. 메인 구현 로직 설명
1. `get_class_index` : 분리된 가용 리스트 인덱스를 반환해주는 함수입니다.
```c
 static int get_class_index(size_t size){
    if(size <= 24) return 0;
    else if (size <= 32)  return 1;
    else if (size <= 64)  return 2;
    else if (size <= 128) return 3;
    else if (size <= 256) return 4;
    else if (size <= 512) return 5;
    else if (size <= 1024) return 6;
    else if (size <= 2048) return 7;
    else if (size <= 4096) return 8;
    else if (size <= 8192) return 9;
    else if (size <= 16384) return 10;
    else if (size <= 32768) return 11;
    else if (size <= 65536) return 12;
    else if (size <= 131072) return 13;
    else if (size <= 262144) return 14;
    else return 15;
 }
 ```


2. `find_fit`: first-fit으로 구현되어 있으며, 분리된 가용 리스트에서 자신이 해당되는 클래스에 가용 블록이 없으면 다음 클래스로 넘어간다.
```c
static void *find_fit(size_t asize){
    void *bp;
    int class_idx = get_class_index(asize);

    /* asize가 속하는 클래스 뿐만 아니라 더 큰 클래스까지 찾아야 함 */
    for (int i = class_idx; i < NUM_CLASSES; i++){
        // 다음 페이로드 시작 포인터 반복해서 가리키는 코드(first-fit)
        for (bp = free_list[i]; bp != NULL; bp = SUCC(bp)){
            if(asize <= GET_SIZE(HDRP(bp))){
                return bp;
            }   
        }
    }
    return NULL;
}
```

## 16. 결과 정리
| 할당자 종류 | 배치(탐색) 전략 | Thru + Util Score |
|------|-------------|-----------------|
| implicit | first-fit | 31 + 27 = 58점 |
|  | next-fit + realloc(enhanced) | 44 + 34 = 78점 |
|  | best-fit + realloc(enhanced)  | 42 + 32 = 74점 |
| explicit  | best-fit + realloc(enhanced) | 44 + 40 = 84점 | 
| seglist | first-fit + realloc(enhanced) | **45 + 40 = 85점** | 

- 개선된 realloc은 어떤 정책에서도 큰 폭의 점수 상승을 만든다.
- implicit을 제외한 explicit, seglist에서는 배치 정책(탐색)이 점수에 큰 영향을 미치지 않는다. 

## 17. 회고 및 반성해야 할 점(반성문)

> 네 옆에 있는 팀원들이 고득점인데, 너만 `85점`이면 넌 공간 지역성이 부족한 **실패작**이야. -**chatgpt**- 
>

1. 디버깅 능력 부족
   - 5~6주차 C언어 과정에서 디버깅 도구 사용과 학습을 소홀히 하였다.
   - 특히 이번 주차에서는 복잡한 메모리 구조와 gdb와 valgrind 사용의 미숙함이 겹쳐 디버깅에 시간을 너무 사용하였다.
   - mm_check() 함수 구현 >> 발표 전날에 확인함 
      - 동적 메모리 할당기는 매우 tricky하여 대부분 포인터 연산으로 이루어져 있어 오류가 발생하기 쉽고 디버깅도 어렵다.
      - 따라서 직접 구현한 할당자의 힙 구조를 검사하는 일관성 검사기를 직접 작성해야 한다.
      - 체크할 수 있는 예시 
         - free 리스트에 있는 모든 블록이 실제로 free 상태로 표시되어 있는가?
         - 연속된 free 블록이 병합되지 않고 남아있지는 않은가?
         - 모든 free 블록이 실제로 free 리스트에 포함되어 있는가?
         - free 리스트 안의 포인터들이 실제로 유효한 free 블록을 가리키고 있는가?
         - 할당된 블록들 사이에 겹침(overlap)이 존재하는가?
         - 힙 안의 포인터들이 유효한 힙 주소 범위를 가리키고 있는가?

2. 잘못된 AI 사용법과 설계의 인식
   - Realloc 개선 시 AI의 도움을 받았음.
      ``` c
      /* 요청한 사이즈가 원래 할당된 블록보다 작거나 같으면 */
      if(asize <= current_size){
         size_t remainder = current_size - asize;
         /* 남은 블록이 최소 블록 크기(16바이트)가 넘는다면 분할 */
         if(remainder >= MINBLOCK){
               // 현재 블록을 asize로 축소
               PUT(HDRP(ptr), PACK(asize, 1));
               PUT(FTRP(ptr), PACK(asize, 1));
               
               // 남은 부분을 가용 블록으로 만들기
               void *next = NEXT_BLKP(ptr);
               PUT(HDRP(next), PACK(remainder, 0));
               PUT(FTRP(next), PACK(remainder, 0));
               
               // 가용 블록 병합 시도
               coalesce(next);
         }
         return ptr;
      } else {
         /* 요청한 사이즈가 원래 할당된 블록보다 클 때*/
         /* case 1. 오른쪽 블록이 가용 블록이면서 현재 + 오른쪽 합칠 때 크기 충분 */
         if(!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) && asize <= current_size + rightSize){
               remove_from_free_list(NEXT_BLKP(ptr));
               size_t newsize = current_size + rightSize;
               PUT(HDRP(ptr), PACK(newsize, 1));
               PUT(FTRP(ptr), PACK(newsize, 1));
               
               // 남은 공간이 있으면 분할
               size_t remainder = newsize - asize;
               if(remainder >= MINBLOCK){
                  PUT(HDRP(ptr), PACK(asize, 1));
                  PUT(FTRP(ptr), PACK(asize, 1));
                  void *next = NEXT_BLKP(ptr);
                  PUT(HDRP(next), PACK(remainder, 0));
                  PUT(FTRP(next), PACK(remainder, 0));
                  coalesce(next);
               }
               return ptr;
         } 
         
         /* case 2. 왼쪽 블록이 가용 블록이면서 왼쪽 + 현재 합칠 때 크기 충분 */
         else if(!GET_ALLOC(HDRP(PREV_BLKP(ptr))) && asize <= current_size + leftSize) {
               remove_from_free_list(PREV_BLKP(ptr));
               size_t newsize = current_size + leftSize;
               void *prev = PREV_BLKP(ptr);
               
               // 데이터 복사 (memmove로 안전하게)
               size_t old_payload = current_size - 2 * WSIZE;
               size_t copySize = (size < old_payload) ? size : old_payload;
               memmove(prev, ptr, copySize);
               
               // 병합된 블록 할당
               PUT(HDRP(prev), PACK(newsize, 1));
               PUT(FTRP(prev), PACK(newsize, 1));
               
               // 남은 공간이 있으면 분할
               size_t remainder = newsize - asize;
               if(remainder >= MINBLOCK){
                  PUT(HDRP(prev), PACK(asize, 1));
                  PUT(FTRP(prev), PACK(asize, 1));
                  void *next = NEXT_BLKP(prev);
                  PUT(HDRP(next), PACK(remainder, 0));
                  PUT(FTRP(next), PACK(remainder, 0));
                  coalesce(next);
               }
               return prev;
         } 
         
         /* case 3. 왼쪽, 오른쪽 가용 블록이고 왼쪽 + 현재 + 오른쪽 합치면 충분 */
         else if(!GET_ALLOC(HDRP(PREV_BLKP(ptr))) && !GET_ALLOC(HDRP(NEXT_BLKP(ptr))) 
                  && asize <= current_size + leftSize + rightSize) {
               remove_from_free_list(PREV_BLKP(ptr));
               remove_from_free_list(NEXT_BLKP(ptr));
               size_t newsize = current_size + leftSize + rightSize;
               void *prev = PREV_BLKP(ptr);
               
               // 데이터 복사
               size_t old_payload = current_size - 2 * WSIZE;
               size_t copySize = (size < old_payload) ? size : old_payload;
               memmove(prev, ptr, copySize);
               
               // 병합된 블록 할당
               PUT(HDRP(prev), PACK(newsize, 1));
               PUT(FTRP(prev), PACK(newsize, 1));
               
               // 남은 공간이 있으면 분할
               size_t remainder = newsize - asize;
               if(remainder >= MINBLOCK){
                  PUT(HDRP(prev), PACK(asize, 1));
                  PUT(FTRP(prev), PACK(asize, 1));
                  void *next = NEXT_BLKP(prev);
                  PUT(HDRP(next), PACK(remainder, 0));
                  PUT(FTRP(next), PACK(remainder, 0));
                  coalesce(next);
               }
               return prev;
         } 
         
         /* case 4. 주변 가용 블록이 없거나 합쳐도 크기가 모자라면 */
         else {
               newptr = mm_malloc(size);
               if (newptr == NULL)
                  return NULL;
               copySize = GET_SIZE(HDRP(oldptr)) - 2 * WSIZE;
               if (size < copySize)
                  copySize = size;
               memmove(newptr, oldptr, copySize);
               mm_free(oldptr);
               return newptr;
         }
      }
      ```
   - ai는 병합의 설계를 참고하여, realloc도 유사한 설계를 제공함 -> 필자는 그대로 구현하였음.
      ### realloc padding 기법을 이용한 case 9, 10 개선 
      - realloc 시 inplace를 위해서 realloc 시 필요한 크기보다 더 크게 메모리를 확보한다.
      - 미래의 확장 가능성을 대비하기 위한 일종의 패딩 역할
      - 패딩의 크기는 케이스에 따라 조절
      - case 9, 10 최적화를 위해 설계 변경 시도 -> 설계 변경의 어려움으로 실패
   - 이런 멍청한 설계는 재사용성은 최악인데다, 부동성이 너무 높아 설계 변경 시 변경으로 인해 다른 변경 발생(Code smell)
   - 구현의 도움을 AI에게 받을 수 있지만, 설계까지 도움을 받을 때는 비판적 사고를 가져야 함. 왜 써야 하는가? 대한 끊임없는 질문이 필요함
   - AI는 우리 위에 있다고 생각하지 말자.
   - 로직에 대한 진지한 고민이 필요하다. 

4. 협업 능력 부족
   - 다른 팀원들의 점수들은 97 ~ 98점으로 우수하다. 왜 나만 점수가 낮을까?
   - 나 혼자서 구현할 수 있다는 근거 없는 자신감, 팀원 간 소통이 적었음 >> 메타인지 부재 >> 멍청한 설계로 이어짐.
   - 코어타임 때라던지, 교실 내에서도 끊임없이 서로 간 설계를 공유해야 됨. 
   - 타인의 좋은 설계를 흡수하는 것도 능력.
---
