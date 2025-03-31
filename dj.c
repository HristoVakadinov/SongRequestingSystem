#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>

#define PORT 8080
#define FILE_NAME "songs_db.txt"
#define BUFFER_SIZE 256

void *handle_client(void *client_socket);
int song_exists(const char *artist, const char *song);
void add_song(const char *artist, const char *song);

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) { 
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Сървърът е отворен на порт %d\n", PORT);

    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, &addr_len)) < 0) {
            perror("Грешка при приемане на асистент!");
            continue;
        }

        printf("Нов асистент се свърза\n");

        pthread_t thread_id;
        int *client_sock_ptr = malloc(sizeof(int)); 
        *client_sock_ptr = client_socket;
        pthread_create(&thread_id, NULL, handle_client, client_sock_ptr);
        pthread_detach(thread_id); 
    }

    close(server_fd);
    return 0;
}

void *handle_client(void *client_socket) {
    int sock = *(int *)client_socket;
    free(client_socket); 

    char buffer[BUFFER_SIZE];
    char artist[BUFFER_SIZE], song[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("Асистент излезе.\n");
            break;
        }

        sscanf(buffer, "%[^;];%[^\n]", artist, song);
        printf("Заявка: Изпълнител: %s, Песен: %s\n", artist, song);

        if (song_exists(artist, song)) {
            send(sock, "FOUND", 6, 0);
        } else {
            add_song(artist, song);
            send(sock, "NOT_FOUND", 10, 0);
        }
    }

    close(sock);
    return NULL;
}

int song_exists(const char *artist, const char *song) {
    int fd = open(FILE_NAME, O_RDONLY);
    if (fd < 0) return 0;

    FILE *file = fdopen(fd, "r");
    if (!file) {
        perror("fdopen failed");
        close(fd);
        return 0;
    }

    char line[BUFFER_SIZE];
    while (fgets(line, BUFFER_SIZE, file)) {
        char file_artist[BUFFER_SIZE], file_song[BUFFER_SIZE];
        sscanf(line, "%[^;];%[^\n]", file_artist, file_song);
        if (strcmp(file_artist, artist) == 0 && strcmp(file_song, song) == 0) {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}

void add_song(const char *artist, const char *song) {
    int fd = open(FILE_NAME, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd < 0) {
        perror("Failed to open file");
        return;
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("Failed to lock file");
        close(fd);
        return;
    }

    FILE *file = fdopen(fd, "a");
    if (!file) {
        perror("fdopen failed");
        close(fd);
        return;
    }

    fprintf(file, "%s;%s\n", artist, song);
    fflush(file);

    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);

    fclose(file);
}
