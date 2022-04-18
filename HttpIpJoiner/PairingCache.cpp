//
// Created by Jan Kala on 18.04.2022.
//

#include "PairingCache.h"

#include <iostream>

PairingCache::PairingCache() {
//    cleaner_thread = std::thread(&PairingCache::cleanerWorker, this);
}

PairingCache::~PairingCache() {
//    cleaner_cv.notify_all();
//    if (cleaner_thread.joinable()){
//        cleaner_thread.join();
//    }
}

void PairingCache::addNew(std::string url) {
    while (cacheLock.test_and_set(std::memory_order_acquire));

    cache[url] = std::string();
    std::cout << "ADDING  -- " << url << std::endl;

    cacheLock.clear(std::memory_order_release);
}

void PairingCache::pair(std::string url) {
    while (cacheLock.test_and_set(std::memory_order_acquire));

    std::cout << "PAIRING -- " << url << std::endl;
    for (auto &item: cache){
        auto res = std::mismatch(url.begin(), url.end(), item.first.begin());

        if (res.first == url.end()){
            std::cout << item.first << " -- MATCHED!" << std::endl;
            cache.erase(item.first);
            break;
        }
    }

    cacheLock.clear(std::memory_order_release);
}

void PairingCache::cleanerWorker() {
    std::unique_lock<std::mutex> lock(cleaner_mutex);
    std::chrono::seconds sec(10);

    while (cleaner_cv.wait_for(lock, sec) == std::cv_status::timeout) {
        while (cacheLock.test_and_set(std::memory_order_acquire));

        cacheLock.clear(std::memory_order_release);
    }
}



