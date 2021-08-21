#ifndef HEAD_TEST_H
#define HEAD_TEST_H

#include <stdio.h>
#include <stdlib.h>

size_t test_vec_create(size_t testSize, size_t* testCase);
size_t test_vec_push_back(size_t testSize, size_t* testCase);
size_t test_vec_push_front(size_t testSize, size_t* testCase);
size_t test_vec_pop_back(size_t testSize, size_t* testCase);
size_t test_vec_pop_front(size_t testSize, size_t* testCase);
size_t test_vec_customStruct(size_t testSize, size_t *testCase);
void test_all(void);

#endif // HEAD_TEST_H