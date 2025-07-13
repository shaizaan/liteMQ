/**
 * @file server.c
 * @brief Implements the liteMQ server, a lightweight pub/sub messaging broker.
 * @author Mohammed Uddin
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include "utils.h"
#include "persistence.h"

#define MAX_CLIENTS 32
#define PORT 8080
#define MAX_TOPIC_LEN 50
#define BUFFER_SIZE 1024
#define LOG_DIR "logs"

/**
 * @brief Defines the type of client connected to the server.
 */
typedef enum {
    CLIENT_TYPE_UNKNOWN,    ///< Client type is not yet determined.
    CLIENT_TYPE_SUBSCRIBER  ///< Client is a subscriber.
} client_type_t;

/**
 * @brief Structure to hold information about each connected client.
 */
typedef struct {
    int fd;                 ///< File descriptor of the client socket.
    client_type_t type;     ///< Type of the client (publisher or subscriber).
    char topic[MAX_TOPIC_LEN]; ///< The topic the client is subscribed to (if applicable).
} client_t;

// --- Function Prototypes ---
void handle_new_connection(int server_fd, struct pollfd *fds, client_t *clients);
void handle_client_data(struct pollfd *pfd, client_t *client, persistence_mode_t p_mode, int p_duration, struct pollfd *fds, client_t *clients);

/**
 * @brief Main function for the liteMQ server.
 * Initializes the server, handles command-line arguments for persistence, and enters the main event loop.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings.
 * @return int Returns EXIT_SUCCESS on successful execution, EXIT_FAILURE on error.
 */
