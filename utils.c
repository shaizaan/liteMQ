/**
 * @file utils.c
 * @brief Implements utility functions for the liteMQ project.
 * @author Mohammed Uddin
 */

#include <fcntl.h>
#include <stdio.h>
#include "utils.h"

/**
 * @brief Sets a given file descriptor to non-blocking mode.
 *
 * This function retrieves the current flags of the file descriptor and adds the O_NONBLOCK flag.
 * If any fcntl operation fails, it prints an error message to stderr.
 *
 * @param fd The file descriptor to set to non-blocking.
 */
void set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL)");
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL)");
    }
}
