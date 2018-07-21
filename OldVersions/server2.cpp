#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string>
#include <sstream>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <signal.h>
#include <netdb.h>
#define h_addr h_addr_list[0] /* for backward compatibility */

#define MAX_COUNT 10
#define NB_CHILDREN 4

int currentPID = 0;

std::vector<std::string> handleRequest(std::string request)
{
    std::vector<std::string> response;

    response.push_back(std::string("Pong!")); // Respond with "Pong!"
    //response.push_back(std::string("CLOSE")); // Close the connection with poison pill

    return response;
}

void handleConnection(int newsockfd, sockaddr_in *cli_addr)
{
    char buffer[256]; // Initialize buffer to zeros
    bzero(buffer, 256);

    while (true)
    {
        int n = read(newsockfd, buffer, 255);
        if (n == 0)
        {
            std::cout << inet_ntoa(cli_addr->sin_addr) << ":" << ntohs(cli_addr->sin_port)
                      << " connection closed by client" << std::endl;
            return;
        }
        else if (n < 0)
            std::cerr << "ERROR reading from socket" << std::endl;

        std::stringstream stream;
        stream << buffer << std::flush;
        while (stream.good())
        {
            std::string request;
            getline(stream, request); // Get and print request by lines
            if (request.length() > 0)
            {
                std::cout << inet_ntoa(cli_addr->sin_addr) << ":" << ntohs(cli_addr->sin_port)
                          << ": " << request << std::endl;

                std::vector<std::string> response = handleRequest(request); // Get the response

                for (int i = 0; i < response.size(); i++)
                {
                    std::string output = response[i];
                    if (output != "CLOSE")
                    {
                        n = write(newsockfd, output.c_str(), output.length()); // Write response by line
                        if (n < 0)
                            std::cerr << "ERROR writing to socket" << std::endl;
                    }
                    else
                    {
                        close(newsockfd); // Close the connection if response line == "CLOSE"
                        std::cout << inet_ntoa(cli_addr->sin_addr) << ":" << ntohs(cli_addr->sin_port)
                                  << " connection terminated" << std::endl;
                        return;
                    }
                }
            }
        }
    }
}

void error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void my_handler(int s)
{
    printf("Caught signal %d\n", s);
    //kill(currentPID, SIGKILL);
    exit(EXIT_SUCCESS);
}

