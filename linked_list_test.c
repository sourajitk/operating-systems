#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "linked_list.h"

int tests_run = 0;
#define mu_str_(text) #text
#define mu_str(text) mu_str_(text)
#define mu_assert(message, test) do { if (!(test)) return "FAILURE: See " __FILE__ " Line " mu_str(__LINE__) ": " message; } while (0)
#define mu_run_test(test) do { char *message = test(); tests_run++;     \
                               if (message) return message; } while (0)

static int string_equal(const char* str1, const char* str2)
{
    if ((str1 == NULL) && (str2 == NULL)) {
        return 1;
    }
    if ((str1 == NULL) || (str2 == NULL)) {
        return 0;
    }
    return (strcmp(str1, str2) == 0);
}

typedef struct {
    int value;
} data_item_t;

// Compares data1 and data2 and returns
// -1 if data1 goes before data2
// 0 if data1 is equivalent to data2
// 1 if data1 goes after data2
int test_compare_function(void* data1, void* data2)
{
    data_item_t* dataItem1 = (data_item_t*) data1;
    data_item_t* dataItem2 = (data_item_t*) data2;
    if (dataItem1->value == dataItem2->value) {
        return 0;
    } else if (dataItem1->value > dataItem2->value) {
        return 1;
    }
    return -1;
}

char* test_list_create()
{
    list_t* new_list1 = list_create(NULL);
    mu_assert("test_list_create: Testing if list is not NULL", new_list1 != NULL);
    mu_assert("test_list_create: Testing if head is NULL", list_head(new_list1) == list_end(new_list1));
    mu_assert("test_list_create: Testing if tail is NULL", list_tail(new_list1) == list_end(new_list1));
    mu_assert("test_list_create: Testing if list count is zero ", list_count(new_list1) == 0);
    list_destroy(new_list1);
    return NULL;
}

