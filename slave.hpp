#include <string>

class Slave {
   public:
    Slave(std::string server_ip, int port);
    ~Slave();
    int run();

   private:
    std::string server_ip;
    int port;
};