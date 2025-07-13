/**
 * @file test_message_parsing.c
 * @brief Unit tests for message parsing logic (SUB and PUB commands).
 * @author Mohammed Uddin
 */

#include <stdio.h>
#include <string.h>
#include "minunit.h"

/**
 * @brief Mock client structure for testing purposes.
 * Simulates a subset of the `client_t` structure used in the server.
 */
typedef struct {
    int fd;                 ///< Mock file descriptor.
    int type;               ///< Mock client type (0 for UNKNOWN, 1 for SUBSCRIBER, -1 for error).
    char topic[50];         ///< Mock topic string.
} mock_client_t;

/**
 * @brief Mock implementation of handle_client_data for parsing tests.
 *
 * This function simulates the parsing logic of the real `handle_client_data`
 * without performing actual socket operations or persistence.
 *
 * @param buffer The input buffer containing the command string.
 * @param client Pointer to the mock_client_t structure to update.
 */
void mock_handle_client_data(const char *buffer, mock_client_t *client) {
    // Simulate the relevant part of handle_client_data for parsing
    if (client->type == 0) { // UNKNOWN
        if (strncmp(buffer, "SUB ", 4) == 0) {
            char *topic_start = (char *)buffer + 4;
            char *newline_pos = strchr(topic_start, '\n');
            size_t topic_len;

            if (newline_pos) {
                topic_len = newline_pos - topic_start;
            }

            else {
                topic_len = strlen(topic_start);
            }

            if (topic_len > 0 && topic_len < 50) {
                client->type = 1; // SUBSCRIBER
                strncpy(client->topic, topic_start, topic_len);
                client->topic[topic_len] = '\0';
            }

            else {
                client->type = -1; // Indicate error
            }
        }
    }

    if (strncmp(buffer, "PUB ", 4) == 0) {
        char *msg_start = strchr(buffer, '\n');
        if (msg_start) {
            char *topic_start = (char *)buffer + 4;
            int topic_len = msg_start - topic_start;
            if (topic_len > 0 && topic_len < 50) {
                strncpy(client->topic, topic_start, topic_len);
                client->topic[topic_len] = '\0';
                // For PUB, we just care about topic extraction for this test
            }

            else {
                client->type = -1; // Indicate error
            }
        }

        else {
            client->type = -1; // Indicate error
        }
    }
}

/**
 * @brief Tests the parsing of SUB commands.
 *
 * Verifies correct extraction of topic from SUB commands, including cases
 * with and without a newline, and malformed (too long) topics.
 *
 * @return char* NULL if the test passes, otherwise an error message.
 */
char * test_sub_command_parsing() {
    mock_client_t client = { .fd = 1, .type = 0, .topic = "" };
    mock_handle_client_data("SUB my_topic\n", &client);
    mu_assert("test_sub_command_parsing: Client type should be SUBSCRIBER", client.type == 1);
    mu_assert("test_sub_command_parsing: Topic should be 'my_topic'", strcmp(client.topic, "my_topic") == 0);

    // Test with no newline
    client = (mock_client_t){ .fd = 1, .type = 0, .topic = "" };
    mock_handle_client_data("SUB another_topic", &client);
    mu_assert("test_sub_command_parsing: Client type should be SUBSCRIBER (no newline)", client.type == 1);
    mu_assert("test_sub_command_parsing: Topic should be 'another_topic' (no newline)", strcmp(client.topic, "another_topic") == 0);

    // Test malformed SUB (too long topic)
    client = (mock_client_t){ .fd = 1, .type = 0, .topic = "" };
    char long_topic_sub[100];
    memset(long_topic_sub, 'A', sizeof(long_topic_sub) - 1);
    long_topic_sub[sizeof(long_topic_sub) - 1] = '\0';
    char malformed_sub[200];
    snprintf(malformed_sub, sizeof(malformed_sub), "SUB %s", long_topic_sub);
    mock_handle_client_data(malformed_sub, &client);
    mu_assert("test_sub_command_parsing: Malformed SUB (long topic) should result in error", client.type == -1);

    return 0;
}

/**
 * @brief Tests the parsing of PUB commands.
 *
 * Verifies correct extraction of topic from PUB commands, including cases
 * with and without a newline, and malformed (no newline) messages.
 *
 * @return char* NULL if the test passes, otherwise an error message.
 */
char * test_pub_command_parsing() {
    mock_client_t client = { .fd = 1, .type = 0, .topic = "" };
    mock_handle_client_data("PUB my_pub_topic\nHello World!", &client);
    // For PUB, the client type remains UNKNOWN as it's a one-off message
    mu_assert("test_pub_command_parsing: Topic should be 'my_pub_topic'", strcmp(client.topic, "my_pub_topic") == 0);

    // Test malformed PUB (no newline)
    client = (mock_client_t){ .fd = 1, .type = 0, .topic = "" };
    mock_handle_client_data("PUB no_newline_topic", &client);
    mu_assert("test_pub_command_parsing: Malformed PUB (no newline) should result in error", client.type == -1);

    return 0;
}

/**
 * @brief Aggregates and runs all message parsing tests.
 *
 * @return char* NULL if all tests pass, otherwise an error message from a failed test.
 */
char * all_message_parsing_tests() {
    mu_run_test(test_sub_command_parsing);
    mu_run_test(test_pub_command_parsing);
    return 0;
}
