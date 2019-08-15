#include "EtcdManager.h"

#include <unistd.h>

int main(int argc, char *argv[])
{
    std::vector<std::string> address;
    address.push_back("11.11.11.11:20002");
    address.push_back("22.22.22.22:20002");
    address.push_back("33.33.33.33:20002");

    EtcdManager etcdWatcher(address);
    etcdWatcher.startMultiWatch("/apRouters");

    while (true)
    {
        sleep(1);
    }
    
    return 0;
}
