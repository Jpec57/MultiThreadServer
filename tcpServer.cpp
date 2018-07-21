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

std::vector<std::string> handleRequest(std::string request)
{
	std::vector<std::string> response;
	if (request == "close")
		response.push_back(std::string("CLOSE"));
	return response;
}

int main()
{

	int sockfd;
	int ret;
	struct sockaddr_in serverAddr;

	int newSocket;
	struct sockaddr_in newAddr;

	socklen_t addr_size;

	char buffer[256];
	bzero(buffer, 256);
	pid_t childpid;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Server Socket is created.\n");

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	ret = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	if (ret < 0)
	{
		printf("[-]Error in binding.\n");
		exit(1);
	}
	printf("[+]Bind to port %d\n", PORT);

	if (listen(sockfd, 10) == 0)
	{
		printf("[+]Listening....\n");
	}
	else
	{
		printf("[-]Error in binding.\n");
	}

	while (42)
	{
		newSocket = accept(sockfd, (struct sockaddr *)&newAddr, &addr_size);
		if (newSocket < 0)
		{
			exit(EXIT_FAILURE);
		}
		printf("Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));

		if ((childpid = fork()) == 0)
		{
			close(sockfd);
			while (42)
			{
				bzero(buffer, 256);
				int n = read(newSocket, buffer, 255);
				if (n == 0)
				{
					std::cout << inet_ntoa(newAddr.sin_addr) << ":" << ntohs(newAddr.sin_port)
							  << " connection closed by client" << std::endl;
					break;
				}
				else if (n < 0)
				{
					std::cerr << "ERROR reading from socket" << std::endl;
					exit(EXIT_FAILURE);
				}
				std::stringstream stream;
				stream << buffer << std::flush;
				while (stream.good())
				{
					std::string request;
					getline(stream, request); // Get and print request by lines
					if (request.length() > 0)
					{
						std::cout << inet_ntoa(newAddr.sin_addr) << ":" << ntohs(newAddr.sin_port)
								  << ": " << request << std::endl;

						std::vector<std::string> response = handleRequest(request); // Get the response

						for (int i = 0; i < response.size(); i++)
						{
							std::string output = response[i];
							if (output != "CLOSE")
							{
								n = write(newSocket, output.c_str(), output.length()); // Write response by line
								if (n < 0)
									std::cerr << "ERROR writing to socket" << std::endl;
							}
							else
							{
								close(newSocket); // Close the connection if response line == "CLOSE"
								std::cout << inet_ntoa(newAddr.sin_addr) << ":" << ntohs(newAddr.sin_port)
										  << " connection terminated" << std::endl;
								break;
							}
						}
					}
				}
			}
		}
	}

	close(newSocket);

	return 0;
}
