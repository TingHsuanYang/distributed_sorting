/**
 * Slave node receives the file from server and sort it.
 * After sorting, it sends the sorted part back to server.
 * The sorting processes happen concurrently.
 */
#include "slave.hpp"
#include "external_sort.hpp"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
#define BUFFER_SIZE 4096

Slave::Slave(string server_ip, int port) : server_ip(server_ip), port(port)
{
}

int Slave::run()
{
    // create socket, AF_INET = IPv4, SOCK_STREAM = TCP
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        printf("Fail to create a socket.\n");
    }

    // set reuse address
    int reuse_addr = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse_addr, sizeof(reuse_addr));

    // set server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;                   // IPv4
    server_addr.sin_port = htons(port);          // port
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str()); // server IP address
    socklen_t server_addr_len = sizeof(server_addr);

    // connect to server
    int err = connect(socket_fd, (struct sockaddr *)&server_addr, server_addr_len);
    if (err < 0)
    {
        printf("Fail to connect to server.\n");
        close(socket_fd);
        exit(1);
    }
    printf("Connected to server.\n");

    // receive file and write to disk
    string out_name = "slave.input";
    ofstream output(out_name, ios::out | ios::binary);
    char buffer[BUFFER_SIZE];
    ssize_t len;
    while (true)
    {
        len = recv(socket_fd, buffer, sizeof(buffer), 0);
        if (len < 0)
        {
            printf("Fail to receive file.\n");
            close(socket_fd);
            exit(1);
        }
        else if (len == 0)
        {
            break;
        }
        printf("Received %ld bytes.\n", len);
        output.write(buffer, len);
    }

    // using external sort to sort records
    string sort_out_name = "slave.output";
    ExternalSort * es = new ExternalSort(out_name, sort_out_name);
    es->run();

    delete es;

    // send sorted data back to master
    ifstream input;
    input.open(sort_out_name, ios::in | ios::binary);
    if (!input.good())
    {
        printf("Fail to open input file.\n");
        close(socket_fd);
        exit(1);
    }

    struct stat stat_buf;
    int rc = stat(sort_out_name.c_str(), &stat_buf);
    double file_size = rc == 0 ? stat_buf.st_size : -1;
    printf("file size: %.2f GB\n", file_size / 1024.0 / 1024.0 / 1024.0);

    char *buffer = new char[BUFFER_SIZE];
    while (!input.eof())
    {
        input.read(buffer, BUFFER_SIZE);
        int read_size = input.gcount();
        int send_size = send(socket_fd, buffer, read_size, 0);
        if (send_size < 0)
        {
            printf("Fail to send file to server.\n");
            close(socket_fd);
            exit(1);
        }
        printf("Send %d bytes to server\n", send_size);
        bzero(buffer, BUFFER_SIZE);
    }

    // send 0 bytes to server to indicate the end of file
    int send_size = send(socket_fd, buffer, 0, 0);
    if (send_size < 0)
    {
        printf("Fail to send file to server.\n");
        close(socket_fd);
        exit(1);
    }
    printf("Send %d bytes to server\n", send_size);
    
    // close input file
    input.close();

    // close socket
    close(socket_fd);

    output.close();
    return 0;
}
