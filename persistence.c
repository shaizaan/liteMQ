/**
 * @file persistence.c
 * @brief Implements functions for message persistence in liteMQ.
 * @author Mohammed Uddin
 */

#include "persistence.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

/**
 * @brief Persists a message to a topic's log file based on the persistence mode.
 *
 * If `PERSIST_NONE`, the message is not saved. If `PERSIST_ALL`, the message is appended
 * to the topic's log file. If `PERSIST_TIMED`, the message is appended with a timestamp.
 *
 * @param topic The topic of the message.
 * @param message The content of the message.
 * @param p_mode The persistence mode to use.
 */
void persist_message(const char *topic, const char *message, persistence_mode_t p_mode) {
    if (p_mode == PERSIST_NONE) return;

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s.log", LOG_DIR, topic);

    FILE *fp = fopen(filepath, "a");
    if (fp == NULL) {
        perror("fopen for persistence");
        return;
    }

    if (p_mode == PERSIST_TIMED) {
        fprintf(fp, "%ld %s", time(NULL), message);
    } else { // PERSIST_ALL
        fprintf(fp, "%s", message);
    }
    fflush(fp);
    fclose(fp);
}

/**
 * @brief Sends persisted messages for a given topic to a subscriber.
 *
 * For `PERSIST_ALL` mode, all messages in the log file are sent. For `PERSIST_TIMED` mode,
 * only messages that have not expired are sent, and expired messages are removed from the log file.
 *
 * @param fd The file descriptor of the subscriber to send messages to.
 * @param topic The topic for which to send persisted messages.
 * @param p_mode The persistence mode of the server.
 * @param p_duration The duration in seconds for timed persistence.
 */
void send_persisted_messages(int fd, const char *topic, persistence_mode_t p_mode, int p_duration) {
    if (p_mode == PERSIST_NONE) return;

    char filepath[256];
    char temp_filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s.log", LOG_DIR, topic);
    snprintf(temp_filepath, sizeof(temp_filepath), "%s/%s.log.tmp", LOG_DIR, topic);

    FILE *fp_read = fopen(filepath, "r");
    if (fp_read == NULL) {
        // No log file yet, which is fine
        return;
    }

    FILE *fp_write = NULL;
    if (p_mode == PERSIST_TIMED) {
        fp_write = fopen(temp_filepath, "w");
        if (fp_write == NULL) {
            perror("fopen for temp persistence file");
            fclose(fp_read);
            return;
        }
    }

    char line[BUFFER_SIZE];
    time_t now = time(NULL);

    while (fgets(line, sizeof(line), fp_read)) {
        if (p_mode == PERSIST_TIMED) {
            time_t msg_time;
            char *msg_content_start;

            // Parse timestamp and message content
            msg_time = strtol(line, &msg_content_start, 10);
            // Skip space after timestamp
            if (*msg_content_start == ' ') {
                msg_content_start++;
            }

            if (now - msg_time <= p_duration) {
                // Message is still valid, send it and write to temp file
                if (write(fd, msg_content_start, strlen(msg_content_start)) < 0) {
                    perror("write persisted (timed)");
                }
                fprintf(fp_write, "%ld %s", msg_time, msg_content_start);
            } else {
                // Message expired, do not send or write to temp file
            }
        } else { // PERSIST_ALL
            // Send message
            if (write(fd, line, strlen(line)) < 0) {
                perror("write persisted (all)");
            }
            // For PERSIST_ALL, we don't rewrite the file here, as it's append-only
        }
    }
    fclose(fp_read);

    if (p_mode == PERSIST_TIMED) {
        fclose(fp_write);
        // Replace original file with temp file
        if (rename(temp_filepath, filepath) != 0) {
            perror("rename temp file");
        }
    }
}
