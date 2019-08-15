#ifndef ETCDMANAGER_H
#define ETCDMANAGER_H

#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>

#include "./cetcd/cetcd.h"

class EtcdManager
{
public:
    // 集群节点
    struct Member
    {
        std::string  m_key;
        uint32_t     m_ip;
        uint16_t     m_port;
        bool         m_isInCluster;
        uint32_t     m_ttl;
    };

    // 集群
    using Cluster = std::unordered_map<std::string, Member>;

    enum Action
    {
        Set = 0,
        Get,
        Update,
        Create,
        Delete,
        Expire,
        CompareAndSwap,    
        CompareAndDelete,    
    };
    
    EtcdManager(const std::vector<std::string> &address);
    ~EtcdManager();

    EtcdManager(const EtcdManager &) = delete;
    EtcdManager &operator=(const EtcdManager &) = delete;
    
    void get(const std::string &key);

    void set(const std::string &key, const std::string value, uint32_t ttl);    

    void startMultiWatch(const std::string &services);

    void deleteKey(const std::string &key);
    
private:
    void addMember(cetcd_response_node *node);

    void updateMember(cetcd_response_node *node, Member &m);

    static int watchCallback(void *userdata, cetcd_response *resp);
    
    std::vector<std::string>  m_address;
    cetcd_array               m_etcdAddrs;
    cetcd_client              m_etcdClient;
    cetcd_array               m_watchers;
    cetcd_watch_id            m_watchId;
    Cluster                   m_cluster;
    uint32_t                  m_watchModifiedIndex;
};

#endif /* ETCDMANAGER_H */
