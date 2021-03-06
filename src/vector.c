#include "vector.h"

#include <string.h>
#include <stdio.h>

#define SHIFT(n) ((size_t)1 << n) // fast 2^n
// this come from stackoverflow, I don't know how it works, but it works
// carefull, return the biggest power of 2 that is smaller than n, 
// so need to add 1 to have the smallest power of 2 larger than n
// don't give 0 to it, it'll return nonsense.
#define LOG2(X) ((unsigned) (8*sizeof (unsigned long long) - __builtin_clzll((X)) - 1)) 
#define vec_getInfo(vec) (*(vec_t**)((vec) - sizeof(vec_t*)))
#define vec_front(vec) ((vec)->baseArr + ((vec)->offset * (vec)->memSize))
#define vec_back(vec) ((vec)->baseArr + (((vec)->offset + (vec)->size) * (vec)->memSize))
#define vec_index(vec, i) ((vec)->baseArr + (((vec)->offset + (i)) * (vec)->memSize))
#define vec_indexFromBack(vec, i) ((vec)->baseArr + (((vec)->offset + (vec)->size - (i)) * (vec)->memSize))
// the next 3 macros are made based on assumpttions:
// it assume that index1 and index2 are < vec->size and index1 != index2
// no need for memmove here as index1 and index2 are assumed different
// so dest and src should not overlap

// use the unused space at the front of the vector as a temporary buffer
// assume that offset > 0
#define vec_swap_front(vecInfo, index1, index2) \
    memcpy((vecInfo)->baseArr, vec_index(vecInfo, index1), (vecInfo)->memSize); \
    memcpy(vec_index(vecInfo, index1), vec_index(vecInfo, index2), (vecInfo)->memSize); \
    memcpy(vec_index(vecInfo, index2), (vecInfo)->baseArr, (vecInfo)->memSize); \
// use the unused space at the end of the vector as a temporary buffer
// assume that the array is not full
#define vec_swap_back(vecInfo, index1, index2) \
    memcpy(vec_back(vecInfo), vec_index(vecInfo, index1), (vecInfo)->memSize); \
    memcpy(vec_index(vecInfo, index1), vec_index(vecInfo, index2), (vecInfo)->memSize); \
    memcpy(vec_index(vecInfo, index2), vec_back(vecInfo), (vecInfo)->memSize); \
// assume that buff is of size memSize
#define vec_swap_buff(vecInfo, index1, index2, buff) \
    memcpy((buff), vec_index(vecInfo, index1), (vecInfo)->memSize); \
    memcpy(vec_index(vecInfo, index1), vec_index(vecInfo, index2), (vecInfo)->memSize); \
    memcpy(vec_index(vecInfo, index2), (buff), (vecInfo)->memSize); \


static void*(*allocator)(size_t) = malloc;
static void(*deallocator)(void*) = free;

// the size of the array is 2^(baseSize) and should be able to be stored in a size_t
// so baseSize can't be bigger than sizeof(size_t) * 8
// so baseSize should'nt be bigger than 64
// so can be easily stored in 1 byte, therefore a unsigned char is enough
typedef struct {
    void* baseArr; // adress of the allocated array
    unsigned char baseSize; // log2 of the allocated size for the array
    size_t size; // number of elem in vec
    size_t offset; // discarded element in front of the vec
    size_t memSize; // size of 1 element
    int (*cmp)(const void*, const void*); // compare function
} vec_t;

static vec_t* vec_init(size_t memSize, size_t size) {
    vec_t* vec = allocator(sizeof(vec_t));
    if(vec == NULL) {
        fprintf(stderr, "vec_init: malloc failed, requested size: %zu\n", sizeof(vec_t));
        return NULL;
    }
    vec->size = size;
    // calculate the smallest power of 2 that is bigger than the size
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
    vec->cmp = NULL;
    return vec;
}

// resize the array to the new baseSize and copy the old array to the new one
// reset offset to 0
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

// check if the array need to be expanded,
// if so, double its size
static void vec_extend(vec_t* vec) {
    // if size + offset is less than the effective size of the array, do nothing
    if(vec->size + vec->offset < SHIFT(vec->baseSize)) return;
    size_t newBaseSize = vec->baseSize + 1;
    vec_resize(vec, newBaseSize);
}

