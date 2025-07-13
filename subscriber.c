/**
 * @file subscriber.c
 * @brief Implements the liteMQ subscriber client.
 * @author Mohammed Uddin
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080

/**
 * @brief Main function for the liteMQ subscriber client.
 * Connects to the server, subscribes to a specified topic, and continuously receives messages.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings (expected: <topic>).
 * @return int Returns EXIT_SUCCESS on successful execution, EXIT_FAILURE on error.
 */
int main(int argc, char const *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <topic>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    char message[1024];
    snprintf(message, sizeof(message), "SUB %s", argv[1]);

    send(sock, message, strlen(message), 0);
    printf("Subscribed to topic: %s\n", argv[1]);

    while (1) {
        int valread = read(sock, buffer, 1024);
        if (valread == 0) {
            printf("Server disconnected\n");
            break;
        }
        printf("%s\n", buffer);
        memset(buffer, 0, sizeof(buffer));
    }

    close(sock);
    return 0;
}
