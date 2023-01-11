/**
 * Slave node receives the file from server and sort it.
 * After sorting, it sends the sorted part back to server.
 * The sorting processes happen concurrently.
 */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12000

using namespace std;

int main(int argc, char const* argv[]) {
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
    server_addr.sin_family = AF_INET;                    // IPv4
    server_addr.sin_port = htons(SERVER_PORT);           // port
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);  // server IP address
    socklen_t server_addr_len = sizeof(server_addr);

    // connect to server
    int err = connect(socket_fd, (struct sockaddr*)&server_addr, server_addr_len);
    if (err < 0) {
        printf("Fail to connect to server.\n");
        close(socket_fd);
        exit(1);
    }
    printf("Connected to server.\n");

    // receive file and write to disk
    string out_name = "part";
    ofstream output(out_name, ios::out | ios::binary);
    char buffer[100];
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
    return 0;
}
