#include <string>
#include <vector>
class Master {
   public:
    Master(int port, int slaveNum, std::string inputName, std::string outputName);
    int run();

   private:
    int port;
    int slaveNum;
    std::string inputName;
    std::string outputName;
    std::vector<int> client_fds;
};