// check if the array need to be shrinked,
// if so, halve its size
static void vec_shrink(vec_t* vec) {
    // if baseSize = 0 or size * 2 is greater than the effective size of the array, do nothing
    if(vec->baseSize == 0) return;
    if(vec->size * 2 > SHIFT(vec->baseSize)) return;
    size_t newBaseSize = vec->baseSize - 1;
    vec_resize(vec, newBaseSize);
}

// create a new vector of default size size and with a size of elements of memeSize
void* vec_create(size_t memSize, size_t size) {
    if(memSize == 0) return NULL;
    vec_t* darr = vec_init(memSize, size);
    if(darr == NULL) return NULL;
    return darr->baseArr;
}

// push an element at the end of the vector
static void vec_pushBack(vec_t* vec, void* value) {
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

// push an element at the front of the vector
static void vec_pushFront(vec_t* vec, void* value) {
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

// resturn the size of the vector
size_t vec_size(const void* vec) {
    if(vec == NULL) return 0;
    const vec_t* vecInfo = vec_getInfo(vec);
    return vecInfo->size;
}

// free the vector
void vec_free(void* vec) {
    if(vec == NULL) return;
    vec_t* arrInfo = *(vec_t**)(vec - sizeof(vec_t*));
    deallocator(arrInfo->baseArr - sizeof(vec_t*));
    deallocator(arrInfo);
}

// store the last element in buff and remove it from the vector
// if buff is NULL, the element is just deleted
static void vec_popBack(vec_t* vec, void* buff) {
    if(vec->size == 0) return;
    if(buff != NULL) memcpy(buff, vec_indexFromBack(vec, 1), vec->memSize);
    vec->size--;
    vec_shrink(vec);
}

void _vec_priv_popBack(void** vecPtr, void* buff) {
    if(vecPtr == NULL || *vecPtr == NULL) return;
    vec_t* vecInfo = vec_getInfo(*vecPtr);
    if(vecInfo->size == 0) return;
    vec_popBack(vecInfo, buff);
    *vecPtr = vec_front(vecInfo);
}

// store the first element in buff and remove it from the vector
// if buff is NULL, the element is just deleted
static void vec_popFront(vec_t* vec, void* buff) {
    if(vec->size == 0) return;
    if(buff != NULL) memcpy(buff, vec->baseArr + vec->offset * vec->memSize, vec->memSize);
    vec->size--;
    vec->offset++;
    memcpy(vec_front(vec) - sizeof(vec_t*), &vec, sizeof(vec_t*));
    vec_shrink(vec);
}

void _vec_priv_popFront(void** vecPtr, void* buff) {
    if(vecPtr == NULL || *vecPtr == NULL) return;
    vec_t* vecInfo = vec_getInfo(*vecPtr);
    if(vecInfo->size == 0) return;
    vec_popFront(vecInfo, buff);
    *vecPtr = vec_front(vecInfo);
}

// sort the vector using the given comparator
void vec_sort(void* vec) {
    if(vec == NULL) return;
    vec_t* vecInfo = vec_getInfo(vec);
    if(vecInfo->cmp == NULL) {
        fprintf(stderr, "Error: vec_sort: no comparator set\n");
        return;
    }
    qsort(vec_front(vecInfo), vecInfo->size, vecInfo->memSize, vecInfo->cmp);
}

// sort the vector using the comparator given as argument
void vec_qsort(void* vec, int (*compar_fn) (const void *, const void *)) {
    if(vec == NULL) return;
    vec_t* vecInfo = vec_getInfo(vec);
    qsort(vec_front(vecInfo), vecInfo->size, vecInfo->memSize, compar_fn);
}

// return a new array containing the elements beetween start and end, end excluded
void* _vec_priv_slice(void* vec, size_t start, size_t end) {
    if(vec == NULL) return NULL;
    vec_t* vecInfo = vec_getInfo(vec);
    // if start is out of bounds return empty array
    if(start >= vecInfo->size) return vec_create(vecInfo->memSize, 0);
    // if end is out of bounds set it to the end of the array
    if(end > vecInfo->size) end = vecInfo->size;
    // just create a new array with the right size
    void* newArr = vec_create(vecInfo->memSize, end - start);
    // and copy the values
    memcpy(newArr, vec_index(vecInfo, start), (end - start) * vecInfo->memSize);
    return newArr;
}

// insert an element at the given index
static void vec_insert(vec_t* vecInfo, size_t index, void* value) {
    if(index > vecInfo->size) return;
    // if index is at the end, just pushBack
    if(index >= vecInfo->size) {
        vec_pushBack(vecInfo, value);
        return;
    }
    // if at the front pushFront
    if(index == 0) {
        vec_pushFront(vecInfo, value);
        return;
    }
    vec_extend(vecInfo);
    // need memmove here because everything is moved over itself by one element

    // case where less elements are at the left of the index and offset != 0
    if(index < vecInfo->size - index && vecInfo->offset > 0) {
        // move everything at the left of the index to the left by one element
        // can access index -1 as offset is > 0
        memmove(vec_index(vecInfo, -1), vec_front(vecInfo), index * vecInfo->memSize);
        vecInfo->offset--;
    } else { 
        // move everything at the right of the index to the right by one element
        memmove(vec_index(vecInfo, index + 1), vec_index(vecInfo, index), (vecInfo->size - index) * vecInfo->memSize);
    }
    // insert the value
    memcpy(vec_index(vecInfo, index), value, vecInfo->memSize);
    vecInfo->size++;
}

void _vec_priv_insert(void** vecPtr, size_t index, void* value) {
    if(vecPtr == NULL || *vecPtr == NULL) return;
    vec_t* vecInfo = vec_getInfo(*vecPtr);
    vec_insert(vecInfo, index, value);
    *vecPtr = vec_front(vecInfo);
}

void _vec_priv_remove(void** vecPtr, size_t index, void* buff) {
    if(vecPtr == NULL || *vecPtr == NULL) return;
    vec_t* vecInfo = vec_getInfo(*vecPtr);
    if(index >= vecInfo->size) return;
    if(buff != NULL) memcpy(buff, vec_index(vecInfo, index), vecInfo->memSize);
    // need memmove here because everything is moved over itself by one element
    memmove(vec_index(vecInfo, index), vec_index(vecInfo, index + 1), (vecInfo->size - index - 1) * vecInfo->memSize);
    vecInfo->size--;
    vec_shrink(vecInfo);
    *vecPtr = vec_front(vecInfo);
}


// swap the elements at the given indexes
void vec_swap(void* vec, size_t index1, size_t index2) {
    if(vec == NULL || index1 == index2) return;
    vec_t* vecInfo = vec_getInfo(vec);
    if(index1 >= vecInfo->size || index2 >= vecInfo->size) return;

    // the two possibility here avoid the need of a temp buffer
    // by writing the values directly to unused memory of the array
    // see functions definitions for more details
    if(vecInfo->offset != 0) {
        vec_swap_front(vecInfo, index1, index2);
        return;
    } else if(SHIFT(vecInfo->baseSize) > vecInfo->size) {
        vec_swap_back(vecInfo, index1, index2);
        return;
    }

    // maybe allocating the buffer to the stack could be an optimization

    void* buff = allocator(vecInfo->memSize);
    if(buff == NULL) {
        fprintf(stderr, "vec_swap: buffer malloc failed, requested size: %zu\n", vecInfo->memSize);
        return;
    }
    vec_swap_buff(vecInfo, index1, index2, buff);
    deallocator(buff);
}

// remove all elements from the vector and set its size to 0
void _vec_priv_clear(void** vecPtr) {
    if(vecPtr == NULL || *vecPtr == NULL) return;
    vec_t* vecInfo = vec_getInfo(*vecPtr);
    // need to set size to 0 now because vec_resize copy the old array
    vecInfo->size = 0;
    vec_resize(vecInfo, 0);
    *vecPtr = vec_front(vecInfo);
}


void _vec_debug_print(void* vec, FILE* stream) {
    if(vec == NULL) return;
    vec_t* vecInfo = vec_getInfo(vec);
    fprintf(stream, "size: %lu, offset: %lu, memSize: %lu, baseSize: %u\n", vecInfo->size, vecInfo->offset, vecInfo->memSize, vecInfo->baseSize);
    fprintf(stream, "effective memsize: %lu\n", SHIFT(vecInfo->baseSize) * vecInfo->memSize + sizeof(vec_t) + sizeof(vec_t*));
}

// preallocate the vector to the given size
static void vec_reserve(vec_t* vec, size_t newSize) {
    if(newSize <= vec->baseSize) return;
    size_t newBaseSize = LOG2(newSize ? newSize : 1) + 1;
    if(newBaseSize > vec->baseSize) {
        vec_resize(vec, newBaseSize);
    }
}

// preallocates the vector to the given size and if resize is true set its size to the given size
void vec_allocate(void* vecPtr, size_t newSize, int resize) {
    if(vecPtr == NULL || *(void**)vecPtr == NULL) return;
    vec_t* vecInfo = vec_getInfo(*(void**)vecPtr);
    vec_reserve(vecInfo, newSize);
    if(resize) vecInfo->size = newSize;
    *(void**)vecPtr = vec_front(vecInfo);
}

// reverse the vector
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
            vec_swap_front(vecInfo, i, j);
        }
        return;
    } else if(SHIFT(vecInfo->baseSize) > vecInfo->size) {
        for(size_t i = 0, j = vecInfo->size - 1; i < j; i++, j--) {
            vec_swap_back(vecInfo, i, j);
        }
        return;
    }

    // same as vec_swap, maybe allocating the buffer to the stack
    void* buff = allocator(vecInfo->memSize);
    if(buff == NULL) {
        fprintf(stderr, "vec_reverse: buffer malloc failed, requested size: %zu\n", vecInfo->memSize);
        return;
    }
    for(size_t i = 0, j = vecInfo->size - 1; i < j; i++, j--) {
        vec_swap_buff(vecInfo, i, j, buff);
    }
    deallocator(buff);
}