char* test_list_insert()
{
    /*
     * After the first insert, head and tail should be the same
     */
    list_t* new_list = list_create(NULL);
    data_item_t data[5];
    data[0].value = 1;
    list_insert(new_list, &data[0]);
    mu_assert("test_list_insert: [NoCompareFn] Head and tail should be the same at this point", list_head(new_list) == list_tail(new_list));

    /* 
     * Checking if nodes are being added at the head when no compare function is supplied
     */
    for (int i = 1; i < 5; i++) {
        data[i].value = i + 1;
        list_insert(new_list, &data[i]);
        mu_assert("test_list_insert: [NoCompareFn] Testing if node count is correct", list_count(new_list) == i + 1);
    }

    list_node_t* current_node = list_head(new_list);
    mu_assert("test_list_insert: [NoCompareFn] Testing if node has correct value, it should have 5", ((data_item_t*)list_data(current_node))->value == 5);
    current_node = list_next(current_node);
    mu_assert("test_list_insert: [NoCompareFn] Testing if node has correct value, it should have 4", ((data_item_t*)list_data(current_node))->value == 4);
    current_node = list_next(current_node);
    mu_assert("test_list_insert: [NoCompareFn] Testing if node has correct value, it should have 3", ((data_item_t*)list_data(current_node))->value == 3);
    current_node = list_next(current_node);
    mu_assert("test_list_insert: [NoCompareFn] Testing if node has correct value, it should have 2", ((data_item_t*)list_data(current_node))->value == 2);
    current_node = list_next(current_node);
    mu_assert("test_list_insert: [NoCompareFn] Testing if node has correct value, it should have 1", ((data_item_t*)list_data(current_node))->value == 1);
    mu_assert("test_list_insert: [NoCompareFn] Testing if node is at the tail", current_node == list_tail(new_list));

    /* 
     * When compare function is not supplied, checking values from tail to head, using prev
     */
    current_node = list_tail(new_list);
    mu_assert("test_list_insert: [NoCompareFn] Testing if node has correct value, it should have 1", ((data_item_t*)list_data(current_node))->value == 1);
    current_node = list_prev(current_node);
    mu_assert("test_list_insert: [NoCompareFn] Testing if node has correct value, it should have 2", ((data_item_t*)list_data(current_node))->value == 2);
    current_node = list_prev(current_node);
    mu_assert("test_list_insert: [NoCompareFn] Testing if node has correct value, it should have 3", ((data_item_t*)list_data(current_node))->value == 3);
    current_node = list_prev(current_node);
    mu_assert("test_list_insert: [NoCompareFn] Testing if node has correct value, it should have 4", ((data_item_t*)list_data(current_node))->value == 4);
    current_node = list_prev(current_node);
    mu_assert("test_list_insert: [NoCompareFn] Testing if node has correct value, it should have 5", ((data_item_t*)list_data(current_node))->value == 5);
    mu_assert("test_list_insert: [NoCompareFn] Testing if node is at the head", current_node == list_head(new_list));

    list_destroy(new_list);

    /* 
     * When compare function is supplied, checking if nodes values are properly added, adding values in ascending order
     */
    list_t* new_list1 = list_create(test_compare_function);
    data_item_t data1[5];

    for (int i = 0; i < 5; i++) {
        data1[i].value = i + 1;
        list_insert(new_list1, &data1[i]);
        mu_assert("test_list_insert: [WithCompareFn1], Testing if node count is correct", list_count(new_list1) == i + 1);
    }

    current_node = list_head(new_list1);
    mu_assert("test_list_insert: [WithCompareFn1] Testing if node has correct value, it should have 1", ((data_item_t*)list_data(current_node))->value == 1);
    current_node = list_next(current_node);
    mu_assert("test_list_insert: [WithCompareFn1] Testing if node has correct value, it should have 2", ((data_item_t*)list_data(current_node))->value == 2);
    current_node = list_next(current_node);
    mu_assert("test_list_insert: [WithCompareFn1] Testing if node has correct value, it should have 3", ((data_item_t*)list_data(current_node))->value == 3);
    current_node = list_next(current_node);
    mu_assert("test_list_insert: [WithCompareFn1] Testing if node has correct value, it should have 4", ((data_item_t*)list_data(current_node))->value == 4);
    current_node = list_next(current_node);
    mu_assert("test_list_insert: [WithCompareFn1] Testing if node has correct value, it should have 5", ((data_item_t*)list_data(current_node))->value == 5);
    mu_assert("test_list_insert: [WithCompareFn1] Testing if node is at the tail", current_node == list_tail(new_list1));

    current_node = list_tail(new_list1);
    mu_assert("test_list_insert: [WithCompareFn1] Testing if node has correct value, it should have 5", ((data_item_t*)list_data(current_node))->value == 5);
    current_node = list_prev(current_node);
    mu_assert("test_list_insert: [WithCompareFn1] Testing if node has correct value, it should have 4", ((data_item_t*)list_data(current_node))->value == 4);
    current_node = list_prev(current_node);
    mu_assert("test_list_insert: [WithCompareFn1] Testing if node has correct value, it should have 3", ((data_item_t*)list_data(current_node))->value == 3);
    current_node = list_prev(current_node);
    mu_assert("test_list_insert: [WithCompareFn1] Testing if node has correct value, it should have 2", ((data_item_t*)list_data(current_node))->value == 2);
    current_node = list_prev(current_node);
    mu_assert("test_list_insert: [WithCompareFn1] Testing if node has correct value, it should have 1", ((data_item_t*)list_data(current_node))->value == 1);
    mu_assert("test_list_insert: [WithCompareFn1] Testing if node is at the head", current_node == list_head(new_list1));

    list_destroy(new_list1);

    /* 
     * When compare function is supplied, inserting values with gaps, then adding values that go in those gaps,
     * however, values should be in an ascending order from head to tail
     */
    list_t* new_list2 = list_create(test_compare_function);
    data_item_t data2[6];
    int data_values[6] = {2, 4, 6, 1, 3, 5};

    for (int i = 0; i < 6; i++) {
        data2[i].value = data_values[i];
        list_insert(new_list2, &data2[i]);
        mu_assert("test_list_insert: [WithCompareFn2], Testing if node count is correct", list_count(new_list2) == i + 1);
    }

    current_node = list_head(new_list2);
    mu_assert("test_list_insert: [WithCompareFn2] Testing if node has correct value, it should have 1", ((data_item_t*)list_data(current_node))->value == 1);
    current_node = list_next(current_node);
    mu_assert("test_list_insert: [WithCompareFn2] Testing if node has correct value, it should have 2", ((data_item_t*)list_data(current_node))->value == 2);
    current_node = list_next(current_node);
    mu_assert("test_list_insert: [WithCompareFn2] Testing if node has correct value, it should have 3", ((data_item_t*)list_data(current_node))->value == 3);
    current_node = list_next(current_node);
    mu_assert("test_list_insert: [WithCompareFn2] Testing if node has correct value, it should have 4", ((data_item_t*)list_data(current_node))->value == 4);
    current_node = list_next(current_node);
    mu_assert("test_list_insert: [WithCompareFn2] Testing if node has correct value, it should have 5", ((data_item_t*)list_data(current_node))->value == 5);
    current_node = list_next(current_node);
    mu_assert("test_list_insert: [WithCompareFn2] Testing if node has correct value, it should have 6", ((data_item_t*)list_data(current_node))->value == 6);
    mu_assert("test_list_insert: [WithCompareFn2] Testing if node is at the tail", current_node == list_tail(new_list2));

    current_node = list_tail(new_list2);
    mu_assert("test_list_insert: [WithCompareFn2] Testing if node has correct value, it should have 6", ((data_item_t*)list_data(current_node))->value == 6);
    current_node = list_prev(current_node);
    mu_assert("test_list_insert: [WithCompareFn2] Testing if node has correct value, it should have 5", ((data_item_t*)list_data(current_node))->value == 5);
    current_node = list_prev(current_node);
    mu_assert("test_list_insert: [WithCompareFn2] Testing if node has correct value, it should have 4", ((data_item_t*)list_data(current_node))->value == 4);
    current_node = list_prev(current_node);
    mu_assert("test_list_insert: [WithCompareFn2] Testing if node has correct value, it should have 3", ((data_item_t*)list_data(current_node))->value == 3);
    current_node = list_prev(current_node);
    mu_assert("test_list_insert: [WithCompareFn2] Testing if node has correct value, it should have 2", ((data_item_t*)list_data(current_node))->value == 2);
    current_node = list_prev(current_node);
    mu_assert("test_list_insert: [WithCompareFn2] Testing if node has correct value, it should have 1", ((data_item_t*)list_data(current_node))->value == 1);
    mu_assert("test_list_insert: [WithCompareFn2] Testing if node is at the head", current_node == list_head(new_list2));

    list_destroy(new_list2);

    /* 
     * When compare function is supplied, checking if most values are the same, but one number is larger and one is smaller
     */
    list_t* new_list3 = list_create(test_compare_function);
    data_item_t data3[6];
    int data_values2[6] = {2, 2, 1, 2, 4, 2};

    for (int i = 0; i < 6; i++) {
        data3[i].value = data_values2[i];
        list_insert(new_list3, &data3[i]);
        mu_assert("test_list_insert: [WithCompareFn3], Testing if node count is correct", list_count(new_list3) == i + 1);
    }

    current_node = list_head(new_list3);
    mu_assert("test_list_insert: [WithCompareFn3] Testing if node has correct value, it should have 1", ((data_item_t*)list_data(current_node))->value == 1);
    current_node = list_next(current_node);
    mu_assert("test_list_insert: [WithCompareFn3] Testing if node has correct value, it should have 2", ((data_item_t*)list_data(current_node))->value == 2);
    current_node = list_next(current_node);
    mu_assert("test_list_insert: [WithCompareFn3] Testing if node has correct value, it should have 2", ((data_item_t*)list_data(current_node))->value == 2);
    current_node = list_next(current_node);
    mu_assert("test_list_insert: [WithCompareFn3] Testing if node has correct value, it should have 2", ((data_item_t*)list_data(current_node))->value == 2);
    current_node = list_next(current_node);
    mu_assert("test_list_insert: [WithCompareFn3] Testing if node has correct value, it should have 2", ((data_item_t*)list_data(current_node))->value == 2);
    current_node = list_next(current_node);
    mu_assert("test_list_insert: [WithCompareFn3] Testing if node has correct value, it should have 4", ((data_item_t*)list_data(current_node))->value == 4);
    mu_assert("test_list_insert: [WithCompareFn3] Testing if node is at the tail", current_node == list_tail(new_list3));

    current_node = list_tail(new_list3);
    mu_assert("test_list_insert: [WithCompareFn3] Testing if node has correct value, it should have 4", ((data_item_t*)list_data(current_node))->value == 4);
    current_node = list_prev(current_node);
    mu_assert("test_list_insert: [WithCompareFn3] Testing if node has correct value, it should have 2", ((data_item_t*)list_data(current_node))->value == 2);
    current_node = list_prev(current_node);
    mu_assert("test_list_insert: [WithCompareFn3] Testing if node has correct value, it should have 2", ((data_item_t*)list_data(current_node))->value == 2);
    current_node = list_prev(current_node);
    mu_assert("test_list_insert: [WithCompareFn3] Testing if node has correct value, it should have 2", ((data_item_t*)list_data(current_node))->value == 2);
    current_node = list_prev(current_node);
    mu_assert("test_list_insert: [WithCompareFn3] Testing if node has correct value, it should have 2", ((data_item_t*)list_data(current_node))->value == 2);
    current_node = list_prev(current_node);
    mu_assert("test_list_insert: [WithCompareFn3] Testing if node has correct value, it should have 1", ((data_item_t*)list_data(current_node))->value == 1);
    mu_assert("test_list_insert: [WithCompareFn3] Testing if node is at the tail", current_node == list_head(new_list3));

    list_destroy(new_list3);

    return NULL;
}

