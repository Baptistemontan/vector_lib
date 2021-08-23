#include "test.h"

#define TESTSIZE (size_t)100

typedef int (*subtest_func_t)(size_t);
typedef size_t (*test_func_t)(size_t, size_t*);

typedef struct {
    int a;
    float b;
    char c;
} test_struct_t;

VEC_DEF_ALL(int, int)
VEC_DEF_ALL(test_struct_t, test_struct)

#define PUSH_CASE 2

void test_all(void)
{
    test_func_t test_funcs[] = {
        test_vec_create,
        test_vec_push_back,
        test_vec_push_front,
        test_vec_pop_back,
        test_vec_pop_front,
        test_vec_customStruct
    };
    size_t test_size = sizeof(test_funcs) / sizeof(test_funcs[0]);
    size_t passed = 0;
    size_t total = 0;
    size_t buff;
    printf("\n\nSTARTING TEST FOR VECTOR LIB\n\n");
    for (size_t i = 0; i < test_size; i++) {
        passed += test_funcs[i](TESTSIZE, &buff);
        total += buff;
    }
    printf("\n\nTOTAL TEST: %zu, TEST PASSED: %zu\n", total, passed);
    printf("\n\nTEST FOR VECTOR LIB DONE\n\n");
}

size_t test_func(subtest_func_t tests[], size_t size, size_t testSize) {
    size_t passed = 0;
    int res;
    for(int i = 0; i < size; i++) {
        printf("TEST %d\n", i + 1);
        res = tests[i](testSize);
        printf("pushBack test %d: %s\n", i + 1, res ? "passed" : "failed");
        passed += res;
    }
    return passed;
}

static int test_vec_create_1(size_t size) {
    int* v = vec_create_int(size);
    int res = vec_size(v) == size;
    vec_free(v);
    return res;
}

size_t test_vec_create(size_t testSize, size_t* testCase) {
    subtest_func_t tests[] = {
        test_vec_create_1
    };
    *testCase = sizeof(tests) / sizeof(subtest_func_t);
    printf("\n\nTESTING vec_create()\n");
    return test_func(tests, *testCase, testSize);
}

// check if after pushing to back TESTSIZE elements, the vector contains them in order
static int test_vec_push_back_1(size_t testSize) {
    int* v = vec_create_int(0);
    for(int i = 0; i < testSize; i++) {
        vec_pushBack_int(&v, i);
    }
    int res = 1;
    for(int i = 0; i < testSize; i++) {
        if(v[i] != i) {
            res = 0;
            break;
        }
    }
    vec_free(v);
    return res;
}

// check if after pushing to back TESTSIZE elements, the vector is of size TESTSIZE
static int test_vec_push_back_2(size_t testSize) {
    int* v = vec_create_int(0);
    for(int i = 0; i < testSize; i++) {
        vec_pushBack_int(&v, i);
    }
    int res = vec_size(v) == testSize;
    vec_free(v);
    return res;
}


// check all subfunctions for pushback test
size_t test_vec_push_back(size_t testSize, size_t *testCase)
{
    subtest_func_t tests[] = {
        test_vec_push_back_1,
        test_vec_push_back_2
    };
    *testCase = sizeof(tests) / sizeof(subtest_func_t);
    printf("\n\nTESTING vec_pushBack()\n");
    return test_func(tests, *testCase, testSize);
}

// check if after pushing to front TESTSIZE elements, the vector contains them in order
static int test_vec_push_front_1(size_t testSize) {
    int* v = vec_create_int(0);
    for(int i = 0; i < testSize; i++) {
        vec_pushFront_int(&v, i);
    }
    int res = 1;
    for(int i = 0; i < testSize; i++) {
        if(v[i] != testSize - i - 1) {
            res = 0;
            break;
        }
    }
    vec_free(v);
    return res;
}

// check if after pushing to front TESTSIZE elements, the vector is of size TESTSIZE
static int test_vec_push_front_2(size_t testSize) {
    int* v = vec_create_int(0);
    for(int i = 0; i < testSize; i++) {
        vec_pushFront_int(&v, i);
    }
    int res = vec_size(v) == testSize;
    vec_free(v);
    return res;
}