// set theallocator function
void vec_set_allocator(void* (*_allocator)(size_t)) {
    allocator = _allocator;
}

// set the deallocator function
void vec_set_deallocator(void (*_deallocator)(void*)) {
    deallocator = _deallocator;
}

// set the comparator function for the vector
void vec_setComparator(void* vec, int (*cmp)(const void*, const void*)) {
    if(vec == NULL) return;
    vec_t* vecInfo = vec_getInfo(vec);
    vecInfo->cmp = cmp;
}

// return the index where the element should be inserted to keep the vector sorted
static size_t vec_find_sorted_insertion(const vec_t* vecInfo, const void* value) {
    if(vecInfo->cmp == NULL) {
        fprintf(stderr, "vec_find_sorted_insertion: no compare function set\n");
        return vecInfo->size;
    }
    size_t i = 0, j = vecInfo->size, m;
    // don't search an equal value, but rather the insertion point
    // loop until finding m such as
    // the value at index m - 1 is smaller or equal to the value
    // and the value at index m is greater to the value
    // in case value is the smallest value, the insertion point is 0
    // in case value is the largest value, the insertion point is vecInfo->size
    // this is not that much more heavy work, as it will just always reach worst case scenario of a "normal" bsearch
    while(i < j) {
        m = (i + j) / 2;
        if(vecInfo->cmp(vec_index(vecInfo, m), value) <= 0) {
            i = m + 1;
        } else {
            j = m;
        }
    }
    return i;
}