void serverWork(int *pids, int i, int argc, char **argv)
{
    int sockfd; // Socket file descriptor
    int portno; // Port number

    sockaddr_in serv_addr; // Server address

    if (argc < 2)
    {
        std::cerr << "ERROR no port provided" << std::endl;
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create new socket, save file descriptor
    if (sockfd < 0)
    {
        std::cerr << "ERROR opening socket" << std::endl;
    }

    int reusePort = 1; // Disables default "wait time" after port is no longer in use before it is unbound
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &reusePort, sizeof(reusePort));

    bzero((char *)&serv_addr, sizeof(serv_addr)); // Initialize serv_addr to zeros
    portno = atoi(argv[1]);                       // Reads port number from char* array

    serv_addr.sin_family = AF_INET;         // Sets the address family
    serv_addr.sin_port = htons(portno);     // Converts number from host byte order to network byte order
    serv_addr.sin_addr.s_addr = INADDR_ANY; // Sets the IP address of the machine on which this server is running

    if (bind(sockfd, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) // Bind the socket to the address
        std::cerr << "ERROR on binding" << std::endl;

    unsigned int backlogSize = 5; // Number of connections that can be waiting while another finishes
    listen(sockfd, backlogSize);
    std::cout << "C++ server opened on port " << portno << std::endl;
    while (true)
    {
        printf("PID %d: I am the boss\n", pids[i]);
        int newsockfd;        // New socket file descriptor
        unsigned int clilen;  // Client address size
        sockaddr_in cli_addr; // Client address

        clilen = sizeof(sockaddr_in);
        newsockfd = accept(sockfd, (sockaddr *)&cli_addr, &clilen); // Block until a client connects
        if (newsockfd < 0)
            std::cerr << "ERROR on accept" << std::endl;
//TODO
        std::cout << inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port)
                  << " connected" << std::endl;

        handleConnection(newsockfd, &cli_addr); // Handle the connection
        //std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void    clientWork(int argc, char **argv)
{
            int sockfd, portno, n;
            struct sockaddr_in serv_addr;
            struct hostent *server;

            //Detect close
            struct sigaction act;
            act.sa_handler = my_handler;
            sigemptyset(&act.sa_mask);
            act.sa_flags = 0;

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
                printf("CONNECTED\n");
            while (true)
            {
                printf("[son] pid %d from [parent] pid %d:      SLEEPING\n", getpid(), getppid());
                printf("Please enter the message: \n");
                bzero(buffer, 256);
                fgets(buffer, 255, stdin);
                n = write(sockfd, buffer, strlen(buffer));
                if (n < 0)
                    error("ERROR writing to socket\n");
                bzero(buffer, 256);
                n = read(sockfd, buffer, 255);
                sigaction(SIGINT, &act, 0);
                if (n < 0)
                    error("ERROR reading from socket\n");
                if (strcmp(buffer, "CLOSE") == 0 || strcmp(buffer, "close") == 0)
                {
                    break;
                }
                printf("BEFORE\n");
                printf("%s\n", buffer);
                printf("AFTER\n");
            }

            exit(0);
}

int main(int argc, char **argv)
{
    //Detect close
    struct sigaction act;
    act.sa_handler = my_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    int *pids = (int *)malloc(sizeof(int) * (NB_CHILDREN + 1));
    bzero(pids, NB_CHILDREN);
    for (int i = 0; i < NB_CHILDREN; i++)
    {
        pids[i] = fork();
        if (pids[i] == 0)
        {
            clientWork(argc, argv);
        }
        else if (pids[i] > 0)
        {
            if (i == NB_CHILDREN - 1)
            {
                serverWork(pids, i, argc, argv);
            }
        }
        else
        {
            printf("Error in forking\n");
            exit(EXIT_FAILURE);
        }
    }
    /*
    for (int i = 0; i < NB_CHILDREN; i++)
    {
        wait(NULL);
    }*/
    printf("END\n");
    return (EXIT_SUCCESS);
}

/*

int main(int argc, const char *argv[]) {
	int sockfd; // Socket file descriptor
	int portno; // Port number

	sockaddr_in serv_addr; // Server address

	if (argc < 2) {
		std::cerr << "ERROR no port provided" << std::endl;
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create new socket, save file descriptor
	if (sockfd < 0) {
		std::cerr << "ERROR opening socket" << std::endl;
	}

	int reusePort = 1; // Disables default "wait time" after port is no longer in use before it is unbound
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &reusePort, sizeof(reusePort));

	bzero((char *) &serv_addr, sizeof(serv_addr)); // Initialize serv_addr to zeros
	portno = atoi(argv[1]); // Reads port number from char* array

	serv_addr.sin_family = AF_INET; // Sets the address family
	serv_addr.sin_port = htons(portno); // Converts number from host byte order to network byte order
	serv_addr.sin_addr.s_addr = INADDR_ANY; // Sets the IP address of the machine on which this server is running

	if (bind(sockfd, (sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) // Bind the socket to the address
		std::cerr << "ERROR on binding" << std::endl;

	unsigned int backlogSize = 5; // Number of connections that can be waiting while another finishes
	listen(sockfd, backlogSize);
	std::cout << "C++ server opened on port " << portno << std::endl;;

	while (true) {
		int newsockfd; // New socket file descriptor
		unsigned int clilen; // Client address size
		sockaddr_in cli_addr; // Client address

		clilen = sizeof(sockaddr_in);
		newsockfd = accept(sockfd, (sockaddr *) &cli_addr, &clilen); // Block until a client connects
		if (newsockfd < 0)
			std::cerr << "ERROR on accept" << std::endl;

		std::cout << inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port)
				<< " connected" << std::endl;

		handleConnection(newsockfd, &cli_addr); // Handle the connection
	}
	return 0;
}
*/

/*
    int pid = fork();
    if (pid == 0)
    {
        while (true)
        {
                printf("[son] pid %d from [parent] pid %d:      SLEEPING\n", getpid(), getppid());
                std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    else if (pid > 0)
    {
        while (true)
        {
                               printf("PID %d: I am the boss\n", pid);
                    std::this_thread::sleep_for(std::chrono::seconds(2)); 
        }
    }
    else
    {
        printf("ERROR\n");
    }*/
