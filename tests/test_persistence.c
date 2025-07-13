/**
 * @file test_persistence.c
 * @brief Unit tests for the message persistence logic.
 * @author Mohammed Uddin
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h> // For errno and perror
#include "minunit.h"
#include "../persistence.h" // Include the persistence functions

// Mock definitions for server functions and types
// #define LOG_DIR "test_logs" // Now defined in persistence.h
#define BUFFER_SIZE 1024

/**
 * @brief Helper function to create a dummy log directory for tests.
 * Creates the directory specified by LOG_DIR if it doesn't already exist.
 */
void setup_log_dir() {
    if (mkdir(LOG_DIR, 0755) == -1) {
        if (errno != EEXIST) { // Ignore if directory already exists
            perror("mkdir failed in setup_log_dir");
        }
    }
}

/**
 * @brief Helper function to clean up the dummy log directory and its contents.
 * Removes the directory specified by LOG_DIR and all its contents.
 */
void teardown_log_dir() {
    char command[256];
    snprintf(command, sizeof(command), "rm -rf %s", LOG_DIR);
    if (system(command) == -1) {
        perror("system(rm -rf) failed in teardown_log_dir");
    }
}

// --- Test Cases for persist_message ---

/**
 * @brief Tests persist_message with PERSIST_NONE mode.
 * Verifies that no log file is created when persistence is set to NONE.
 * @return char* NULL if the test passes, otherwise an error message.
 */
char * test_persist_message_none() {
    setup_log_dir();
    persist_message("topic_none", "message_none\n", PERSIST_NONE);
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/topic_none.log", LOG_DIR);
    FILE *fp = fopen(filepath, "r");
    mu_assert("test_persist_message_none: Log file should not exist", fp == NULL);
    teardown_log_dir();
    return 0;
}

/**
 * @brief Tests persist_message with PERSIST_ALL mode.
 * Verifies that messages are correctly appended to the log file.
 * @return char* NULL if the test passes, otherwise an error message.
 */
char * test_persist_message_all() {
    setup_log_dir();
    persist_message("topic_all", "message_all_1\n", PERSIST_ALL);
    persist_message("topic_all", "message_all_2\n", PERSIST_ALL);

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/topic_all.log", LOG_DIR);
    printf("Attempting to open log file: %s\n", filepath); // Debug print
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        perror("fopen failed in test_persist_message_all"); // Debug print
    }
    mu_assert("test_persist_message_all: Log file should exist", fp != NULL);

    char buffer[BUFFER_SIZE];
    fgets(buffer, sizeof(buffer), fp);
    mu_assert("test_persist_message_all: First message incorrect", strcmp(buffer, "message_all_1\n") == 0);
    fgets(buffer, sizeof(buffer), fp);
    mu_assert("test_persist_message_all: Second message incorrect", strcmp(buffer, "message_all_2\n") == 0);

    fclose(fp);
    teardown_log_dir();
    return 0;
}

/**
 * @brief Tests persist_message with PERSIST_TIMED mode.
 * Verifies that messages are appended with a timestamp.
 * @return char* NULL if the test passes, otherwise an error message.
 */
char * test_persist_message_timed() {
    setup_log_dir();
    persist_message("topic_timed", "message_timed_1\n", PERSIST_TIMED);
    sleep(1); // Ensure different timestamps
    persist_message("topic_timed", "message_timed_2\n", PERSIST_TIMED);

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/topic_timed.log", LOG_DIR);
    FILE *fp = fopen(filepath, "r");
    mu_assert("test_persist_message_timed: Log file should exist", fp != NULL);

    char buffer[BUFFER_SIZE];
    long timestamp1, timestamp2;
    char msg1[BUFFER_SIZE], msg2[BUFFER_SIZE];

    fgets(buffer, sizeof(buffer), fp);
    sscanf(buffer, "%ld %s", &timestamp1, msg1);
    mu_assert("test_persist_message_timed: First message content incorrect", strcmp(msg1, "message_timed_1") == 0);

    fgets(buffer, sizeof(buffer), fp);
    sscanf(buffer, "%ld %s", &timestamp2, msg2);
    mu_assert("test_persist_message_timed: Second message content incorrect", strcmp(msg2, "message_timed_2") == 0);
    mu_assert("test_persist_message_timed: Timestamps should be different", timestamp1 != timestamp2);

    fclose(fp);
    teardown_log_dir();
    return 0;
}

// --- Test Cases for send_persisted_messages ---

// Mock write function for send_persisted_messages
static char mock_write_buffer[BUFFER_SIZE * 5]; // Large enough for multiple messages
static int mock_write_pos = 0;

/**
 * @brief Mock implementation of the write system call for testing.
 * Writes data to a static buffer instead of a file descriptor.
 *
 * @param fd The file descriptor (unused in mock).
 * @param buf The buffer containing data to write.
 * @param count The number of bytes to write.
 * @return ssize_t The number of bytes written, or -1 on error.
 */
ssize_t __wrap_write(int fd, const void *buf, size_t count) {
    (void)fd; // Unused
    if (mock_write_pos + count < sizeof(mock_write_buffer)) {
        memcpy(mock_write_buffer + mock_write_pos, buf, count);
        mock_write_pos += count;
        mock_write_buffer[mock_write_pos] = '\0'; // Null-terminate for easy inspection
        return count;
    }
    return -1; // Buffer overflow
}

