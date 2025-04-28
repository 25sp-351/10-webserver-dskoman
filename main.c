#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 80
#define BUFFER_SIZE 1024
#define STATIC_DIR "./static"

// Function prototypes
void *handle_client(void *client_socket);
void handle_static(int client_socket, const char *file_path);
void handle_calc(int client_socket, const char *operation, const char *num1, const char *num2);
void send_response(int client_socket, const char *status, const char *content_type, const char *body);

int main(int argc, char *argv[])
{
    int port = PORT;
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // begin reading arguments

    // arg[0] == program name
    // arg[1] == port number (optional)
    //
    // if no port number is given, use 80
    if (argc < 2)
    {
        printf("starting server on default port (80)\n");
        fprintf(stderr, "to launch with custom port: %s <port>\n", argv[0]);
    }
    else if (argc == 2)
    {
        // parse the port number if supplied
        port = strtol(argv[1], NULL, 10);
        if (port <= 0 || port > 65535)
        {
            fprintf(stderr, "invalid port number: %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        fprintf(stderr, "invalid number of arguments\n");
        printf("to launch with custom port: %s <port>\n", argv[0]);
        printf("to start with default port (80): %s\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // end reading arguments

    // Bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 10) < 0)
    {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is running on port %d...\n", port);

    while (1)
    {
        // Accept a client connection
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Accept failed");
            continue;
        }

        // Create a new thread to handle the client
        pthread_t thread_id;
        int *client_sock_ptr = malloc(sizeof(int));
        *client_sock_ptr = client_socket;
        if (pthread_create(&thread_id, NULL, handle_client, client_sock_ptr) != 0)
        {
            perror("Thread creation failed");
            close(client_socket);
            free(client_sock_ptr);
        }

        // Detach the thread to free resources when it finishes
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}

void *handle_client(void *client_socket)
{
    int sock = *(int *)client_socket;
    free(client_socket);

    char buffer[BUFFER_SIZE] = {0};
    read(sock, buffer, BUFFER_SIZE);

    // Parse the HTTP request
    char method[16], path[256];
    sscanf(buffer, "%s %s", method, path);

    if (strcmp(method, "GET") != 0)
    {
        send_response(sock, "405 Method Not Allowed", "text/plain", "Only GET method is supported.");
        close(sock);
        return NULL;
    }

    // Handle /static
    if (strncmp(path, "/static/", 8) == 0)
    {
        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s%s", STATIC_DIR, path + 7);
        handle_static(sock, file_path);
    }
    // Handle /calc
    else if (strncmp(path, "/calc/", 6) == 0)
    {
        char operation[16], num1[16], num2[16];
        if (sscanf(path, "/calc/%15[^/]/%15[^/]/%15s", operation, num1, num2) == 3)
        {
            handle_calc(sock, operation, num1, num2);
        }
        else
        {
            send_response(sock, "400 Bad Request", "text/plain", "Invalid /calc request format.");
        }
    }
    else
    {
        send_response(sock, "404 Not Found", "text/plain", "The requested resource was not found.");
    }

    close(sock);
    return NULL;
}

void handle_static(int client_socket, const char *file_path)
{
    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0)
    {
        send_response(client_socket, "404 Not Found", "text/plain", "File not found.");
        return;
    }

    struct stat file_stat;
    fstat(file_fd, &file_stat);

    char *file_buffer = malloc(file_stat.st_size);
    read(file_fd, file_buffer, file_stat.st_size);

    send_response(client_socket, "200 OK", "application/octet-stream", file_buffer);

    free(file_buffer);
    close(file_fd);
}

void handle_calc(int client_socket, const char *operation, const char *num1, const char *num2)
{
    double n1 = atof(num1);
    double n2 = atof(num2);
    double result = 0;

    if (strcmp(operation, "add") == 0)
    {
        result = n1 + n2;
    }
    else if (strcmp(operation, "mul") == 0)
    {
        result = n1 * n2;
    }
    else if (strcmp(operation, "div") == 0)
    {
        if (n2 == 0)
        {
            send_response(client_socket, "400 Bad Request", "text/plain", "Division by zero.");
            return;
        }
        result = n1 / n2;
    }
    else
    {
        send_response(client_socket, "400 Bad Request", "text/plain", "Invalid operation.");
        return;
    }

    char response_body[128];
    snprintf(response_body, sizeof(response_body), "Result: %.2f\n", result);
    send_response(client_socket, "200 OK", "text/plain", response_body);
}

void send_response(int client_socket, const char *status, const char *content_type, const char *body)
{
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response),
             "HTTP/1.1 %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "\r\n"
             "%s",
             status, content_type, strlen(body), body);
    send(client_socket, response, strlen(response), 0);
}