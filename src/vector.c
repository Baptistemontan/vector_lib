#include "vector.h"

#include <string.h>
#include <stdio.h>

#define SHIFT(n) (1 << n) // fast 2^n
#define LOG2(X) ((unsigned) (8*sizeof (unsigned long long) - __builtin_clzll((X)) - 1)) // this come from stackoverflow, I don't know how it works, but it works
#define vec_getInfo(vec) (*(vec_t**)((vec) - sizeof(vec_t*)))
#define vec_front(vec) ((vec)->baseArr + ((vec)->offset * (vec)->memSize))
#define vec_back(vec) ((vec)->baseArr + (((vec)->offset + (vec)->size) * (vec)->memSize))
#define vec_index(vec, i) ((vec)->baseArr + (((vec)->offset + (i)) * (vec)->memSize))
#define vec_indexFromBack(vec, i) ((vec)->baseArr + (((vec)->offset + (vec)->size - (i)) * (vec)->memSize))

static void*(*allocator)(size_t) = malloc;
static void(*deallocator)(void*) = free;

// size_t for everything is kinda overkill but meh,
// struct is 40 bytes and storing the adress is 8 more
// thats a total of 48 bytes per array to store info
// thats the size of an array of 12 int.
typedef struct {
    void* baseArr; // adress of the allocated array
    size_t baseSize; // log2 of the allocated size for the array
    size_t size; // number of elem in vec
    size_t offset; // discarded element in front of the vec
    size_t memSize; // size of 1 element
} vec_t;

static vec_t* vec_init(size_t memSize, size_t size) {
    vec_t* vec = allocator(sizeof(vec_t));
    if(vec == NULL) {
        fprintf(stderr, "vec_init: malloc failed, requested size: %zu\n", sizeof(vec_t));
        return NULL;
    }
    vec->size = size;
    vec->baseSize = LOG2(size ? size : 1) + 1;
    void* baseArr = allocator((memSize * SHIFT(vec->baseSize)) + sizeof(vec_t*));
    if(baseArr == NULL) {
        fprintf(stderr, "vec_init: malloc failed, requested size: %zu\n", (memSize * SHIFT(vec->baseSize)) + sizeof(vec_t*));
        deallocator(vec);
        return NULL;
    }
    vec->baseArr = baseArr + sizeof(vec_t*);
    memcpy(baseArr, &vec, sizeof(vec_t*));
    vec->offset = 0;
    vec->memSize = memSize;
    return vec;
}

static void vec_resize(vec_t* vec, size_t newBaseSize) {
    void* newArr = allocator(vec->memSize * SHIFT(newBaseSize) + sizeof(vec_t*));
    if(newArr == NULL) {
        fprintf(stderr, "vec_resize: malloc failed, requested size: %zu\n", vec->memSize * SHIFT(newBaseSize) + sizeof(vec_t*));
        return;
    }
    memcpy(newArr + sizeof(vec_t*), vec_front(vec), vec->size * vec->memSize);
    memcpy(newArr, &vec, sizeof(vec_t*));
    deallocator(vec->baseArr - sizeof(vec_t*));
    vec->baseArr = newArr + sizeof(vec_t*);
    vec->baseSize = newBaseSize;
    vec->offset = 0;
}

static void vec_extend(vec_t* vec) {
    if(vec->size + vec->offset < SHIFT(vec->baseSize)) return;
    size_t newBaseSize = vec->baseSize + 1;
    vec_resize(vec, newBaseSize);
}

static void vec_reduce(vec_t* vec) {
    if(vec->baseSize == 0) return;
    if(vec->size * 2 > SHIFT(vec->baseSize)) return;
    size_t newBaseSize = vec->baseSize - 1;
    vec_resize(vec, newBaseSize);
}

void* vec_create(size_t memSize, size_t size) {
    if(memSize == 0) return NULL;
    vec_t* darr = vec_init(memSize, size);
    if(darr == NULL) return NULL;
    return darr->baseArr;
}

static void vec_pushBack(vec_t*__restrict__ vec, void*__restrict__ value) {
    vec_extend(vec);
    memcpy(vec_back(vec), value, vec->memSize);
    vec->size++;
}

void _vec_priv_pushBack(void** vecPtr, void* value) {
    if(value == NULL || vecPtr == NULL || *vecPtr == NULL) return;
    vec_t* vecInfo = vec_getInfo(*vecPtr);
    vec_pushBack(vecInfo, value);
    *vecPtr = vec_front(vecInfo);
}

static void vec_pushFront(vec_t* vec, void*__restrict__ value) {
    if(vec->offset > 0) {
        vec->offset--;
        vec->size++;
        memcpy(vec_front(vec), value, vec->memSize);
        memcpy(vec_front(vec) - sizeof(vec_t*), &vec, sizeof(vec_t*));
    } else {
        // create room if needed
        vec_extend(vec);
        // shift everything to back of the array
        vec->offset = SHIFT(vec->baseSize) - vec->size;
        // if no offset (should not be possible) do nothing,
        // this is to avoid infinite recursive calls
        if(vec->offset == 0) return;
        // need memmove here because everything is moved over itself
        memmove(vec_front(vec), vec->baseArr, vec->size * vec->memSize);
        // retry to push the value
        vec_pushFront(vec,value);
    }
}

