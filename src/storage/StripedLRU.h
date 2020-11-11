#ifndef AFINA_STORAGE_STRIPED_LRU_H
#define AFINA_STORAGE_STRIPED_LRU_H

#include <map>
#include <functional>
#include <string>
#include <memory>
#include <mutex>
#include <vector>

#include "ThreadSafeSimpleLRU.h"
#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Based on ThreadSafeSimplLRU
 * Consists of some copies of ThreadSafeSimplLRU
 * every part works independently  
 */
class StripedLRU : public Afina::Storage {
public:
    explicit StripedLRU(size_t shards_number = 4,
                        size_t max_storage_size = 1024*1024*8): _shards_number(shards_number),
                                                        _max_storage_size(max_storage_size)
    {
        size_t shard_size = _max_storage_size/_shards_number;
        
        if(_max_storage_size/(_shards_number) < 1024*1024){
            throw std::runtime_error("Storage size too small");
        }

        for (size_t i=0; i<_shards_number; i++){
            _shards.emplace_back(new ThreadSafeSimplLRU(shard_size));
        }

    }
    
    ~StripedLRU() {};

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) override;



private:
    size_t _shards_number;
    //size_t _max_shard_size;
    size_t _max_storage_size;
    
    std::hash<std::string> _hash_func;
    std::vector< std::unique_ptr<ThreadSafeSimplLRU>> _shards;
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_STRIPED_SIMPLE_LRU_H
