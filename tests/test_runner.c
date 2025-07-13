/**
 * @file test_runner.c
 * @brief Main test runner for the liteMQ unit tests.
 * @author Mohammed Uddin
 */

#include <stdio.h>
#include "minunit.h"

// Declare external test suite functions
extern char * test_set_non_blocking();
extern char * all_message_parsing_tests();
extern char * all_persistence_tests();

/**
 * @brief Global counter for the number of tests run.
 */
int tests_run = 0;

/**
 * @brief Runs all defined unit tests.
 *
 * This function calls each test suite and aggregates their results.
 *
 * @return char* NULL if all tests pass, otherwise a string indicating the failed test.
 */
static char * all_tests() {
    mu_run_test(test_set_non_blocking);
    mu_run_test(all_message_parsing_tests);
    mu_run_test(all_persistence_tests);
    return 0;
}

/**
 * @brief Main function to execute the unit tests.
 *
 * Prints a summary of the test results.
 *
 * @param argc Argument count (unused).
 * @param argv Argument vector (unused).
 * @return int Returns 0 if all tests pass, 1 if any test fails.
 */
int main(int argc, char **argv) {
    (void)argc; // Suppress unused parameter warning
    (void)argv; // Suppress unused parameter warning
    printf("\n--- RUNNING ALL TESTS ---\n");

    char *result = all_tests();
    if (result != 0) {
        printf("\n%s\n", result);
    } else {
        printf("\nALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}

