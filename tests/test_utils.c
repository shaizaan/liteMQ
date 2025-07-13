/**
 * @file test_utils.c
 * @brief Unit tests for utility functions (e.g., non-blocking socket setup).
 * @author Mohammed Uddin
 */

#define _POSIX_C_SOURCE 200809L // For fileno
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h> // For fileno
#include "minunit.h"
#include "../utils.h" // Include the utility functions

/**
 * @brief Tests the set_non_blocking function.
 *
 * Verifies that a file descriptor is correctly set to non-blocking mode.
 * It uses stdin for testing purposes and resets its flags afterwards.
 *
 * @return char* NULL if the test passes, otherwise an error message.
 */
char * test_set_non_blocking() {
    int fd = fileno(stdin); // Use stdin for testing, it's usually blocking
    int flags_before = fcntl(fd, F_GETFL, 0);
    mu_assert("Initial flags should not have O_NONBLOCK", !(flags_before & O_NONBLOCK));

    set_non_blocking(fd);

    int flags_after = fcntl(fd, F_GETFL, 0);
    mu_assert("Flags should have O_NONBLOCK after set_non_blocking", (flags_after & O_NONBLOCK));

    // Reset flags to original state to avoid side effects on stdin
    fcntl(fd, F_SETFL, flags_before);

    return 0;
}