char* test_list_find()
{
    list_t* new_list = list_create(NULL);
    data_item_t data[6];
    data_item_t dataNotInList;
    int data_values[] = {2, 4, 6, 5, 3, 1};

    // Case 1: When list is empty, the search should return NULL
    mu_assert("test_list_find: Testing if search returns NULL if list is empty", list_find(new_list, &data[1]) == NULL);

    // Case 2: Item is in list, item is found 
    for (int i = 0; i < 6; i++) {
        data[i].value = data_values[i];
        list_insert(new_list, &data[i]);
    }
    for (int i = 0; i < 6; i++) {
        mu_assert("test_list_find: Testing if node is found if searched node exists", list_data(list_find(new_list, &data[i])) == &data[i]);
    }

    // Case 3: Item is not found 
    mu_assert("test_list_find: Testing if search returns NULL if searched node does not exist", list_find(new_list, &dataNotInList) == NULL);

    // Case 4: Item is NULL
    list_insert(new_list, NULL);
    mu_assert("test_list_find: Testing if search can find NULL data", list_find(new_list, NULL) == list_head(new_list));

    list_destroy(new_list);
    return NULL;
}

char* test_list_remove()
{
    /* 
     * Case 1: Inserted 5 nodes, now always removing at the head till we reach the tail (NULL)
     */
    list_t* new_list = list_create(NULL);
    data_item_t data[5];
    for (int i = 0; i < 5; i++) {
        data[i].value = i + 1;
        list_insert(new_list, &data[i]);
    }
    mu_assert("test_list_remove: List node count should be 5", list_count(new_list) == 5);

    list_remove(new_list, list_head(new_list));
    mu_assert("test_list_remove: Removing the list head, new head should have the value 4", ((data_item_t*)list_data(list_head(new_list)))->value == 4);
    mu_assert("test_list_remove: List node count should be 4", list_count(new_list) == 4);

    list_remove(new_list, list_head(new_list));
    mu_assert("test_list_remove: Removing the list head, new head should have the value 3", ((data_item_t*)list_data(list_head(new_list)))->value == 3);
    mu_assert("test_list_remove: List node count should be 3", list_count(new_list) == 3);

    list_remove(new_list, list_head(new_list));
    mu_assert("test_list_remove: Removing the list head, new head should have the value 2", ((data_item_t*)list_data(list_head(new_list)))->value == 2);
    mu_assert("test_list_remove: List node count should be 2", list_count(new_list) == 2);

    list_remove(new_list, list_head(new_list));
    mu_assert("test_list_remove: Removing the list head, new head should have the value 1", ((data_item_t*)list_data(list_head(new_list)))->value == 1);
    mu_assert("test_list_remove: List node count should be 1", list_count(new_list) == 1);
    mu_assert("test_list_remove: Testing if head is equal to the tail", list_head(new_list) == list_tail(new_list));

    list_remove(new_list, list_head(new_list));
    mu_assert("test_list_remove: Removing the list head, now head should be NULL", list_head(new_list) == list_end(new_list));
    mu_assert("test_list_remove: Removing the list head, now tail should be NULL", list_tail(new_list) == list_end(new_list));
    mu_assert("test_list_remove: List node count should be 0", list_count(new_list) == 0);

    list_destroy(new_list);

    /* 
     * Case 2: Inserted 5 nodes, now removing in the middle
     */
    list_t* new_list1 = list_create(NULL);
    data_item_t data1[5];
    for (int i = 0; i < 5; i++) {
        data1[i].value = i + 1;
        list_insert(new_list1, &data1[i]);
    }
    mu_assert("test_list_remove: List node count should be 5", list_count(new_list1) == 5);

    list_node_t *current_node = list_head(new_list1); // at value 5
    current_node = list_next(current_node); // at value 4
    list_remove(new_list1, current_node);
    mu_assert("test_list_remove: List node count should be 4", list_count(new_list1) == 4);

    current_node = list_head(new_list1);
    mu_assert("test_list_remove: Testing if node has correct value, it should have 5", ((data_item_t*)list_data(current_node))->value == 5);
    current_node = list_next(current_node);
    mu_assert("test_list_remove: Testing if node has correct value, it should have 3", ((data_item_t*)list_data(current_node))->value == 3);
    current_node = list_next(current_node);
    mu_assert("test_list_remove: Testing if node has correct value, it should have 2", ((data_item_t*)list_data(current_node))->value == 2);
    current_node = list_next(current_node);
    mu_assert("test_list_remove: Testing if node has correct value, it should have 1", ((data_item_t*)list_data(current_node))->value == 1);
    mu_assert("test_list_remove: Testing if node is at the tail", current_node == list_tail(new_list1));

    current_node = list_tail(new_list1);
    mu_assert("test_list_remove: Testing if node has correct value, it should have 1", ((data_item_t*)list_data(current_node))->value == 1);
    current_node = list_prev(current_node);
    mu_assert("test_list_remove: Testing if node has correct value, it should have 2", ((data_item_t*)list_data(current_node))->value == 2);
    current_node = list_prev(current_node);
    mu_assert("test_list_remove: Testing if node has correct value, it should have 3", ((data_item_t*)list_data(current_node))->value == 3);
    current_node = list_prev(current_node);
    mu_assert("test_list_remove: Testing if node has correct value, it should have 5", ((data_item_t*)list_data(current_node))->value == 5);
    mu_assert("test_list_remove: Testing if node is at the head", current_node == list_head(new_list1));

    list_destroy(new_list1);

    /* 
     * Case 3: Inserted 5 nodes, now always removing at the tail till we reach the head then NULL
     */
    list_t* new_list2 = list_create(NULL);
    data_item_t data2[5];
    for (int i = 0; i < 5; i++) {
        data2[i].value = i + 1;
        list_insert(new_list2, &data2[i]);
    }
    mu_assert("test_list_remove: List node count should be 5", list_count(new_list2) == 5);

    list_remove(new_list2, list_tail(new_list2));
    mu_assert("test_list_remove: Removing the list tail, new tail should have the value 2", ((data_item_t*)list_data(list_tail(new_list2)))->value == 2);
    mu_assert("test_list_remove: List node count should be 4", list_count(new_list2) == 4);

    list_remove(new_list2, list_tail(new_list2));
    mu_assert("test_list_remove: Removing the list tail, new tail should have the value 3", ((data_item_t*)list_data(list_tail(new_list2)))->value == 3);
    mu_assert("test_list_remove: List node count should be 3", list_count(new_list2) == 3);

    list_remove(new_list2, list_tail(new_list2));
    mu_assert("test_list_remove: Removing the list tail, new tail should have the value 4", ((data_item_t*)list_data(list_tail(new_list2)))->value == 4);
    mu_assert("test_list_remove: List node count should be 2", list_count(new_list2) == 2);

    list_remove(new_list2, list_tail(new_list2));
    mu_assert("test_list_remove: Removing the list tail, new tail should have the value 5", ((data_item_t*)list_data(list_tail(new_list2)))->value == 5);
    mu_assert("test_list_remove: List node count should be 1", list_count(new_list2) == 1);
    mu_assert("test_list_remove: Testing if tail is equal to the head", list_tail(new_list2) == list_head(new_list2));

    list_remove(new_list2, list_tail(new_list2));
    mu_assert("test_list_remove: Removing the list tail, now tail should be NULL", list_tail(new_list2) == list_end(new_list2));
    mu_assert("test_list_remove: Removing the list tail, now head should be NULL", list_head(new_list2) == list_end(new_list2));
    mu_assert("test_list_remove: List node count should be 0", list_count(new_list2) == 0);

    list_destroy(new_list2);
    return NULL;
}