void _vec_priv_pushFront(void** vecPtr, void* value) {
    if(value == NULL || vecPtr == NULL || *vecPtr == NULL) return;
    vec_t* vecInfo = vec_getInfo(*vecPtr);
    vec_pushFront(vecInfo, value);
    *vecPtr = vec_front(vecInfo);
}

size_t vec_size(const void* vec) {
    if(vec == NULL) return 0;
    const vec_t* vecInfo = vec_getInfo(vec);
    return vecInfo->size;
}

void vec_free(void* vec) {
    if(vec == NULL) return;
    vec_t* arrInfo = *(vec_t**)(vec - sizeof(vec_t*));
    deallocator(arrInfo->baseArr - sizeof(vec_t*));
    deallocator(arrInfo);
}


static void vec_popBack(vec_t*__restrict__ vec, void*__restrict__ buff) {
    if(vec->size == 0) return;
    if(buff != NULL) memcpy(buff, vec_indexFromBack(vec, 1), vec->memSize);
    vec->size--;
    vec_reduce(vec);
}

void _vec_priv_popBack(void** vecPtr, void* buff) {
    if(vecPtr == NULL || *vecPtr == NULL) return;
    vec_t* vecInfo = vec_getInfo(*vecPtr);
    if(vecInfo->size == 0) return;
    vec_popBack(vecInfo, buff);
    *vecPtr = vec_front(vecInfo);
}

static void vec_popFront(vec_t* vec, void*__restrict__ buff) {
    if(vec->size == 0) return;
    if(buff != NULL) memcpy(buff, vec->baseArr + vec->offset * vec->memSize, vec->memSize);
    vec->size--;
    vec->offset++;
    memcpy(vec_front(vec) - sizeof(vec_t*), &vec, sizeof(vec_t*));
    vec_reduce(vec);
}

void _vec_priv_popFront(void** vecPtr, void* buff) {
    if(vecPtr == NULL || *vecPtr == NULL) return;
    vec_t* vecInfo = vec_getInfo(*vecPtr);
    if(vecInfo->size == 0) return;
    vec_popFront(vecInfo, buff);
    *vecPtr = vec_front(vecInfo);
}

void vec_qsort(void* vec, int (*compar_fn) (const void *, const void *)) {
    if(vec == NULL) return;
    vec_t* vecInfo = vec_getInfo(vec);
    qsort(vec, vecInfo->size, vecInfo->memSize, compar_fn);
}

void* _vec_priv_slice(void* vec, size_t start, size_t end) {
    if(vec == NULL) return NULL;
    vec_t* vecInfo = vec_getInfo(vec);
    if(start >= vecInfo->size) return vec_create(vecInfo->memSize, 0);
    if(end > vecInfo->size) end = vecInfo->size;
    void* newArr = vec_create(vecInfo->memSize, end - start);
    memcpy(newArr, vec_index(vecInfo, start), (end - start) * vecInfo->memSize);
    return newArr;
}

static void vec_insert(vec_t* vec, size_t index, void* value) {
    if(index > vec->size) return;
    vec_extend(vec);
    // need memmove here because everything is moved over itself by one element
    memmove(vec_index(vec, index + 1), vec_index(vec, index), (vec->size - index) * vec->memSize);
    memcpy(vec_index(vec, index), value, vec->memSize);
    vec->size++;
}

void _vec_priv_insert(void** vecPtr, size_t index, void* value) {
    if(vecPtr == NULL || *vecPtr == NULL) return;
    vec_t* vecInfo = vec_getInfo(*vecPtr);
    if(index >= vecInfo->size) {
        vec_pushBack(vecInfo, value);
        return;
    }
    if(index == 0) {
        vec_pushFront(vecInfo, value);
        return;
    }
    vec_insert(vecInfo, index, value);
    *vecPtr = vec_front(vecInfo);
}

void _vec_priv_remove(void** vecPtr, size_t index, void*__restrict__ buff) {
    if(vecPtr == NULL || *vecPtr == NULL) return;
    vec_t* vecInfo = vec_getInfo(*vecPtr);
    if(index >= vecInfo->size) return;
    if(buff != NULL) memcpy(buff, vec_index(vecInfo, index), vecInfo->memSize);
    // need memmove here because everything is moved over itself by one element
    memmove(vec_index(vecInfo, index), vec_index(vecInfo, index + 1), (vecInfo->size - index - 1) * vecInfo->memSize);
    vecInfo->size--;
    vec_reduce(vecInfo);
    *vecPtr = vec_front(vecInfo);
}