// insert the value at the right place to keep the array sorted
// assume that the array is already sorted
size_t _vec_priv_sortedInsert(void** vecPtr, void* value) {
    if(vecPtr == NULL || *vecPtr == NULL) return 0;
    vec_t* vecInfo = vec_getInfo(*vecPtr);
    size_t i = vec_find_sorted_insertion(vecInfo, value);
    vec_insert(vecInfo, i, value);
    *vecPtr = vec_front(vecInfo);
    return i;
}

// return if the array is sorted, do a linear comparaison
int vec_isSorted(const void* vec) {
    if(vec == NULL) return 1;
    const vec_t* vecInfo = vec_getInfo(vec);
    if(vecInfo->cmp == NULL) {
        fprintf(stderr, "vec_isSorted: no compare function set\n");
        return 0;
    }
    for(size_t i = 1; i < vecInfo->size; i++) {
        if(vecInfo->cmp(vec_index(vecInfo, i - 1), vec_index(vecInfo, i)) > 0) {
            return 0;
        }
    }
    return 1;
}

/**
 * 
 * TLDR: I'm doing black magic in C, and it work better than it should.
 * 
 * 
 * I will try to explain the inner workings of this code, which is pure black magic,
 * and will surely make some people mad.
 * (first thing, the struct vec_t contain the infos about the vector.)
 * When I tried to design this lib for the first time, 
 * I gave back the struct to the user and he add to access an array in the struct
 * But it was not practical, and I wanted to just do vec[i] = x, and not vec.arr[i] = x
 * so I gave back the array to the user and he can access it with the [] operator.
 * but now, how do I access the info when the user want to push item or the size?
 * well at first I've done the most stupid things that came to my mind, 
 * I just created an array that store the infos of all the current vec, 
 * and do a bsearch when I need to access the info of a vec.
 * so as you can imagine, I need to sort the array everytime the vec address change,
 * so most of the workload was sorting and searching in an array.
 * but now, I use some address pointers tricks to make it more efficient.
 * when I allocate the memory for the array, I also allocate the size of an address to the struct vec_t
 * so instead of allocating size * memSize, I allocate size * memSize + sizeof(vec_t*)
 * and I give back the address, but shifted of sizeof(vec_t*)
 * so when the user use functions on the vector, I just shift it back to retrieve the address
 * (see macro vec_getInfo(vec))
 * I don't know if its the best, if it's really safe, but it works, and it's fast, 
 * and it's very convenient, for the user and for me.
 * 
 * user could use the "privates" functions of the library, but the prefered way is to define wrapper functions.
 * macros to define them are defines in the header file.
 * most of the functions are inlined so it does'nt create actual functions, and can be defined in multiple files
 * without creating the same function multiple times, 
 * and the compiler can optimize them as they are very small wrappers.
 * 
 * An other thing, I'm not an expert in dynammic arrays, so I just implemented
 * a logic I came accross one time, and I've done it the way that seams the best for me.
 * in vect_t, the property baseSize is an power of 2 of the actual size of the array,
 * so the array size is 2^(baseSize)
 * yes this means that there is unused memory in the array, but this make insertion and deletion
 * fast as they are now O(log(n)) instead of O(n))
 * (I speak in term of reallocation, which mean copying the array, only log2(n) times instead of n times)
 * I also heard about the fact that computer likes to always be propare that functions are gonna be call multiple
 * so when the user push to the front, if their is no space at the front, I move the array to the back,
 * not just of one element, but of the whole array.
 * I do the smae thing when the user push to the back, I just move the array to the front, setting the offset to 0.
 * This whole logic is done in vec_expand() and vec_shrink(), which are just wrappers for vec_resize().
 * no other functions than vec_resize() should resize, only move memory.
 * all function that insert element in the vector call vec_expand(),
 * and all function that remove element from the vector call vec_shrink().
 * this insure that the array is always in the right size.
 * the only function that can oversize the array is vec_reserve() (which is also a wrapper for vec_resize())
 * 
 * some macro are defined to access part of the array, see vec_: front, back, index, indexFromBack
 * defined to make the code more readeable when moving memory around.
 * 
 * functions that intend to modify the vec size take the vec address as first argument,
 * that way the memory address of the arr can be replaced by the new one if the array is resized.
 * so all these function finish by *vecPtr = vec_front(vecInfo)
 * 
 * 
 * I hope the code is not too hard to understand, I try to keep it as clean as possible,
 * but this is difficult has it's mostly memory movement.
 * this lib is just me tweaking around in C to improve myself,
 * I'm sure most of the functions could be written in a better way and more optimized,
 * If you see any improvement, please let me know.
 * 
 * Thanks for reading,
 * have a nice day,
 * Baptiste de Montangon.
 */
