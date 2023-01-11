/**
 * Server splits the large file into small parts and transfers then to slaves.
 * Later will receive the sorted parts from slaves and merge them into one file.
 * The sorting processes happen concurrently.
 * Here we can see the overhead of transferring files.
 *
 * usage: ./server input output
 */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12000

using namespace std;

// vector for client socket fd
vector<int> client_fds;

int main(int argc, char const* argv[]) {
    if (argc != 3) {
        printf("usage: ./server input output\n");
        exit(1);
    }

    string in_name = argv[1];
    string out_name = argv[2];

    // create socket, AF_INET = IPv4, SOCK_STREAM = TCP
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        printf("Fail to create a socket.");
    }


    // set server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;           // IPv4
    server_addr.sin_port = htons(SERVER_PORT);  // port
    server_addr.sin_addr.s_addr = INADDR_ANY;   // Any IP address

    // bind socket to address
    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Fail to bind socket to address.");
        close(socket_fd);
        exit(1);
    }

    printf("Server is listening on port %d\n", SERVER_PORT);

    // listen for incoming connections
    if (listen(socket_fd, 5) < 0) {
        printf("Fail to listen.");
        close(socket_fd);
        exit(1);
    }

    // wait until we have 5 client connections
    while (client_fds.size() < 1) {
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
    input.open(in_name, ios::in | ios::binary);
    if (!input.good()) {
        printf("Fail to open input file.\n");
        close(socket_fd);
        exit(1);
    }

    struct stat stat_buf;
    int rc = stat(in_name.c_str(), &stat_buf);
    double file_size = rc == 0 ? stat_buf.st_size : -1;
    printf("file size: %.2f GB\n", file_size / 1024.0 / 1024.0 / 1024.0);

    // divide the file into 5 parts and send to clients
    int fd_index = 0;
    char* buffer = new char[4000];
    while (!input.eof()) {
        input.read(buffer, 4000);
        int read_size = input.gcount();
        int send_size = send(client_fds[fd_index], buffer, read_size, 0);
        if (send_size < 0) {
            printf("Fail to send file to client.\n");
            close(socket_fd);
            exit(1);
        }
        printf("Send %d bytes to client %d\n", send_size, fd_index);
        fd_index = (fd_index + 1) % 5;
        bzero(buffer, 4000);
    }

    // close input file
    input.close();

    // receive sorted parts from clients

    // close socket
    close(socket_fd);

    /* code */
    return 0;
}
