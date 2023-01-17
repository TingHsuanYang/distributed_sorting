#include <string>

class Slave {
   public:
    Slave(std::string server_ip, int port);
    ~Slave();
    int run();
    void receive(int socket_fd, std::string input_name);
    void sendback(int socket_fd, std::string sort_out_name);

   private:
    std::string server_ip;
    int port;
};