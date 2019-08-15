#include "EtcdManager.h"

#include <unistd.h>

void heartBeat(EtcdManager &etcdMgr)
{
    for (int i = 0; i < 10; i++)
    {
        etcdMgr.set("/apRouters/1", "1.1.1.1:12345", 10);
        etcdMgr.set("/apRouters/2", "2.2.2.2:12345", 10);        
        sleep(3);
    }
}

int main(int argc, char *argv[])
{
    std::vector<std::string> address;
    address.push_back("11.11.11.11:20002");
    address.push_back("22.22.22.22:20002");
    address.push_back("33.33.33.33:20002");
    
    EtcdManager etcdMgr(address);

    heartBeat(etcdMgr);
    etcdMgr.deleteKey("/apRouters/1");
    etcdMgr.deleteKey("/apRouters/2");    
    
    return 0;
}
