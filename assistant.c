#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 256

int main() {
    int sock;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE], response[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Грешка в сокета");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Проблем с връзката");
        exit(EXIT_FAILURE);
    }

    while (1) {
        printf("Въведете изпълнител и песен (формат: Изпълнител;Песен) или 'exit' за да излезете: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        
        if (strcmp(buffer, "exit") == 0) {
            break;
        }
        
        send(sock, buffer, strlen(buffer), 0);
        memset(response, 0, BUFFER_SIZE);
        recv(sock, response, BUFFER_SIZE, 0);
        
        if (strcmp(response, "FOUND") == 0) {
            printf("Песента е налична! Изпейте я.....\n");
        } else {
            printf("Песента не е налична! Ще бъде добавена за следващия път.\n");
        }
    }
    
    close(sock);
    return 0;
}