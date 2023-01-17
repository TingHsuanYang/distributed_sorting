#include <string>
#include <vector>

class Master {
   public:
    Master(int port, int slaveNum, std::string inputName, std::string outputName);
    ~Master();
    int run();
    void thread_send(std::string inputName, long long pos, long long size, int client_fd, int client_idx);
    void thread_recv(int socket_fd, int client_idx);
    void merge();

   private:
    int port;
    int slaveNum;
    std::string inputName;
    std::string outputName;
    std::vector<int> client_fds;
    std::vector<std::string> part_names;
};