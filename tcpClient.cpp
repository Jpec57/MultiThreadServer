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
#define PORT 4444

void my_handler(int s)
{
	printf("Caught signal %d\n", s);
	//kill(currentPID, SIGKILL);
	exit(EXIT_SUCCESS);
}

int main()
{

	int clientSocket, ret;
	struct sockaddr_in serverAddr;
	char buffer[1024];

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket < 0)
	{
		printf("[-]Error in connection.\n");
		exit(EXIT_FAILURE);
	}
	printf("[+]Client Socket is created.\n");

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	ret = connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	if (ret < 0)
	{
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Connected to Server.\n");
	int n;
		//Detect close
	struct sigaction act;
	act.sa_handler = my_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	while (true)
	{
		printf("[son] pid %d from [parent] pid %d\n", getpid(), getppid());
		printf("Please enter the message: \n");
		//bzero(buffer, 256);
		fgets(buffer, 255, stdin);
		n = write(clientSocket, buffer, strlen(buffer));
		if (n < 0)
		{
			printf("ERROR writing to socket\n");
			exit(EXIT_FAILURE);
		}
		bzero(buffer, 256);
		//printf("1/BEFORE\n");
		//TO CLOSE IF REQUEST FROM SERVER IS "CLOSE"
		//n = read(clientSocket, buffer, 255);
		//printf("1/AFTER %d\n", n);
		if (n < 0)
		{
			printf("ERROR reading from socket\n");
			exit(EXIT_FAILURE);
		}
		if (strcmp(buffer, "CLOSE") == 0 || strcmp(buffer, "close") == 0)
		{
			break;
		}
		//TO PRINT CONTENT OF BUFFER
        //printf("%s\n", buffer);
		sigaction(SIGINT, &act, 0);

	}

	return EXIT_SUCCESS;
}
