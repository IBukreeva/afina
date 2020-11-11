

#include "StripedLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool StripedLRU::Put(const std::string &key, const std::string &value) { 
    return _shards[_hash_func(key) % _shards_number]->Put(key, value);
}

// See MapBasedGlobalLockImpl.h
bool StripedLRU::PutIfAbsent(const std::string &key, const std::string &value) { 
    return _shards[_hash_func(key) % _shards_number]->PutIfAbsent(key, value);
}

// See MapBasedGlobalLockImpl.h
bool StripedLRU::Set(const std::string &key, const std::string &value) {
    return _shards[_hash_func(key) % _shards_number]->Set(key, value);
}

// See MapBasedGlobalLockImpl.h
bool StripedLRU::Delete(const std::string &key) { 
    return _shards[_hash_func(key) % _shards_number]->Delete(key);
}

// See MapBasedGlobalLockImpl.h
bool StripedLRU::Get(const std::string &key, std::string &value) { 
    return _shards[_hash_func(key) % _shards_number]->Get(key, value);
}

} // namespace Backend
} // namespace Afina
