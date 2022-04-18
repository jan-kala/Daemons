//
// Created by Jan Kala on 18.04.2022.
//

#ifndef HTTPIPJOINER_PAIRINGCACHE_H
#define HTTPIPJOINER_PAIRINGCACHE_H

#include <string>
#include <map>
#include <thread>

class PairingCache {
public:
    PairingCache();
    ~PairingCache();

    void addNew(std::string url);
    void pair(std::string url);

    std::atomic_flag cacheLock = ATOMIC_FLAG_INIT;
    std::map<std::string, std::string> cache;

    void cleanerWorker();
    std::thread cleaner_thread;
    std::condition_variable cleaner_cv;
    std::mutex cleaner_mutex;
};


#endif //HTTPIPJOINER_PAIRINGCACHE_H
