#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage {
public:
    SimpleLRU(size_t max_size = 1024) : _max_size(max_size), _cur_size(0),
                                        _lru_tail(nullptr), _lru_head(nullptr) {}

    ~SimpleLRU() {
        _lru_index.clear(); 

        while (_lru_head != nullptr) {
          auto next = std::move(_lru_head->next);
          _lru_head.reset(nullptr);
          _lru_head = std::move(next);
        }
        /*/
        while (_lru_tail != nullptr){
            auto ptr = _lru_tail->prev;
            ptr->next.reset(nullptr);
            _lru_tail = ptr;
        }
        /*/
    }

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
    // LRU cache node
    using lru_node = struct lru_node {
        const std::string key;
        std::string value;
        lru_node * prev;
        std::unique_ptr<lru_node> next;
    };

    // move the most recently used element to tail
    void move_tail(SimpleLRU::lru_node &node);

    // add new element to the storage
    bool add_element(const std::string &key, const std::string &value);
    
    // update existing node
    bool update_element(SimpleLRU::lru_node &node, const std::string &val);

    // delete existing node
    bool delete_node(SimpleLRU::lru_node &node);

    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be less the _max_size
    std::size_t _max_size;
    std::size_t _cur_size;

    // Main storage of lru_nodes, elements in this list ordered descending by "freshness": in the head
    // element that wasn't used for longest time.
    //
    // List owns all nodes
    std::unique_ptr<lru_node> _lru_head;
    //pointer to the last element of the storage
    lru_node * _lru_tail; 

    // Index of nodes from list above, allows fast random access to elements by lru_node#key
    std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>, std::less<const std::string>> _lru_index;
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
