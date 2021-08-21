#ifndef HEAD_VEC_T
#define HEAD_VEC_T

#include <stdlib.h>
#include <stdio.h>

/**  
 * All functions defined with macros are inlined (except for maps functions),
 * they are only wrappers for the internal functions (marked with _priv_),
 * made for abstraction. they will not create new functions, as the compiler will
 * inline them, so you can define them in all your files without creating
 * multiple time the same functions.
 * you also can define them in a header file, and include it in all your files.
 * 
 * You should use the macros, but if you feel you understand 
 * the mechanics of the internal functions, feel free to use them.
 * 
 * Functions that modify the array need the pointer to the array as parameter, 
 * not the array itself, param name will be vecPtr instead of vec.
 * 
 * DO NOT GIVE SHIFTED ARRAY TO THESE FUNCTIONS, THEY WILL NOT WORK.
 * exemple: if you give vec + x to vec_size, it will segfault if x != 0.
 * this doesn't mean you can't use shifted arrays, just don't give them to the functions,
 * it will segfault.
 * 
 * for all macros the first parameter is the array type,
 * the second parameter is a suffix for the functions, 
 * this way you can define functions for array of pointers.
 * 
 * for details about each function see the functions comments.
 * 
 * functions can be defined individually, but VEC_DEF_ALL() define all the functions at once.
 * 
 * You also can overwrite allocator and deallocator functions to use custom ones. 
 */

// return an array of the given type that can be accessed like a normal array
// need to be freed with vec_free()
#define VEC_DEF_CREATE(type, suffix) \
    inline type* vec_create_##suffix(size_t _size) { \
        return (type*)vec_create(sizeof(type), _size); \
    } 

// push an element to the end of the array
// need the array pointer as parameter, not the array itself
#define VEC_DEF_PUSHBACK(type, suffix) \
    inline type vec_pushBack_##suffix(type** _vecPtr, type _value) { \
        _vec_priv_pushBack((void**)_vecPtr, &_value); \
        return _value; \
    }

// push an element to the front of the array
// need the array pointer as parameter, not the array itself
#define VEC_DEF_PUSHFRONT(type, suffix) \
    inline type vec_pushFront_##suffix(type** _vecPtr, type _value) { \
        _vec_priv_pushFront((void**)_vecPtr, &_value); \
        return _value; \
    }

// pop an element from the end of the array
// need the array pointer as parameter, not the array itself
#define VEC_DEF_POPBACK(type, suffix) \
    inline type vec_popBack_##suffix(type** _vecPtr) { \
        type _buff; \
        _vec_priv_popBack((void**)_vecPtr, &_buff); \
        return _buff; \
    }

// pop an element from the front of the array
// need the array pointer as parameter, not the array itself
#define VEC_DEF_POPFRONT(type, suffix) \
    inline type vec_popFront_##suffix(type** _vecPtr) { \
        type _buff; \
        _vec_priv_popFront((void**)_vecPtr, &_buff); \
        return _buff; \
    }

// return a new array containing the elements of the given array
// beetween the given indexes
// need to be freed with vec_free()
#define VEC_DEF_SLICE(type, suffix) \
    inline type* vec_slice_##suffix(type* _vec, size_t _start, size_t _end) { \
        return (type*)_vec_priv_slice(_vec, _start, _end); \
    }

// insert an element at the given index
// need the array pointer as parameter, not the array itself
// if index is out of bounds, the element will be pushed to the end of the array
// if insertion is either at front or end, will fallback to pushBack or pushFront
// for other index this is slower
#define VEC_DEF_INSERT(type, suffix) \
    inline type vec_insert_##suffix(type** _vecPtr, size_t _index, type _value) { \
        _vec_priv_insert((void**)_vecPtr, _index, &_value); \
        return _value; \
    }

// remove an element at the given index and return it
// need the array pointer as parameter, not the array itself
#define VEC_DEF_REMOVE(type, suffix) \
    inline type vec_remove_##suffix(type** _vecPtr, size_t _index) { \
        type _buff; \
        _vec_priv_remove((void**)_vecPtr, _index, &_buff); \
        return _buff; \
    }

// wrapper for bsearch, so behave just like it.
// bsearch being inline, the size is saved in a variable to avoid recomputing it
#define VEC_DEF_BSEARCH(type, suffix) \
    inline type* vec_bsearch_##suffix(type* _vec, type _value, int (*_compar_fn) (const void *, const void *)) { \
        size_t _size = vec_size(_vec); \
        return bsearch(&_value, _vec, _size, sizeof(type), _compar_fn); \
    }

