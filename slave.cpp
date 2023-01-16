/**
 * Slave node receives the file from server and sort it.
 * After sorting, it sends the sorted part back to server.
 * The sorting processes happen concurrently.
 */
#include "slave.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "external_sort.hpp"
#define BUFFER_SIZE 4096

using namespace std;

Slave::Slave(string server_ip, int port) : server_ip(server_ip), port(port) {}
Slave::~Slave() {}

int Slave::run() {
    // create socket, AF_INET = IPv4, SOCK_STREAM = TCP
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        printf("Fail to create a socket.\n");
    }

    // set reuse address
    int reuse_addr = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse_addr, sizeof(reuse_addr));

    // set server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;                            // IPv4
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());  // server IP address
    server_addr.sin_port = htons(port);                          // port
    socklen_t server_addr_len = sizeof(server_addr);

    // set client port
    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;          // IPv4
    client_addr.sin_addr.s_addr = INADDR_ANY;  // Any IP address
    client_addr.sin_port = htons(12346);           // Any port
    socklen_t client_addr_len = sizeof(client_addr);

    // bind socket to client port
    int err = bind(socket_fd, (struct sockaddr*)&client_addr, client_addr_len);
    if (err < 0) {
        printf("Fail to bind socket to client port.\n");
        close(socket_fd);
        exit(1);
    }

    // connect to server
    err = connect(socket_fd, (struct sockaddr*)&server_addr, server_addr_len);
    if (err < 0) {
        printf("Fail to connect to server.\n");
        close(socket_fd);
        exit(1);
    }
    printf("Connected to server.\n");

    // receive file and write to disk
    string input_name = "slave.input";
    ofstream output(input_name, ios::out | ios::binary);
    char buffer[BUFFER_SIZE];
    ssize_t len;
    while (true) {
        len = recv(socket_fd, buffer, sizeof(buffer), 0);
        if (len < 0) {
            printf("Fail to receive file.\n");
            close(socket_fd);
            exit(1);
        } else if (len == 0) {
            break;
        }
        printf("Received %ld bytes.\n", len);
        output.write(buffer, len);
    }

    output.close();

    // using external sort to sort records
    string sort_out_name = "slave.output";
    ExternalSort* es = new ExternalSort(input_name, sort_out_name);
    es->run();
    delete es;

    // send sorted data back to master
    // ifstream input;
    // input.open(sort_out_name, ios::in | ios::binary);
    // if (!input.good()) {
    //     printf("Fail to open input file.\n");
    //     close(socket_fd);
    //     exit(1);
    // }

    // struct stat stat_buf;
    // int rc = stat(sort_out_name.c_str(), &stat_buf);
    // double file_size = rc == 0 ? stat_buf.st_size : -1;
    // printf("file size: %.2f GB\n", file_size / 1024.0 / 1024.0 / 1024.0);

    // bzero(buffer, BUFFER_SIZE);

    // while (!input.eof()) {
    //     input.read(buffer, BUFFER_SIZE);
    //     int read_size = input.gcount();
    //     int send_size = send(socket_fd, buffer, read_size, 0);
    //     if (send_size < 0) {
    //         printf("Fail to send file to server.\n");
    //         close(socket_fd);
    //         exit(1);
    //     }
    //     printf("Send %d bytes to server\n", send_size);
    //     bzero(buffer, BUFFER_SIZE);
    // }

    // // send 0 bytes to server to indicate the end of file
    // int send_size = send(socket_fd, buffer, 0, 0);
    // if (send_size < 0) {
    //     printf("Fail to send file to server.\n");
    //     close(socket_fd);
    //     exit(1);
    // }
    // printf("Send %d bytes to server\n", send_size);

    // close input file
    // input.close();

    // close socket
    close(socket_fd);

    return 0;
}