void vec_swap(void* vec, size_t index1, size_t index2) {
    if(vec == NULL || index1 == index2) return;
    vec_t* vecInfo = vec_getInfo(vec);
    if(index1 >= vecInfo->size || index2 >= vecInfo->size) return;
    // no need for memmove here as index1 and index2 are always different
    // so dest and src can't overlap

    // the two possibility here avoid the need of a temp buffer
    // by writing the values directly to unused memory of the array
    if(vecInfo->offset != 0) {
        memcpy(vecInfo->baseArr, vec_index(vecInfo, index1), vecInfo->memSize);
        memcpy(vec_index(vecInfo, index1), vec_index(vecInfo, index2), vecInfo->memSize);
        memcpy(vec_index(vecInfo, index2), vecInfo->baseArr, vecInfo->memSize);
        return;
    } else if(SHIFT(vecInfo->baseSize) > vecInfo->size) {
        memcpy(vec_back(vecInfo), vec_index(vecInfo, index1), vecInfo->memSize);
        memcpy(vec_index(vecInfo, index1), vec_index(vecInfo, index2), vecInfo->memSize);
        memcpy(vec_index(vecInfo, index2), vec_back(vecInfo), vecInfo->memSize);
        return;
    }

    void* buff = allocator(vecInfo->memSize);
    if(buff == NULL) {
        fprintf(stderr, "vec_swap: buffer malloc failed, requested size: %zu\n", vecInfo->memSize);
        return;
    }
    memcpy(buff, vec_index(vecInfo, index1), vecInfo->memSize);
    memcpy(vec_index(vecInfo, index1), vec_index(vecInfo, index2), vecInfo->memSize);
    memcpy(vec_index(vecInfo, index2), buff, vecInfo->memSize);
    deallocator(buff);
}

void _vec_priv_clear(void** vecPtr) {
    if(vecPtr == NULL || *vecPtr == NULL) return;
    vec_t* vecInfo = vec_getInfo(*vecPtr);
    vecInfo->size = 0;
    vec_resize(vecInfo, 0);
    *vecPtr = vec_front(vecInfo);
}


void _vec_debug_print(void* vec, FILE* stream) {
    if(vec == NULL) return;
    vec_t* vecInfo = vec_getInfo(vec);
    fprintf(stream, "size: %lu, offset: %lu, memSize: %lu, baseSize: %lu\n", vecInfo->size, vecInfo->offset, vecInfo->memSize, vecInfo->baseSize);
    fprintf(stream, "effective memsize: %lu\n", SHIFT(vecInfo->baseSize) * vecInfo->memSize + sizeof(vec_t) + sizeof(vec_t*));
}

static void vec_preReserve(vec_t* vec, size_t newSize) {
    if(newSize <= vec->baseSize) return;
    size_t newBaseSize = LOG2(newSize ? newSize : 1) + 1;
    if(newBaseSize > vec->baseSize) {
        vec_resize(vec, newBaseSize);
    }
}


void vec_allocate(void* vecPtr, size_t newSize, int resize) {
    if(vecPtr == NULL || *(void**)vecPtr == NULL) return;
    vec_t* vecInfo = vec_getInfo(*(void**)vecPtr);
    vec_preReserve(vecInfo, newSize);
    if(resize) vecInfo->size = newSize;
    *(void**)vecPtr = vec_front(vecInfo);
}

void vec_reverse(void* vec) {
    if(vec == NULL) return;
    vec_t* vecInfo = vec_getInfo(vec);
    // no need for memmove here as i is strictly inferior to j,
    // so i always different from j
    // so dest and src can't overlap

    // the two possibility here avoid the need of a temp buffer
    // by writing the values directly to the array
    if(vecInfo->offset != 0) {
        for(size_t i = 0, j = vecInfo->size - 1; i < j; i++, j--) {
            memcpy(vecInfo->baseArr, vec_index(vecInfo, i), vecInfo->memSize);
            memcpy(vec_index(vecInfo, i), vec_index(vecInfo, j), vecInfo->memSize);
            memcpy(vec_index(vecInfo, j), vecInfo->baseArr, vecInfo->memSize);
        }
        return;
    } else if(SHIFT(vecInfo->baseSize) > vecInfo->size) {
        for(size_t i = 0, j = vecInfo->size - 1; i < j; i++, j--) {
            memcpy(vec_back(vecInfo), vec_index(vecInfo, i), vecInfo->memSize);
            memcpy(vec_index(vecInfo, i), vec_index(vecInfo, j), vecInfo->memSize);
            memcpy(vec_index(vecInfo, j), vec_back(vecInfo), vecInfo->memSize);
        }
        return;
    }
    void* buff = allocator(vecInfo->memSize);
    if(buff == NULL) {
        fprintf(stderr, "vec_reverse: buffer malloc failed, requested size: %zu\n", vecInfo->memSize);
        return;
    }
    for(size_t i = 0, j = vecInfo->size - 1; i < j; i++, j--) {
        memcpy(buff, vec_index(vecInfo, i), vecInfo->memSize);
        memcpy(vec_index(vecInfo, i), vec_index(vecInfo, j), vecInfo->memSize);
        memcpy(vec_index(vecInfo, j), buff, vecInfo->memSize);
    }
    deallocator(buff);
}

void vec_set_allocator(void* (*_allocator)(size_t)) {
    allocator = _allocator;
}

void vec_set_deallocator(void (*_deallocator)(void*)) {
    deallocator = _deallocator;
}