int main(int argc, char *argv[]) {
    persistence_mode_t persistence_mode = PERSIST_NONE;
    int persistence_duration = 0;

    // --- Argument Parsing ---
    if (argc > 1) {
        if (strcmp(argv[1], "--persist-all") == 0) {
            persistence_mode = PERSIST_ALL;
            printf("Persistence mode: ALL\n");
        } else if (strcmp(argv[1], "--persist-timed") == 0) {
            if (argc > 2) {
                persistence_mode = PERSIST_TIMED;
                persistence_duration = atoi(argv[2]);
                printf("Persistence mode: TIMED (%d seconds)\n", persistence_duration);
            } else {
                fprintf(stderr, "Usage: %s --persist-timed <seconds>\n", argv[0]);
                exit(EXIT_FAILURE);
            }
        }
    } else {
        printf("Persistence mode: NONE\n");
    }

    // Create logs directory if it doesn't exist
    mkdir(LOG_DIR, 0755);

    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    
    struct pollfd fds[MAX_CLIENTS + 1];
    client_t clients[MAX_CLIENTS + 1];

    // Initializing data structures
    for (int i = 0; i <= MAX_CLIENTS; i++) {
        fds[i].fd = -1;
        clients[i].fd = -1;
        clients[i].type = CLIENT_TYPE_UNKNOWN;
        memset(clients[i].topic, 0, MAX_TOPIC_LEN);
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    set_non_blocking(server_fd);

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    clients[0].fd = server_fd; 

    printf("Server listening on port %d\n", PORT);

    while (1) {
        int ret = poll(fds, MAX_CLIENTS + 1, -1);
        if (ret < 0) {
            perror("poll");
            break;
        }

        if (fds[0].revents & POLLIN) {
            handle_new_connection(server_fd, fds, clients);
        }

        for (int i = 1; i <= MAX_CLIENTS; i++) {
            if (fds[i].fd != -1 && (fds[i].revents & POLLIN)) {
                handle_client_data(&fds[i], &clients[i], persistence_mode, persistence_duration, fds, clients);
            }
        }
    }

    close(server_fd);
    return 0;
}

/**
 * @brief Handles a new incoming client connection.
 * Accepts the new connection, sets it to non-blocking mode, and adds it to the list of monitored file descriptors.
 *
 * @param server_fd The server's listening socket file descriptor.
 * @param fds Pointer to the array of pollfd structures.
 * @param clients Pointer to the array of client_t structures.
 */
void handle_new_connection(int server_fd, struct pollfd *fds, client_t *clients) {
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (new_socket < 0) {
        perror("accept");
        return;
    }

    set_non_blocking(new_socket);

    int i;
    for (i = 1; i <= MAX_CLIENTS; i++) {
        if (fds[i].fd == -1) {
            fds[i].fd = new_socket;
            fds[i].events = POLLIN;
            clients[i].fd = new_socket;
            printf("New connection on fd %d\n", new_socket);
            return;
        }
    }

    printf("Max clients reached. Rejecting new connection.\n");
    close(new_socket);
}

/**
 * @brief Handles incoming data from an existing client connection.
 * Reads data from the client, parses commands (SUB/PUB), and manages client state and message forwarding.
 *
 * @param pfd Pointer to the pollfd structure for the client.
 * @param client Pointer to the client_t structure for the client.
 * @param p_mode The current persistence mode of the server.
 * @param p_duration The persistence duration in seconds (if PERSIST_TIMED).
 * @param fds Pointer to the array of pollfd structures (for forwarding messages).
 * @param clients Pointer to the array of client_t structures (for forwarding messages).
 */
void handle_client_data(struct pollfd *pfd, client_t *client, persistence_mode_t p_mode, int p_duration, struct pollfd *fds, client_t *clients) {
    char buffer[BUFFER_SIZE] = {0};
    int valread = read(pfd->fd, buffer, BUFFER_SIZE - 1);

    if (valread <= 0) {
        printf("Client on fd %d disconnected.\n", pfd->fd);
        close(pfd->fd);
        pfd->fd = -1;
        client->fd = -1;
        client->type = CLIENT_TYPE_UNKNOWN;
        memset(client->topic, 0, MAX_TOPIC_LEN);
        return;
    }

    buffer[valread] = '\0';

    if (client->type == CLIENT_TYPE_UNKNOWN) {
        if (strncmp(buffer, "SUB ", 4) == 0) {
            char *topic_start = buffer + 4;
            char *newline_pos = strchr(topic_start, '\n');
            size_t topic_len;

            if (newline_pos) {
                topic_len = newline_pos - topic_start;
            } else {
                topic_len = strlen(topic_start);
            }

            if (topic_len > 0 && topic_len < MAX_TOPIC_LEN) {
                client->type = CLIENT_TYPE_SUBSCRIBER;
                strncpy(client->topic, topic_start, topic_len);
                client->topic[topic_len] = '\0';
                printf("fd %d subscribed to topic '%s'\n", pfd->fd, client->topic);
                send_persisted_messages(pfd->fd, client->topic, p_mode, p_duration);
            } else {
                fprintf(stderr, "fd %d sent malformed SUB command: %s\n", pfd->fd, buffer);
                close(pfd->fd);
                pfd->fd = -1;
                client->fd = -1;
                client->type = CLIENT_TYPE_UNKNOWN;
                memset(client->topic, 0, MAX_TOPIC_LEN);
            }
        } else if (strncmp(buffer, "PUB ", 4) == 0) {
            // This is a publisher trying to send a message without first being identified.
            // For simplicity, we'll allow it to proceed as a publisher for this single message.
            // In a more complex system, publishers might also register.
            // Fall through to the PUB handling below.
        } else {
            fprintf(stderr, "fd %d sent unknown command: %s\n", pfd->fd, buffer);
            close(pfd->fd);
            pfd->fd = -1;
            client->fd = -1;
            client->type = CLIENT_TYPE_UNKNOWN;
            memset(client->topic, 0, MAX_TOPIC_LEN);
            return;
        }
    }

    if (strncmp(buffer, "PUB ", 4) == 0) {
        char pub_topic[MAX_TOPIC_LEN];
        char *msg_start = strchr(buffer, '\n');

        if (msg_start) {
            int topic_len = msg_start - (buffer + 4);
            if (topic_len > 0 && topic_len < MAX_TOPIC_LEN) {
                strncpy(pub_topic, buffer + 4, topic_len);
                pub_topic[topic_len] = '\0';
                msg_start++; 

                printf("Received message for topic '%s' from fd %d\n", pub_topic, pfd->fd);
                persist_message(pub_topic, msg_start, p_mode);

                // Forward to subscribers
                for (int j = 1; j <= MAX_CLIENTS; j++) {
                    if (fds[j].fd != -1 && clients[j].type == CLIENT_TYPE_SUBSCRIBER && strcmp(clients[j].topic, pub_topic) == 0) {
                        char message_to_send[BUFFER_SIZE];
                        int bytes_to_send = snprintf(message_to_send, BUFFER_SIZE, "MSG %s\n%s", pub_topic, msg_start);
                        if (bytes_to_send < 0 || bytes_to_send >= BUFFER_SIZE) {
                            fprintf(stderr, "Error formatting message for subscriber fd %d\n", clients[j].fd);
                            continue;
                        }
                        if (write(clients[j].fd, message_to_send, bytes_to_send) < 0) {
                            perror("write to subscriber failed");
                        }
                    }
                }
            } else {
                fprintf(stderr, "fd %d sent malformed PUB topic: %s\n", pfd->fd, buffer);
            }
        } else {
            fprintf(stderr, "fd %d sent malformed PUB message (no newline): %s\n", pfd->fd, buffer);
        }
        // Disconnect publisher
        close(pfd->fd);
        pfd->fd = -1;
        client->fd = -1;
    } else if (client->type == CLIENT_TYPE_SUBSCRIBER) {
        // If a subscriber sends data after initial SUB command, it's unexpected.
        // For now, we'll just log it and close the connection.
        fprintf(stderr, "Subscriber fd %d sent unexpected data: %s\n", pfd->fd, buffer);
        close(pfd->fd);
        pfd->fd = -1;
        client->fd = -1;
        client->type = CLIENT_TYPE_UNKNOWN;
        memset(client->topic, 0, MAX_TOPIC_LEN);
    }
}


