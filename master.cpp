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

#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#define BUFFER_SIZE 1000
#define DATA_SIZE 100

using namespace std;

mutex mtx;

struct HeapNode {
    int index;
    char value[DATA_SIZE];
    HeapNode(int index, char* value) {
        this->index = index;
        memcpy(this->value, value, DATA_SIZE);
    }
};

// comparator for the heap
struct Comparator {
    bool operator()(const HeapNode* node1, const HeapNode* node2) {
        return memcmp(node1->value, node2->value, DATA_SIZE) > 0;
    }
};

Master::Master(int port, int slaveNum, string inputName, string outputName)
    : port(port),
      slaveNum(slaveNum),
      inputName(inputName),
      outputName(outputName) {}

Master::~Master() {}

void Master::thread_send(string inputName, long long pos, long long size, int client_fd, int client_idx) {
    printf("Send file to client %d...\n", client_idx);
    ifstream input;
    input.open(inputName, ios::in | ios::binary);
    if (!input.good()) {
        printf("Fail to open input file.\n");
        close(client_fd);
        exit(1);
    }

    // calculate the time of sending file
    auto start = chrono::high_resolution_clock::now();

    // read the file chunk and send to client
    char* buffer = new char[BUFFER_SIZE];
    input.seekg(pos);
    while (size > 0) {
        if (size > BUFFER_SIZE) {
            input.read(buffer, BUFFER_SIZE);
        } else {
            input.read(buffer, size);
        }
        int read_size = input.gcount();
        int send_size = send(client_fd, buffer, read_size, 0);
        if (send_size < 0) {
            printf("Fail to send file to client.\n");
            close(client_fd);
            exit(1);
        }
        // printf("Send %d bytes to client %d\n", send_size, client_idx);
        memset(buffer, 0, BUFFER_SIZE);
        size -= read_size;
    }

    // send 0 to client to indicate the end of file
    int send_size = send(client_fd, buffer, 0, 0);
    if (send_size < 0) {
        printf("Fail to send file to client.\n");
        close(client_fd);
        exit(1);
    }

    // calculate the time of sending file
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    printf("Send file to client %d in %.2f seconds.\n", client_idx, duration.count() * 1.0 / 1000000);

    // close input file
    input.close();

    // free buffer
    delete[] buffer;

    // close client socket
    close(client_fd);
}

void Master::thread_recv(int socket_fd, int client_idx) {
    // accept incoming connection
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_fd < 0) {
        printf("Fail to accept incoming connection.");
        close(socket_fd);
        exit(1);
    }
    printf("Get connection from client: [%s:%d]\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // calculate the time of receiving file
    auto start = chrono::high_resolution_clock::now();

    printf("Receive file from client %d...\n", client_idx);

    // receive sorted parts from clients
    string part_name = "slave";
    part_name.append(to_string(client_idx)).append(".part");

    // add mutex lock
    mtx.lock();
    part_names.push_back(part_name);
    mtx.unlock();

    ofstream output(part_name, ios::out | ios::binary);
    char* buffer = new char[4096];
    ssize_t len;
    while (true) {
        len = recv(client_fd, buffer, sizeof(buffer), 0);
        if (len < 0) {
            printf("Fail to receive file.\n");
            close(client_fd);
            exit(1);
        } else if (len == 0) {
            break;
        }
        // printf("Received %ld bytes.\n", len);
        output.write(buffer, len);
    }
    output.close();

    // calculate the time of receiving file
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    printf("Finish receiving file from client %d in %.2f seconds.\n", client_idx, duration.count() * 1.0 / 1000000);

    // free buffer
    delete[] buffer;

    // close client socket
    close(client_fd);
}

// k-way merge
void Master::merge() {
    printf("Merge the sorted parts...\n");

    // open the output file
    ofstream output(outputName, ios::out | ios::binary);
    priority_queue<HeapNode*, vector<HeapNode*>, Comparator> heap;

    // calculate the time of merging
    auto start = chrono::high_resolution_clock::now();

    // open all the part files
    vector<ifstream> part_files;
    for (int i = 0; i < part_names.size(); i++) {
        ifstream part_file(part_names[i], ios::in | ios::binary);
        part_files.push_back(move(part_file));
    }

    // read the first record of each part file
    char* buffer = new char[DATA_SIZE];
    for (int i = 0; i < part_files.size(); i++) {
        part_files[i].read(buffer, DATA_SIZE);
        if (part_files[i].gcount() > 0) {
            HeapNode* node = new HeapNode(i, buffer);
            heap.push(node);
        }
    }

    // get the smallest record from the heap and write it to the output file
    while (!heap.empty()) {
        HeapNode* node = heap.top();
        heap.pop();
        output.write(node->value, DATA_SIZE);

        // read the next record of the part file
        // if the part file is not empty, push the record to the heap
        part_files[node->index].read(buffer, DATA_SIZE);
        if (part_files[node->index].gcount() > 0) {
            HeapNode* next_node = new HeapNode(node->index, buffer);
            heap.push(next_node);
        }

        delete node;
    }

    // free buffer
    delete[] buffer;

    // close all the part files
    for (int i = 0; i < part_files.size(); i++) {
        part_files[i].close();
    }
    output.close();

    // calculate the time of merging
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    printf("Merge the sorted parts in %.2f seconds.\n", duration.count() * 1.0 / 1000000);
}

int Master::run() {
    // calculate the total time of running
    auto start = chrono::high_resolution_clock::now();

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
    if (listen(socket_fd, slaveNum) < 0) {
        printf("Fail to listen.");
        close(socket_fd);
        exit(1);
    }

    // wait until we have reached the number of slaves
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

    struct stat stat_buf;
    int rc = stat(inputName.c_str(), &stat_buf);
    double file_size = rc == 0 ? stat_buf.st_size : -1;
    printf("file size: %.2f GB\n", file_size / 1024.0 / 1024.0 / 1024.0);

    // divide the file into 5 parts and send to clients
    long long recNum = file_size / 100;
    // records number per slave
    long long partRecNum = recNum / slaveNum;
    int remainRecNum = (int)recNum % slaveNum;

    vector<thread> threads;
    long long currPos = 0;
    for (int i = 0; i < slaveNum; i++) {
        long long size = partRecNum * 100;
        if (remainRecNum > 0) {
            size += 100;
            remainRecNum--;
        }
        threads.push_back(thread(&Master::thread_send, this, inputName, currPos, size, client_fds[i], i));
        currPos += size;
    }

    for (int i = 0; i < threads.size(); i++) {
        threads[i].join();
    }
    threads.clear();

    // remove the original input file
    remove(inputName.c_str());

    // clear client socket fds
    client_fds.clear();

    // receive sorted parts from clients
    for (int i = 0; i < slaveNum; i++) {
        threads.push_back(thread(&Master::thread_recv, this, socket_fd, i));
    }

    for (int i = 0; i < threads.size(); i++) {
        threads[i].join();
    }

    threads.clear();

    // merge all parts from slaves
    merge();

    // remove the part files
    for (int i = 0; i < part_names.size(); i++) {
        remove(part_names[i].c_str());
    }

    // closing the listening socket
    close(socket_fd);

    // calculate the total time of running
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    printf("Total running time: %.2f seconds for file size: %.2f GB with %d slaves\n", duration.count() * 1.0 / 1000000, file_size / 1024.0 / 1024.0 / 1024.0, slaveNum);

    return 0;
}
