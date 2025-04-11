#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <pthread.h>
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <ifaddrs.h>
#include <sstream>

// global variables
int server_socket;
struct sockaddr_in server_addr;
struct sockaddr_in client_addr;
socklen_t addr_len = sizeof(client_addr);
char buffer[1024];
pthread_t thread_ID;
const char* sIP = std::getenv("MY_IP");
const char* dPath = std::getenv("MY_PATH");
const int PORT = 8080;
const int MAX_CONNECTIONS = 5;


void handle_client(int client_socket)
{

}



int main()
{   
    // create a socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    // check if the socket was created successfully
    if (server_socket == -1)
    {
        std::cerr << "Socket creation error" << std::endl;
        return 1;
    }

    // set the server address and port
    memset(&server_addr, 0, sizeof(server_addr)); // Clear the structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(sIP); // Use the IP address from the environment variable

    // bind the socket to the address and port
    if (bind(server_socker, (struct sockaddr*)&server_addr), sizeof(server_addr)) == -1)
    {
        std::cerr << "Bind error!" << std::endl;
        close(server_socket);
        return 1;
    }

    // listen for incoming connections
    if (listen(server_socket, MAX_CONNECTIONS) == -1)
    {
        std::cerr << "Listen error!" << std::endl;
        close(server_socket);
        return 1;
    }

    std::cout << "The server is listening on " << sIP << ":" << PORT << std::endl;








}


