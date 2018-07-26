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
#include <array>
#include <sys/stat.h>
#include <fcntl.h>

#define h_addr h_addr_list[0] /* for backward compatibility */

#define MAX_COUNT 10
#define NB_CHILDREN 3
#define NUM_THREADS 4
#define NB_LOOPS 3

int currentPID = 0;
//Server file descriptor
int sockfd;
std::mutex stdoutMutex;

char **outputFiles;
char **outputNames;
char *fruits[5];
char *meats[5];
char *vegetables[5];
char **all[5];

std::vector<std::string> handleRequest(std::string request, int k)
{
    /*
    std::vector<std::string> response;
    if (k < NB_LOOPS)
    {
        response.push_back(std::string(request));
    }
    else
    {
        response.push_back(std::string("CLOSE"));
    }*/
    std::vector<std::string> response;
    response.push_back(std::string(request));
    return response;
}

void handleConnection(int newsockfd, sockaddr_in *cli_addr, int nb)
{
    char buffer[256];
    std::string request;
    int n;
    int k = 0;

    int fileFd = open(outputFiles[nb], O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fileFd < 0)
    {
        stdoutMutex.lock();
        std::cerr << "ERROR opening file " << outputFiles[nb] << std::endl;
        stdoutMutex.unlock();
    }

    while (true)
    {
        bzero(buffer, 256);
        n = read(newsockfd, buffer, 255);
        if (n == 0)
        {
            stdoutMutex.lock();
            std::cout << inet_ntoa(cli_addr->sin_addr) << ":" << ntohs(cli_addr->sin_port)
                      << " connection closed by client" << std::endl;
            stdoutMutex.unlock();
            return;
        }
        else if (n < 0)
        {
            stdoutMutex.lock();
            std::cerr << "\nNo more " << outputNames[nb] << "\n"
                      << std::endl;
            stdoutMutex.unlock();
            return;
        }
        std::stringstream stream;
        stream << buffer << std::flush;
        while (stream.good())
        {
            for (int i = 0; i < request.length(); i++)
            {
                request[i] = '\0';
            }
            getline(stream, request);
            if (request.length() > 0)
            {
                std::vector<std::string> response = handleRequest(request, k); // Get the response
                stdoutMutex.lock();

                //std::cout << inet_ntoa(cli_addr->sin_addr) << ":" << ntohs(cli_addr->sin_port) << "  " << request << std::endl;

                write(fileFd, request.c_str(), request.length());
                write(fileFd, "\n", 1);
                stdoutMutex.unlock();

                for (int i = 0; i < response.size(); i++)
                {
                    std::string output = response[i];

                    if (output != "CLOSE")
                    {
                        n = write(newsockfd, output.c_str(), output.length()); // Write response by line
                        if (n < 0)
                        {
                            stdoutMutex.lock();
                            std::cerr << "ERROR writing to socket" << std::endl;
                            stdoutMutex.unlock();
                            return;
                        }
                        else
                        {
                            k++;
                        }
                    }
                    else
                    {
                        close(newsockfd); // Close the connection if response line == "CLOSE"
                        stdoutMutex.lock();
                        std::cout << inet_ntoa(cli_addr->sin_addr) << ":" << ntohs(cli_addr->sin_port)
                                  << " connection terminated" << std::endl;
                        stdoutMutex.unlock();
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
    const char *signal_name;
    switch (s)
    {
    case SIGHUP:
        signal_name = "SIGHUP";
        break;
    case SIGUSR1:
        signal_name = "SIGUSR1";
        break;
    case SIGKILL:
        signal_name = "SIGKILL";
        break;
    case SIGILL:
        signal_name = "SIGILL";
        break;
    case SIGALRM:
        signal_name = "SIGALRM";
        break;
    case SIGQUIT:
        signal_name = "SIGQUIT";
        break;
    case SIGCHLD:
        signal_name = "SIGCHLD";
        break;
    case SIGSTOP:
        signal_name = "SIGSTOP";
        break;
    case SIGTSTP:
        signal_name = "SIGTSTP";
        break;
    case SIGTTIN:
        signal_name = "SIGTTIN";
        break;
    case SIGTTOU:
        signal_name = "SIGTTOU";
        break;
    case SIGINT:
        signal_name = "SIGINT";
        break;
    default:
        fprintf(stderr, "Caught wrong signal: %d\n", s);
        exit(EXIT_FAILURE);
        break;
    }
    printf("Caught signal %s\n", signal_name);
    remove("meats.txt");
    remove("fruits.txt");
    remove("vegetables.txt");
    exit(0);
}

void *perform_work(void *arg)
{
    char buffer[256];
    int nb = *((int *)arg);
    if (nb == 0 || nb == 1 || nb == 2 || nb == 3)
    {
        printf("Connecting %d\n", nb);
        int newsockfd;
        unsigned int clilen;
        sockaddr_in cli_addr;

        clilen = sizeof(sockaddr_in);
        newsockfd = accept(sockfd, (sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            stdoutMutex.lock();
            std::cerr << "ERROR on accept" << std::endl;
            stdoutMutex.unlock();
            return (arg);
        }
        stdoutMutex.lock();
        std::cout << inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port)
                  << " connected" << std::endl;
        stdoutMutex.unlock();

        handleConnection(newsockfd, &cli_addr, nb);
    }
    else
    {
        return (arg);
        while (true)
        {
            sleep(5);
            int f = open(outputFiles[rand() % 3], O_RDONLY, 0666);
            if (f < 0)
            {
                stdoutMutex.lock();
                while (read(f, buffer, strlen(buffer)))
                {
                    std::cout << buffer;
                }
                std::cout << " " << std::endl;
                stdoutMutex.unlock();
            }
        }
    }
    return (arg);
}

void serverWork(int *pids, int i, int argc, char **argv)
{
    int portno;

    sockaddr_in serv_addr;

    if (argc < 2)
    {
        std::cerr << "ERROR no port provided" << std::endl;
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "ERROR opening socket" << std::endl;
    }

    int reusePort = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &reusePort, sizeof(reusePort));

    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        std::cerr << "ERROR on binding" << std::endl;

    unsigned int backlogSize = NB_CHILDREN;
    listen(sockfd, backlogSize);
    std::cout << "C++ server opened on port " << portno << std::endl;
    pthread_t threads[NUM_THREADS];
    int index;
    int result_code;
    int thread_args[NUM_THREADS];
    for (int j = 0; j < NUM_THREADS; j++)
    {
        thread_args[j] = j;
    }
    for (index = 0; index < NUM_THREADS; ++index)
    {
        thread_args[index] = index;
        result_code = pthread_create(&threads[index], NULL, perform_work, &thread_args[index]);
    }

    for (index = 0; index < NUM_THREADS; ++index)
    {
        result_code = pthread_join(threads[index], NULL);
    }
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
        sleep(1);
        if (n < 0)
            error("ERROR writing to socket\n");
        n = read(sockfd, buffer, 255);
        if (n < 0)
        {
            return;
        }
        if (strcmp(buffer, "CLOSE") == 0 || strcmp(buffer, "close") == 0)
        {
            return;
        }
        printf("PID %d: %s\n", getpid(), buffer);
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

    outputFiles = (char **)malloc(sizeof(char *) * (3));
    outputFiles[0] = strdup("meats.txt");
    outputFiles[1] = strdup("fruits.txt");
    outputFiles[2] = strdup("vegetables.txt");

    outputNames = (char **)malloc(sizeof(char *) * (3));
    outputNames[0] = strdup("meat");
    outputNames[1] = strdup("fruit");
    outputNames[2] = strdup("vegetable");
}

int main(int argc, char **argv)
{
    initArrays();

    int *pids = (int *)malloc(sizeof(int) * (NB_CHILDREN + 1));
    bzero(pids, NB_CHILDREN);
    signal(SIGINT, my_handler);

    for (int i = 0; i < NB_CHILDREN; i++)
    {
        pids[i] = fork();
        if (pids[i] == 0)
        {
            clientWork(argc, argv, all[i]);
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
    printf("END\n");
    return (EXIT_SUCCESS);
}
