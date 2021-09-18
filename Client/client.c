#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int sockfd, bytes_read;
    short port;
    struct sockaddr_in servaddr;
    char buffer[2000];

    if(argc < 2) {
        printf("Usage: %s PORT\n", argv[0]);
        exit(0);
    }

    port = (short)atoi(argv[1]);

    printf("Enter file to retrieve: ");
    char userInput[20];
    scanf("%s", userInput);

    printf("Connecting on Localhost at port %d to get file %s\n", port, userInput);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Socket creation failed.\n");
        exit(0);
    }

    //bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1",  &servaddr.sin_addr)<=0) {
        printf("Invalid address, not supported\n");
        exit(0);
    }

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
        perror("Error");
        printf("Connection failed\n");
        exit(0);
    }

    sprintf(buffer, "../Server/root/%s\n", userInput);

    write(sockfd, buffer, strlen(buffer));
    bzero(&buffer, sizeof(buffer));
    bytes_read = read(sockfd, buffer, sizeof(buffer));
    buffer[bytes_read] = 0;
    printf("File size: %d\n", bytes_read);
    printf("From the server: %s\n", buffer);

}