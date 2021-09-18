#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>
#include <pthread.h>
#include "myqueue.h"

#define BUFSIZE 4000
#define SOCKETERROR (-1)
#define SERVER_BACKLOG 1
#define THREAD_POOL_SIZE 4

pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

void * handle_connection(void* p_client_socket);
int check(int exp, const char *msg);
void * thread_function(void *arg);

int main(int argc, char *argv[]) {
    int server_socket, client_socket, addr_size;
    short port;
    SA_IN server_addr, client_addr;

    if (argc < 2){
        printf("Usage: %s PORT\n", argv[0]);
        exit(0);
    }

    port = (short)atoi(argv[1]);

    printf("Launching Server on Port: %d\n", port);

    
    //create threads for thread pooling
    for(int i=0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
        printf("Thread %d Launched and waiting for work\n", i);
    }
    
    check((server_socket = socket(AF_INET , SOCK_STREAM, 0)),
            "Failed to create socket");

    //initizalize the address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    check(bind(server_socket, (SA*)&server_addr, sizeof(server_addr)),
            "Error binding socket\n");

    check(listen(server_socket, 3),
            "Listen Failed\n");

    while (true) {
        //wait for, and eventually accept an incoming connection
        addr_size = sizeof(SA_IN);
        check(client_socket =
                accept(server_socket, (SA*)&client_addr, (socklen_t*)&addr_size),
                "accept failed");
        printf("New Connection\n");

        //thread pooling        
        int *pclient = malloc(sizeof(int));
        *pclient = client_socket;

        //stop race condition
        pthread_mutex_lock(&mutex);
        enqueue(pclient);
        pthread_mutex_unlock(&mutex);
    }

    return 0;
}

int check(int exp, const char *msg) {
    if (exp == SOCKETERROR) {
        perror(msg);
        exit(1);
    }
    return exp;
}

void * thread_function(void *arg) {
    //int t_id = (int)(long)arg;
    //printf("Thread %d handling new connection\n", t_id);
    while (true) {
        pthread_mutex_lock(&mutex);
        int *pclient = dequeue();
        pthread_mutex_unlock(&mutex);

        if (pclient != NULL) {
            //connection is set
            handle_connection(pclient);
        }
    }
}

void *  handle_connection(void* p_client_socket) {
    int client_socket = *((int*)p_client_socket);
    free(p_client_socket);
    size_t bytes_read;
    int msgsize = 0;
    //char message[200];
    char buffer[BUFSIZE];
    char actualpath[PATH_MAX+1];

    //read client's message -- name of file to read
    while((bytes_read = read(client_socket, buffer+msgsize, sizeof(buffer)-msgsize-1)) > 0) {
        msgsize += bytes_read;
        if (msgsize > BUFSIZE-1 || buffer[msgsize-1] == '\n') break;
    }

    check(bytes_read, "recv error");
    buffer[msgsize-1] = 0; //null terminate the message and remove the \n

    printf("From Client (%d): %s\n", msgsize, buffer);
    fflush(stdout);

    //validity check
    if (realpath(buffer, actualpath) == NULL) {
        printf("ERROR(bath path): %s\n", buffer);
        close(client_socket);
        return NULL;
    }

    //read file and send its contents to client
    FILE *fp = fopen(actualpath, "r");
    if (fp == NULL) {
        printf("ERROR(open: %s\n", buffer);
        close(client_socket);
        return NULL;
    }

    printf("Opening %s\n", buffer);

    //read file contents and send them to client
    while ((bytes_read = fread(buffer, 1, BUFSIZE, fp)) > 0) {
        printf("File is %zu bytes\n", bytes_read);
        write(client_socket, buffer, bytes_read);
        printf("Sending: %zu\n", bytes_read);
    }
    close(client_socket);
    fclose(fp);
    printf("Closing Connection\n");
    return NULL;
}