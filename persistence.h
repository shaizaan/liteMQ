/**
 * @file persistence.h
 * @brief Declares functions and types related to message persistence in liteMQ.
 * @author Mohammed Uddin
 */

#ifndef LITEMQ_PERSISTENCE_H
#define LITEMQ_PERSISTENCE_H

#include <stdio.h>
#include <time.h>

#define LOG_DIR "logs"

/**
 * @brief Defines the different modes of message persistence.
 */
typedef enum {
    PERSIST_NONE,   ///< No message persistence.
    PERSIST_ALL,    ///< All messages are persisted indefinitely.
    PERSIST_TIMED   ///< Messages are persisted for a specified duration.
} persistence_mode_t;

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
void persist_message(const char *topic, const char *message, persistence_mode_t p_mode);

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
void send_persisted_messages(int fd, const char *topic, persistence_mode_t p_mode, int p_duration);

#endif // LITEMQ_PERSISTENCE_H
