#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <cstdlib>
#include <fstream>


// global variables
int server_socket;
sockaddr_in server_addr;
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
    char buffer[1024];
    // read data from the client
    ssize_t bytes_received = recv(client_socket, buffer, 1024, 0);

    // check for errors in receiving data
    if (bytes_received == -1)
    {
        std::cerr << "Receive error!" << std::endl;
        close(client_socket);

        delete addr;
        return;
    }
    if (bytes_received == 0)
    {
        std::cout << "Client disconnected!" << std::endl;
        close(client_socket);

        delete addr;
        return;
    }

    // process the data received from the client
    buffer[bytes_received] = '\0'; // null-terminate the string
    std::string path = strtok(buffer, " "); // get the first token ("GET")
    path = strtok(NULL, " "); // get the path from the request (e.g., "/index.html")

    // check if the path is empty
    if (path.empty())
    {
        std::cerr << "Invalid request!" << std::endl;
        close(client_socket);

        delete addr;
        return;
    }

    // setting the default path if the path is not set in the environment variable
    if (path == "/")
        path = std::string(dPath) + "/index.html"; // Use the default path from the environment variable
    else
        path = std::string(dPath) + path; // Append the path to the default path 

    std::cout << "Received request:  " << path << std::endl;


    // open the file and send the response to the client
    std::ifstream file(path, std::ios::binary | std::ios::ate); // Open the file in binary mode and seek to the end
    // check if the file was opened successfully
    if (!file.is_open())
    {
        std::cerr << "File not found!" << std::endl;

        std::string notFound = 
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<h1>404 Not Found</h1>";

        send(client_socket, notFound.c_str(), notFound.size(), 0);
        close(client_socket);

        delete addr;
        return;
    }

    // Determine the file extension
    std::string content_type;
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos != std::string::npos)
        content_type = path.substr(dot_pos + 1);

    // Set the Content-Type based on the file extension
    if (content_type == "html")
        content_type = "text/html";
    else if (content_type == "css")
        content_type = "text/css";
    else if (content_type == "js")
        content_type = "application/javascript";
    else
        content_type = "application/octet-stream"; // Default for unknown types

    // Update the header with the determined Content-Type
    std::string header = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: " + content_type + "\r\n"
                         "Connection: keep-alive\r\n"
                         "Keep-Alive: timeout=5, max=1000\r\n"
                         "\r\n";

    send(client_socket, header.c_str(), header.size(), 0);


    // get the size of the file
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg); // seek back to the beginning of the file
    const size_t BUFFER_SIZE = 4096; // cast to size_t
    char f_buffer[BUFFER_SIZE]; // create a buffer to store the file data
    while(file.read(f_buffer, BUFFER_SIZE))
    {
        // send the file data to the client
        ssize_t bytes_sent = send(client_socket, f_buffer, file.gcount(), 0);
        if (bytes_sent == -1)
        {
            std::cerr << "Send error!" << std::endl;
            break; // exit the loop on error
        }
    }

    if(file.gcount() > 0)
    {
        // send the remaining data to the client
        ssize_t bytes_sent = send(client_socket, f_buffer, file.gcount(), 0);
        if (bytes_sent == -1)
            std::cerr << "Send error!" << std::endl;
    }


    close(client_socket);
    delete addr;
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
    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1)
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
        sockaddr_in* client_addr = new sockaddr_in(); // Create a new sockaddr_in object for the client address
        socklen_t addr_len = sizeof(client_addr); // Initialize the address length

        // accept a new connection
        std::cout << "Waiting for a new connection..." << std::endl;
        int client_socket = accept(server_socket, (sockaddr*)client_addr, &addr_len);
        if (client_socket == -1)
        {
            std::cerr << "Accept error!" << std::endl;
            continue; // Continue to accept other connections
        }

       handle_client(client_socket, client_addr); // Handle the client connection in a separate thread
    }
}