// clear the array, remove all elements and reset size to 0
// need the array pointer as parameter, not the array itself
#define VEC_DEF_CLEAR(type, suffix) \
    inline void vec_clear_##suffix(type** _vecPtr) { \
        _vec_priv_clear((void**)_vecPtr); \
    }

// commodity macro to define all functions
#define VEC_DEF_ALL(type, suffix) \
    VEC_DEF_CREATE(type, suffix) \
    VEC_DEF_PUSHBACK(type, suffix) \
    VEC_DEF_PUSHFRONT(type, suffix) \
    VEC_DEF_POPBACK(type, suffix) \
    VEC_DEF_POPFRONT(type, suffix) \
    VEC_DEF_SLICE(type, suffix) \
    VEC_DEF_INSERT(type, suffix) \
    VEC_DEF_REMOVE(type, suffix) \
    VEC_DEF_BSEARCH(type, suffix) \
    VEC_DEF_CLEAR(type, suffix) \

// map function is not inlined
// so it will define a function that will be compiled.
// if you use this in multiple files, you should define
// the function in only one file.
#define VEC_DEF_MAP(fromType, toType, suffix) \
    toType* vec_map_##suffix(fromType* vec, toType (*map_fn)(fromType, size_t)) { \
        size_t arrSize = vec_size(vec); \
        toType* newArr = vec_create(sizeof(toType), arrSize); \
        for(size_t i = 0; i < arrSize; i++) { \
            newArr[i] = map_fn(vec[i], i); \
        } \
        return newArr; \
    }

// for next 2 functions, put the loop in a new block to scope the val variable

// foreach emulations, can be used like:
// vec_foreach(vec, int, i, val, 
//     printf("%d\n", val);
// )
// val is the value of the current element
// iter is the index iterator, contain index of current element
#define vec_foreach(vec, type, iter, val, loop) \
    { \
        type val; \
        for(size_t iter = 0; iter < vec_size(vec); iter++) {\
            val = vec[iter]; \
            loop \
        } \
    }

// same as vec_foreach, but in reverse order
#define vec_foreach_reverse(vec, type, iter, val, loop) \
    { \
        type val; \
        for(size_t iter = vec_size(vec) - 1; iter >= 0; iter--) {\
            val = vec[iter]; \
            loop \
        } \
    }

// define wrappers for compare function to be used with either bsearch or qsort
// not inlined (for obvious reasons)
#define DYNNAR_COMPARE_FN(type, suffix, compareFn) \
    int vec_compare_##suffix(const void* a, const void* b) { \
        return compareFn(*(type*)a, *(type*)b); \
    }




// public functions

// create a new array of elements of size memSize and of min-size size
// array will be of the given size, if you init it of size 10, every push will append after the 10th element
// if you want to pre allocate memory, init with size 0 and use preAllocate() function
void* vec_create(size_t memSize, size_t size);
// return the size of the array
size_t vec_size(const void* vec);
// free the array
void vec_free(void* vec);
// sort the array, just a wrapper for qsort
void vec_qsort(void* vec, int (*compar_fn) (const void *, const void *));
// if resize is true :
// allocate memory for newSize element and set the size to newSize
// do nothing if newSize is smaller than the current size
// if resize is false :
// allocate memory for newSize element but keep the size, allowing to add elements with adding functions
// without reallocating when place need to be made
// do nothing if enough memory is already allocated
// caution: functions that removes elements will automatically resize the array to its min-size
void vec_allocate(void* vecPtr, size_t newSize, int resize);
// reverse the array
void vec_reverse(void* vec);
// swap two elements in the array
void vec_swap(void* vecPtr, size_t index1, size_t index2);
// overwrite the allocator function of the library, default is malloc
void vec_set_allocator(void* (*_allocator)(size_t));
// overwrite the deallocator function of the library, default is free
void vec_set_deallocator(void (*_deallocator)(void*));

// private functions
void _vec_priv_pushBack(void** vecPtr, void* value);
void _vec_priv_pushFront(void** vecPtr, void* value);
void _vec_priv_popBack(void** vecPtr, void* buff);
void _vec_priv_popFront(void** vecPtr, void* buff);
void* _vec_priv_slice(void* vec, size_t start, size_t end);
void _vec_priv_insert(void** vecPtr, size_t index, void* value);
void _vec_priv_remove(void** vecPtr, size_t index, void* buff);
void _vec_priv_clear(void** vecPtr);
void _vec_debug_print(void* vec, FILE* stream); // write informations about the array to the given stream, for debug purpose

#endif