// check all subfunctions for pushFront test
size_t test_vec_push_front(size_t testSize, size_t *testCase)
{
    subtest_func_t tests[] = {
        test_vec_push_front_1,
        test_vec_push_front_2
    };
    *testCase = sizeof(tests) / sizeof(subtest_func_t);
    printf("\n\nTESTING vec_pushFront()\n");
    return test_func(tests, *testCase, testSize);
}

static int test_vec_pop_back_1(size_t testSize) {
    int* v = vec_create_int(0);
    for(int i = 0; i < testSize; i++) {
        vec_pushBack_int(&v, i);
    }
    int res = 1;
    for(int i = 0; i < testSize; i++) {
        if(vec_popBack_int(&v) != testSize - i - 1) {
            res = 0;
            break;
        }
    }
    vec_free(v);
    return res;
}

static int test_vec_pop_back_2(size_t testSize) {
    int* v = vec_create_int(0);
    for(int i = 0; i < testSize; i++) {
        vec_pushBack_int(&v, i);
    }
    for(int i = 0; i < testSize; i++) {
        vec_popBack_int(&v);
    }
    int res = vec_size(v) == 0;
    vec_free(v);
    return res;
}

size_t test_vec_pop_back(size_t testSize, size_t *testCase)
{
    subtest_func_t tests[] = {
        test_vec_pop_back_1,
        test_vec_pop_back_2
    };
    *testCase = sizeof(tests) / sizeof(subtest_func_t);
    printf("\n\nTESTING vec_popBack()\n\n");
    return test_func(tests, *testCase, testSize);
}

static int test_vec_pop_front_1(size_t testSize) {
    int* v = vec_create_int(0);
    for(int i = 0; i < testSize; i++) {
        vec_pushFront_int(&v, i);
    }
    int res = 1;
    for(int i = 0; i < testSize; i++) {
        if(vec_popFront_int(&v) != testSize - i - 1) {
            res = 0;
            break;
        }
    }
    vec_free(v);
    return res;
}

static int test_vec_pop_front_2(size_t testSize) {
    int* v = vec_create_int(0);
    for(int i = 0; i < testSize; i++) {
        vec_pushFront_int(&v, i);
    }
    for(int i = 0; i < testSize; i++) {
        vec_popFront_int(&v);
    }
    int res = vec_size(v) == 0;
    vec_free(v);
    return res;
}

size_t test_vec_pop_front(size_t testSize, size_t *testCase)
{
    subtest_func_t tests[] = {
        test_vec_pop_front_1,
        test_vec_pop_front_2
    };
    *testCase = sizeof(tests) / sizeof(subtest_func_t);
    printf("\n\nTESTING vec_popFront()\n\n");
    return test_func(tests, *testCase, testSize);
}

static int test_vec_customStruct_1(size_t testSize) {
    test_struct_t* v = vec_create_test_struct(0);
    test_struct_t testStruct;
    testStruct.c = 'a';

    for(int i = 0; i < testSize; i++) {
        testStruct.a = i;
        testStruct.b = i * 2.0;
        vec_pushBack_test_struct(&v, testStruct);
    }
    int res = 1;
    for(int i = 0; i < testSize; i++) {
        testStruct = vec_popFront_test_struct(&v);
        // printf("%d %f %c\n", testStruct.a, testStruct.b, testStruct.c);
        if(testStruct.a != i || testStruct.b != (float)(i * 2.0) || testStruct.c != 'a') {
            res = 0;
            break;
        }
    }
    vec_free(v);
    return res;
}

size_t test_vec_customStruct(size_t testSize, size_t *testCase)
{
    subtest_func_t tests[] = {
        test_vec_customStruct_1
    };
    *testCase = sizeof(tests) / sizeof(subtest_func_t);
    printf("\n\nTESTING vec with customs types\n\n");
    return test_func(tests, *testCase, testSize);
}


