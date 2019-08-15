#include "EtcdManager.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

EtcdManager::EtcdManager(const std::vector<std::string> &address)
    : m_address(address),
      m_watchId(0),
      m_watchModifiedIndex(0)  
{
    cetcd_array_init(&m_etcdAddrs, m_address.size());
    for (auto &addr: m_address)
    {
        cetcd_array_append(&m_etcdAddrs, const_cast<char *>(addr.c_str())); 
    }
        
    cetcd_client_init(&m_etcdClient, &m_etcdAddrs);
    cetcd_array_init(&m_watchers, m_address.size());
}
    
EtcdManager::~EtcdManager()
{
    if (m_watchId != 0)
    {
        cetcd_multi_watch_async_stop(&m_etcdClient, m_watchId);
    }
    cetcd_client_destroy(&m_etcdClient);
        
    cetcd_array_destroy(&m_etcdAddrs);
    cetcd_array_destroy(&m_watchers);        
}

void EtcdManager::get(const std::string &key)
{
    cetcd_response *resp;
    resp = cetcd_get(&m_etcdClient, key.c_str());
    if (resp->err)
    {
        std::cout << "error :" << resp->err->ecode << ", "
                  << resp->err->message << ", " << resp->err->cause
                  << std::endl;
        return;
    }        
    
    cetcd_response_release(resp);        
}

void EtcdManager::set(const std::string &key, const std::string value, uint32_t ttl)
{
    cetcd_response *resp;
    resp = cetcd_set(&m_etcdClient, key.c_str(), value.c_str(), ttl);
    if (resp->err)
    {
        std::cout << "error :" << resp->err->ecode << ", "
                  << resp->err->message << ", " << resp->err->cause
                  << std::endl;
        return;
    }
    std::cout << "set key: " << key << ", value: " << value << ", ttl: " << ttl << std::endl;
    cetcd_response_release(resp);
}

void EtcdManager::deleteKey(const std::string &key)
{
    cetcd_response *resp;
    resp = cetcd_delete(&m_etcdClient, key.c_str());
    if (resp->err)
    {
        std::cout << "error :" << resp->err->ecode << ", "
                  << resp->err->message << ", " << resp->err->cause
                  << std::endl;
        return;
    }
    std::cout << "delete key: " << key << std::endl;
    cetcd_response_release(resp);    
}

void EtcdManager::addMember(cetcd_response_node *node)
{
    if (node == NULL)
    {
        return;
    }

    Member m;
    m.m_key = node->key;
    m.m_isInCluster = true;
    m.m_ttl = node->ttl;
        
    std::string addr = node->value;
    std::size_t found = addr.find(':');
        
    if (found != std::string::npos)
    {
        std::string ip = addr.substr(0, found);
        std::string port = addr.substr(found + 1);
            
        m.m_ip = inet_addr(ip.c_str());
        m.m_port = static_cast<uint16_t>(std::stoi(port));
    }

    m_cluster[m.m_key] = m;

    std::cout << "add member, addr: " << addr << ", ttl: " << m.m_ttl
              << ", m_isInCluster: " << m.m_isInCluster << std::endl;
}

void EtcdManager::updateMember(cetcd_response_node *node, Member &m)
{
    if (node == NULL)
    {
        return;
    }
        
    m.m_ttl = node->ttl;

    struct in_addr ipAddr;
    ipAddr.s_addr = m.m_ip;
    std::cout << "update member, addr: " << inet_ntoa(ipAddr) << ":"
              << std::to_string(m.m_port) << ", ttl: " << m.m_ttl
              << ", m_isInCluster: " << m.m_isInCluster << std::endl;
}

int EtcdManager::watchCallback(void *userdata, cetcd_response *resp)
{
    EtcdManager *etcdMgr = static_cast<EtcdManager *>(userdata);
        
    if (resp->err != NULL)
    {
        std::cout << "error :" << resp->err->ecode << ", "
                  << resp->err->message << ", " << resp->err->cause
                  << std::endl;
        return 0;
    }
 
    cetcd_response_node *node = resp->node;
    if (node == NULL)
    {
        return 0;
    }

    if (node->modified_index <= etcdMgr->m_watchModifiedIndex)
    {
        return 0;
    }
    etcdMgr->m_watchModifiedIndex = node->modified_index;
    
    std::string key = node->key;
    if (resp->action == Action::Set)
    {
        auto iter = etcdMgr->m_cluster.find(key);
        if (iter == etcdMgr->m_cluster.end())
        {
            etcdMgr->addMember(node);                    
        }
        else
        {
            Member &m = iter->second;
            m.m_isInCluster = true;
                    
            etcdMgr->updateMember(node, m);
        }
    }
    else if (resp->action == Action::Expire)
    {
        auto iter = etcdMgr->m_cluster.find(key);
        if (iter != etcdMgr->m_cluster.end())
        {
            Member &m = iter->second;
            m.m_isInCluster = false;
                
            etcdMgr->updateMember(node, m);
        }            
    }
    else if (resp->action == Action::Delete)
    {
        auto iter = etcdMgr->m_cluster.find(key);
        if (iter != etcdMgr->m_cluster.end())
        {
            Member &m = iter->second;
                
            struct in_addr ipAddr;
            ipAddr.s_addr = m.m_ip;
            std::cout << "delete member, addr: " << inet_ntoa(ipAddr) << ":"
                      << std::to_string(m.m_port) << std::endl;
                
            etcdMgr->m_cluster.erase(iter);   
        }
    }
    else
    {
        std::cout << "unsupport action: " << resp->action << std::endl;
    }
        
    return 0;
}    
    
void EtcdManager::startMultiWatch(const std::string &services)
{
    const uint64_t index = 0;
    const int recursive = 1;
    const int once = 0;
        
    for (auto &addr: m_address)
    {
        cetcd_watcher *watcher = cetcd_watcher_create(&m_etcdClient, services.c_str(),
                                                      index, recursive, once,
                                                      watchCallback, this);
        cetcd_add_watcher(&m_watchers, watcher);
    }
        
    m_watchId = cetcd_multi_watch_async(&m_etcdClient, &m_watchers);
}
