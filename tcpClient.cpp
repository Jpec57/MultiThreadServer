#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>

char *fruits[5];
char *meats[5];
char *vegetables[5];
char **all[5];

void my_handler(int s)
{
	printf("Caught signal %d\n", s);
	exit(EXIT_SUCCESS);
}

void error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void clientWork(int argc, char **argv, char **aliments)
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 3)
    {
        fprintf(stderr, "usage %s port hostname\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[1]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(argv[2]);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");
	else
		printf("-------------------------------------------\n\n");
    while (true)
    {
        bzero(buffer, 256);
        int randNum = rand() % 5;
        strcpy(buffer, aliments[randNum]);
        n = write(sockfd, buffer, strlen(buffer));

        for (int j = 0; j < 31; j++)
        {
            signal(j, my_handler);
        }
		//TO SEE OUTPUT
        sleep(1);
        if (n < 0)
            error("ERROR writing to socket\n");
		printf("ACTIVE PID %d: %s\n", getpid(), buffer);
        n = read(sockfd, buffer, 255);
        printf("AFTER\n");
		if (n < 0)
        {
            return;
        }
        if (strcmp(buffer, "CLOSE") == 0 || strcmp(buffer, "close") == 0)
        {
            return;
        }
        bzero(buffer, 256);
    }
    printf("END CLIENT\n");
    exit(0);
}

void initArrays()
{
    vegetables[0] = strdup("Carrots");
    vegetables[1] = strdup("Salads");
    vegetables[2] = strdup("Cucumbers");
    vegetables[3] = strdup("Leeks");
    vegetables[4] = strdup("Cauliflowers");

    fruits[0] = strdup("Apple");
    fruits[1] = strdup("Banana");
    fruits[2] = strdup("Ananas");
    fruits[3] = strdup("Strawberry");
    fruits[4] = strdup("Coconut");

    meats[0] = strdup("Horse");
    meats[1] = strdup("Pork");
    meats[2] = strdup("Lamb");
    meats[3] = strdup("Bison");
    meats[4] = strdup("Roebuck");

    all[0] = meats;
    all[1] = fruits;
    all[2] = vegetables;
}

int main(int argc, char **argv)
{

	int clientSocket, ret;
	struct sockaddr_in serverAddr;
	char buffer[1024];
	initArrays();

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket < 0)
	{
		printf("[-]Error in connection.\n");
		exit(EXIT_FAILURE);
	}
	printf("[+]Client Socket is created.\n");

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[1]));
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	ret = connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	if (ret < 0)
	{
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Connected to Server.\n");
	clientWork(argc, argv, all[0]);

	return EXIT_SUCCESS;
}