/**
 * @brief Tests send_persisted_messages with PERSIST_ALL mode.
 * Verifies that all persisted messages are sent to the mock write buffer.
 * @return char* NULL if the test passes, otherwise an error message.
 */
char * test_send_persisted_messages_all() {
    setup_log_dir();
    persist_message("topic_send_all", "msg1\n", PERSIST_ALL);
    persist_message("topic_send_all", "msg2\n", PERSIST_ALL);

    // Debug: Read content of the log file after persistence
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/topic_send_all.log", LOG_DIR);
    FILE *fp_debug = fopen(filepath, "r");
    if (fp_debug) {
        char debug_buffer[BUFFER_SIZE * 2];
        size_t bytes_read = fread(debug_buffer, 1, sizeof(debug_buffer) - 1, fp_debug);
        debug_buffer[bytes_read] = '\0';
        printf("\nDebug: Content of %s after persist_message:\n---\n%s---\n", filepath, debug_buffer);
        fclose(fp_debug);
    } else {
        perror("Debug: Failed to open log file for reading");
    }

    mock_write_pos = 0;
    memset(mock_write_buffer, 0, sizeof(mock_write_buffer));

    send_persisted_messages(1, "topic_send_all", PERSIST_ALL, 0);

    printf("\nExpected: \"%s\"\n", "msg1\nmsg2\n");
    printf("Actual:   \"%s\"\n", mock_write_buffer);

    mu_assert("test_send_persisted_messages_all: Should send all messages",
              strcmp(mock_write_buffer, "msg1\nmsg2\n") == 0);

    teardown_log_dir();
    return 0;
}

/**
 * @brief Tests send_persisted_messages with PERSIST_TIMED mode for valid messages.
 * Verifies that valid (non-expired) messages are sent to the mock write buffer.
 * @return char* NULL if the test passes, otherwise an error message.
 */
char * test_send_persisted_messages_timed_valid() {
    setup_log_dir();
    // Write a message that is still valid
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/topic_send_timed_valid.log", LOG_DIR);
    FILE *fp = fopen(filepath, "w");
    fprintf(fp, "%ld msg_valid\n", time(NULL));
    fclose(fp);

    mock_write_pos = 0;
    memset(mock_write_buffer, 0, sizeof(mock_write_buffer));

    send_persisted_messages(1, "topic_send_timed_valid", PERSIST_TIMED, 10);

    mu_assert("test_send_persisted_messages_timed_valid: Should send valid message",
              strcmp(mock_write_buffer, "msg_valid\n") == 0);

    // Verify log file content after cleanup (should still contain the valid message)
    fp = fopen(filepath, "r");
    mu_assert("test_send_persisted_messages_timed_valid: Log file should exist after cleanup", fp != NULL);
    char buffer[BUFFER_SIZE];
    fgets(buffer, sizeof(buffer), fp);
    mu_assert("test_send_persisted_messages_timed_valid: Log file content incorrect after cleanup", strstr(buffer, "msg_valid\n") != NULL);
    fclose(fp);

    teardown_log_dir();
    return 0;
}

/**
 * @brief Tests send_persisted_messages with PERSIST_TIMED mode for expired messages.
 * Verifies that expired messages are not sent and are removed from the log file.
 * @return char* NULL if the test passes, otherwise an error message.
 */
char * test_send_persisted_messages_timed_expired() {
    setup_log_dir();
    // Write a message that is expired
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/topic_send_timed_expired.log", LOG_DIR);
    FILE *fp = fopen(filepath, "w");
    fprintf(fp, "%ld msg_expired\n", time(NULL) - 100);
    fclose(fp);

    mock_write_pos = 0;
    memset(mock_write_buffer, 0, sizeof(mock_write_buffer));

    send_persisted_messages(1, "topic_send_timed_expired", PERSIST_TIMED, 10);

    mu_assert("test_send_persisted_messages_timed_expired: Should not send expired message",
              strlen(mock_write_buffer) == 0);

    // Verify log file content after cleanup (should be empty)
    fp = fopen(filepath, "r");
    mu_assert("test_send_persisted_messages_timed_expired: Log file should exist after cleanup", fp != NULL);
    char buffer[BUFFER_SIZE];
    mu_assert("test_send_persisted_messages_timed_expired: Log file should be empty after cleanup",
              fgets(buffer, sizeof(buffer), fp) == NULL);
    fclose(fp);

    teardown_log_dir();
    return 0;
}

/**
 * @brief Aggregates and runs all persistence tests.
 *
 * @return char* NULL if all tests pass, otherwise an error message from a failed test.
 */
char * all_persistence_tests() {
    mu_run_test(test_persist_message_none);
    mu_run_test(test_persist_message_all);
    mu_run_test(test_persist_message_timed);
    mu_run_test(test_send_persisted_messages_all);
    mu_run_test(test_send_persisted_messages_timed_valid);
    mu_run_test(test_send_persisted_messages_timed_expired);
    return 0;
}