typedef char* (*test_fn_t)();
typedef struct {
    char* name;
    test_fn_t test;
} test_t;

test_t tests[] = {
    {"test_list_create", test_list_create},
    {"test_list_insert", test_list_insert},
    {"test_list_find",   test_list_find},
    {"test_list_remove", test_list_remove} 
};
 
size_t num_tests = sizeof(tests)/sizeof(tests[0]);

char* single_test(test_fn_t test, size_t iters)
{
    for (size_t i = 0; i < iters; i++) {
        mu_run_test(test);
    }
    return NULL;
}

char* all_tests(size_t iters)
{
    for (size_t i = 0; i < num_tests; i++) {
        char* result = single_test(tests[i].test, iters);
        if (result != NULL) {
            return result;
        }
    }
    return NULL;
}

int main(int argc, char** argv)
{
    char* result = NULL;
    size_t iters = 1;
    if (argc == 1) {
        result = all_tests(iters);
        if (result != NULL) {
            printf("%s\n", result);
        } else {
            printf("ALL TESTS PASSED\n");
        }

        printf("Tests run: %d\n", tests_run);

        return result != NULL;
    } else if (argc == 3) {
        iters = (size_t)atoi(argv[2]);
    } else if (argc > 3) {
        printf("Wrong number of arguments, only one test is accepted at time");
    }

    result = "Did not find test";

    for (size_t i = 0; i < num_tests; i++) {
        if (string_equal(argv[1], tests[i].name)) {
            result = single_test(tests[i].test, iters);
            break;
        }
    }
    if (result) {
        printf("%s\n", result);
    }
    else {
        printf("ALL TESTS PASSED\n");
    }

    printf("Tests run: %d\n", tests_run);

    return result != NULL;
}
