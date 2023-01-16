/**
 * Server splits the large file into small parts and transfers then to slaves.
 * Later will receive the sorted parts from slaves and merge them into one file.
 * The sorting processes happen concurrently.
 * Here we can see the overhead of transferring files.
 *
 * usage: ./server input output
 */
#include "master.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <thread>

#define BUFFER_SIZE 1000

using namespace std;

Master::Master(int port, int slaveNum, string inputName, string outputName)
    : port(port),
      slaveNum(slaveNum),
      inputName(inputName),
      outputName(outputName) {
}


void thread_send(string inputName, long long pos, long long size, int client_fd, int client_idx){
    ifstream input;
    input.open(inputName, ios::in | ios::binary);
    if (!input.good())
    {
        printf("Fail to open input file.\n");
        close(client_fd);
        exit(1);
    }

    int fd_index = 0;
    char *buffer = new char[BUFFER_SIZE];
    while (size > 0)
    {
        input.seekg(pos);
        if(size > BUFFER_SIZE) {
            input.read(buffer, BUFFER_SIZE);
        } else {
            input.read(buffer, size);
        }
        int read_size = input.gcount();
        int send_size = send(client_fd, buffer, read_size, 0);
        if (send_size < 0)
        {
            printf("Fail to send file to client.\n");
            close(client_fd);
            exit(1);
        }
        printf("Send %d bytes to client %d\n", send_size, client_idx);
        bzero(buffer, BUFFER_SIZE);
        size -= read_size;
    }

    int send_size = send(client_fd, buffer, 0, 0);
    if (send_size < 0)
    {
        printf("Fail to send file to client.\n");
        close(client_fd);
        exit(1);
    }
    printf("Send %d bytes to client %d\n", send_size, client_idx);

    // close input file
    input.close();

    // receive sorted parts from clients
    // receive file and write to disk
    string out_name = "slave";
    out_name.append(to_string(client_idx)).append(".input");
    ofstream output(out_name, ios::out | ios::binary);
    char buffer[BUFFER_SIZE];
    ssize_t len;
    while (true)
    {
        len = recv(client_fd, buffer, sizeof(buffer), 0);
        if (len < 0)
        {
            printf("Fail to receive file.\n");
            close(client_fd);
            exit(1);
        }
        else if (len == 0)
        {
            break;
        }
        printf("Received %ld bytes.\n", len);
        output.write(buffer, len);
    }
    output.close();
}

int Master::run()
{
    // create socket, AF_INET = IPv4, SOCK_STREAM = TCP
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        printf("Fail to create a socket.");
    }

    // set reuse address
    int reuse_addr = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse_addr, sizeof(reuse_addr));

    // set server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;          // IPv4
    server_addr.sin_port = htons(port);        // port
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Any IP address

    // bind socket to address
    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Fail to bind socket to address.");
        close(socket_fd);
        exit(1);
    }

    printf("Server is listening on port %d\n", port);

    // listen for incoming connections
    if (listen(socket_fd, 5) < 0) {
        printf("Fail to listen.");
        close(socket_fd);
        exit(1);
    }

    // wait until we have 5 client connections
    while (client_fds.size() < slaveNum) {
        // accept incoming connection
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_fd < 0) {
            printf("Fail to accept incoming connection.");
            close(socket_fd);
            exit(1);
        }

        // add client socket fd to vector
        client_fds.push_back(client_fd);
        printf("Get connection from client: [%s:%d]\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    }

    // send file to clients
    ifstream input;
    input.open(inputName, ios::in | ios::binary);
    if (!input.good()) {
        printf("Fail to open input file.\n");
        close(socket_fd);
        exit(1);
    }

    struct stat stat_buf;
    int rc = stat(inputName.c_str(), &stat_buf);
    double file_size = rc == 0 ? stat_buf.st_size : -1;
    printf("file size: %.2f GB\n", file_size / 1024.0 / 1024.0 / 1024.0);

    // divide the file into 5 parts and send to clients
    long long recNum = file_size/100;
    // records number per slave
    long long partRecNum = recNum/slaveNum;
    int left = (int)recNum%slaveNum;


    vector<thread> threads;
    long long currPos = 0;
    for(int i = 0; i < slaveNum; i++){
        long long size = partRecNum * 100;
        if(left > 0){
            size += 100;
            left--;
        }
        threads.push_back(thread(thread_send, inputName, currPos, size, client_fds[i], i));
        currPos += size;
    }

    for (int i = 0; i < threads.size(); i++) {
        threads[i].join();
    }

    threads.clear();
    // int fd_index = 0;
    // char *buffer = new char[BUFFER_SIZE];
    // while (!input.eof())
    // {
    //     input.read(buffer, BUFFER_SIZE);
    //     int read_size = input.gcount();
    //     int send_size = send(client_fds[fd_index], buffer, read_size, 0);
    //     if (send_size < 0)
    //     {
    //         printf("Fail to send file to client.\n");
    //         close(socket_fd);
    //         exit(1);
    //     }
    //     printf("Send %d bytes to client %d\n", send_size, fd_index);
    //     fd_index = (fd_index + 1) % slaveNum;
    //     bzero(buffer, BUFFER_SIZE);
    // }

    // // send 0 bytes to slaves to indicate the end of file
    // for (int i = 0; i < slaveNum; i++)
    // {
    //     int send_size = send(client_fds[i], buffer, 0, 0);
    //     if (send_size < 0)
    //     {
    //         printf("Fail to send file to client.\n");
    //         close(socket_fd);
    //         exit(1);
    //     }
    //     printf("Send %d bytes to client %d\n", send_size, i);
    // }

    // // close input file
    // input.close();

    // receive sorted parts from clients

    // closing the connected socket
    for(int i = 0; i < slaveNum; i++){
        close(client_fds[i]);
    }
    // closing the listening socket
    shutdown(socket_fd, SHUT_RDWR);

    /* code */
    return 0;
}
