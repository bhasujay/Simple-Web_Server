#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <cstdlib>


// global variables
int server_socket;
sockaddr_in server_addr;
sockaddr_in client_addr;
socklen_t addr_len = sizeof(client_addr);
const char* sIP = std::getenv("MY_IP");
const char* dPath = std::getenv("MY_PATH");
const int PORT = 8080;
const int MAX_CONNECTIONS = 5;


void handle_client(int client_socket, sockaddr_in* addr)
{
    // handle client connection
    std::cout << "Client connected!" << std::endl;
    std::cout << "Client IP: " << inet_ntoa(addr->sin_addr) << std::endl;

    // buffer to store data received from the client
    char* buffer = new char[1024];
    // read data from the client
    ssize_t bytes_received = recv(client_socket, buffer, 1024, 0);

    // check for errors in receiving data
    if (bytes_received == -1)
    {
        std::cerr << "Receive error!" << std::endl;

        delete addr;
        delete[] buffer;
        close(client_socket);
        return;
    }
    if (bytes_received == 0)
    {
        std::cout << "Client disconnected!" << std::endl;

        delete addr;
        delete[] buffer;
        close(client_socket);
        return;
    }

    // process the data received from the client

    buffer[bytes_received] = '\0'; // null-terminate the string

    std::string path = strtok(buffer, " "); // get the first token ("GET")
    path = strtok(NULL, " "); // get the path from the request (e.g., "/index.html")

    if (path.empty())
    {
        std::cerr << "Invalid request!" << std::endl;

        delete[] buffer;
        close(client_socket);
        return;
    }

    // setting the default path if the path is not set in the environment variable
    if (path == "/")
        path = std::string(dPath); // Use the default path from the environment variable
    else
        path = std::string(dPath) + path; // Append the path to the default path

    std::cout << "Received request:  " << path << std::endl;

    // open the file
    // FILE* file = fopen(path, "rb");
    // if (file == NULL)
    // {
    //     std::cerr << "File not found!" << std::endl;
    //     const char* not_found_response = "HTTP/1.1 404 Not Found\r\n\r\n";
    //     send(client_socket, not_found_response, strlen(not_found_response), 0);

    //     delete[] buffer;
    //     close(client_socket);
    //     return;
    // }


    delete addr;
    delete[] buffer;
    close(client_socket);
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
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(sIP); // Use the IP address from the environment variable

    // bind the socket to the address and port
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
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

    // accept incoming connections in a loop
    while(true)
    {
        // accept a new connection
        std::cout << "Waiting for a new connection..." << std::endl;
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket == -1)
        {
            std::cerr << "Accept error!" << std::endl;
            continue; // Continue to accept other connections
        }

        sockaddr_in* client_addr_ptr = new sockaddr_in(client_addr); // Create a new sockaddr_in object for the client address

        handle_client(client_socket, client_addr_ptr); // Handle the client connection in a separate thread
    }
}


