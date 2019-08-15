# etcd-service-discovery
etcd 服务发现 demo - C++ 版本

## 编译
```bash
$ cd cetcd
$ make
$ cd ..
$ g++ -std=c++11 -o master master.cpp EtcdManager.cpp cetcd/libcetcd.a -lpthread -lcurl
$ g++ -std=c++11 -o node node.cpp EtcdManager.cpp cetcd/libcetcd.a -lpthread -lcurl
$ ./master &
$ ./node &
```
