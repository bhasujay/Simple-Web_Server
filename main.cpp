#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <cstdlib>
#include <fstream>
#include <unordered_map>
#include <ctime>
#include <thread>


// global variables
int server_socket;
sockaddr_in server_addr;
const char* sIP = std::getenv("MY_IP");
const char* dPath = std::getenv("MY_PATH");
const int PORT = std::getenv("MY_PORT");
const int MAX_CONNECTIONS = 5;
const int MAX_REQUESTS = 1000;
const int TIMEOUT_SECONDS = 5;
const size_t REQUEST_BUFFER_SIZE = 1024;
const size_t FILE_BUFFER_SIZE = 4096;

// MIME types map
const std::unordered_map<std::string, std::string> MIME_TYPES = 
{
    {"html", "text/html"},
    {"css", "text/css"},
    {"js", "application/javascript"},
    {"png", "image/png"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"gif", "image/gif"},
    {"mp3", "audio/mpeg"},
    {"wav", "audio/wav"},
    {"mp4", "video/mp4"},
    {"avi", "video/x-msvideo"},
    {"mov", "video/quicktime"},
    {"pdf", "application/pdf"},
    {"zip", "application/zip"},
    {"json", "application/json"},
    {"xml", "application/xml"},
    {"txt", "text/plain"},
    {"ico", "image/x-icon"},
    {"svg", "image/svg+xml"},
    {"woff", "font/woff"},
    {"woff2", "font/woff2"},
    {"otf", "font/otf"},
    {"ttf", "font/ttf"},
    {"eot", "application/vnd.ms-fontobject"},
    {"csv", "text/csv"},
    {"mpg", "video/mpeg"},
    {"mpeg", "video/mpeg"},
    {"webm", "video/webm"},
    {"flv", "video/x-flv"},
    {"mkv", "video/x-matroska"}
};


void handle_client(int client_socket, sockaddr_in* addr)
{
    // handle client connection
    std::cout << "Client connected!" << std::endl;
    std::cout << "Client IP: " << inet_ntoa(addr->sin_addr) << std::endl;

    char buffer[REQUEST_BUFFER_SIZE]; // buffer to store the request data
    char f_buffer[FILE_BUFFER_SIZE]; // buffer to store the file data
    
    // set the timeout for the socket
    short counter = MAX_REQUESTS; // flag to stop the server
    std::clock_t end_time = std::clock() + TIMEOUT_SECONDS * CLOCKS_PER_SEC; // set the timeout duration
    
    // set the socket to non-blocking mode
    while(counter > 0)
    {
        // check if the timeout has been reached
        if (std::clock() > end_time)
        {
            std::cerr << "Timeout reached!" << std::endl;
            break; // set the counter to 0 to exit the loop after the last request
        }
        // read data from the client
        ssize_t bytes_received = recv(client_socket, buffer, 1024, 0);

        // check for errors in receiving data
        if (bytes_received == -1)
        {
            std::cerr << "Receive error!" << std::endl;

            counter--; // Decrement the counter
            continue; // Continue to the next iteration
        }
        if (bytes_received == 0)
        {
            std::cout << "Client disconnected!" << std::endl;
            break; // exit the loop if the client disconnected
        }

        // process the data received from the client
        buffer[bytes_received] = '\0'; // null-terminate the string

        std::string path = strtok(buffer, " "); // get the first token ("GET")
        path = strtok(NULL, " "); // get the path from the request (e.g., "/index.html")

        // check if the path is empty
        if (path.empty())
        {
            std::cerr << "Invalid request!" << std::endl;
            counter--; // Decrement the counter
            continue; // Continue to the next iteration
        }

        // setting the default path if the path is not set in the environment variable
        if (path == "/")
        path = std::string(dPath) + "/index.html"; // Use the default path from the environment variable
        else
        path = std::string(dPath) + path; // Append the path to the default path 
        
        std::cout << "Requested resource: " << path << std::endl;

        // Determine the file extension
        std::string content_type;
        size_t dot_pos = path.find_last_of('.');
        if (dot_pos != std::string::npos)
            content_type = std::string_view(path).substr(dot_pos + 1);

        // Set the Content-Type based on the file extension
        auto it = MIME_TYPES.find(content_type);
        if (it != MIME_TYPES.end())
            content_type = it->second; // Get the MIME type from the map
        else
            content_type = "application/octet-stream"; // Default to binary if not found
        
        // open the file and send the response to the client
        std::ifstream file(path, std::ios::binary); // Open the file in binary mode

        // check if the file was opened successfully

        // if not, send a 404 Not Found response
        if (!file.is_open())
        {
            std::cerr << "File not found!" << std::endl;

            std::string notFound = "HTTP/1.1 404 Not Found\r\n"
                                    "Content-Type: " + content_type + "\r\n"
                                    "Connection: keep-alive\r\n"
                                    "\r\n"
                                    "<h1>404 Not Found</h1>";

            send(client_socket, notFound.c_str(), notFound.size(), 0);

            counter--; // Decrement the counter
            continue; // Continue to the next iteration
        }
        // if opened successfully, send a 200 OK response
        else
        {
            // Update the header with the determined Content-Type
            std::string header = "HTTP/1.1 200 OK\r\n"
                                "Content-Type: " + std::string(content_type) + "\r\n"
                                "Connection: keep-alive\r\n"
                                "Keep-Alive: timeout=" + std::to_string(TIMEOUT_SECONDS) + ", max=" + std::to_string(MAX_REQUESTS) + "\r\n"
                                "\r\n";
    
            send(client_socket, header.c_str(), header.size(), 0);

            counter--; // Decrement the counter
        }

        // read the file and send it to the client
        while(file.read(f_buffer, FILE_BUFFER_SIZE))
        {
            // send the file data to the client
            ssize_t bytes_sent = send(client_socket, f_buffer, file.gcount(), 0);
            if (bytes_sent == -1)
            {
                std::cerr << "Send error!" << std::endl;
                break; // exit the loop on error
            }
        }
        // send any remaining data in the buffer
        if (file.gcount() > 0)
        {
            ssize_t bytes_sent = send(client_socket, f_buffer, file.gcount(), 0);
            if (bytes_sent == -1)
                std::cerr << "Send error!" << std::endl;
        }

        // close the file   
        file.close(); // Close the file after sending it to the client
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
    std::cout << "Server path: " << dPath << std::endl; // Print the server path

    // accept incoming connections in a loop
    while(true)
    {
        sockaddr_in* client_addr = new sockaddr_in(); // Create a new sockaddr_in object for the client address
        socklen_t addr_len = sizeof(*client_addr); // Initialize the address length

        // accept a new connection
        std::cout << "Waiting for a new connection..." << std::endl;
        int client_socket = accept(server_socket, (struct sockaddr*)client_addr, &addr_len);

        std::cout << "New connection accepted!" << std::endl;

        if (client_socket == -1)
        {
            std::cerr << "Accept error!" << std::endl;
            continue; // Continue to accept other connections
        }

        std::thread client_handler(handle_client, client_socket, client_addr); // Create a new thread to handle the client
        client_handler.detach(); // Detach the thread to allow it to run independently
    }
}
