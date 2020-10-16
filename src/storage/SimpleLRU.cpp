#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

//add to the storage the element which exactly is not in storage
bool SimpleLRU::add_element(const std::string &key, const std::string &value){

    std::size_t addsize = key.length() + value.length();
    if( addsize > _max_size){ 
        return false; //no chances to put the element to the storage
    }

    while( addsize + _cur_size > _max_size){
        delete_node(std::ref(*_lru_head));
    }

    if (_lru_head == nullptr){
        SimpleLRU::lru_node *node = new lru_node{key, value, nullptr, nullptr};
        _lru_head = std::move(std::unique_ptr<lru_node>(node));
        _lru_index.emplace(std::ref(node->key), std::ref(*node));
        _lru_tail = node;
    }
    else{
        SimpleLRU::lru_node *node = new lru_node{key, value, _lru_tail, nullptr};
        _lru_tail->next = std::move(std::unique_ptr<lru_node>(node));
        _lru_index.emplace(std::ref(node->key), std::ref(*node));
        _lru_tail = node;
    }
    _cur_size += addsize;

    return true;
}

//update value of the exactly existing element
bool SimpleLRU::update_element(SimpleLRU::lru_node &node, const std::string &value){

    size_t newValSize = value.length();
    size_t oldValSize = node.value.length();

    if(node.key.length()+ newValSize > _max_size){
        return false; //а при обращении к элементу с неудачной попыткой замены нужно перемещать его 
    }

    move_tail(node);

    while( (_cur_size + newValSize - oldValSize) > _max_size){
        delete_node(std::ref(*_lru_head));
    }

    _cur_size = _cur_size + newValSize - oldValSize;
    node.value = value;
    return true;
}


//move the most recently used element to the tail
void SimpleLRU::move_tail(SimpleLRU::lru_node &node){

    if( node.next == nullptr){ //already tail
        return;
    }
    else if( node.prev == nullptr){ //head element
        SimpleLRU::lru_node *ptr = node.next->prev;
        _lru_tail->next = std::move(_lru_head);
        _lru_tail->next->prev = _lru_tail;
        _lru_head = std::move(ptr->next);
        _lru_head->prev = nullptr;
        _lru_tail = ptr;
    }
    else{ //in the middle
        SimpleLRU::lru_node *ptr = node.next->prev;
        ptr->next->prev = ptr->prev;
        _lru_tail->next = std::move(ptr->prev->next);
        ptr->prev->next = std::move(ptr->next);
        ptr->prev = _lru_tail;
        _lru_tail = ptr;
    }
    
    
    return;
}

//delete node that exactly exist
bool SimpleLRU::delete_node(SimpleLRU::lru_node &node){

    _cur_size -= (node.key.length() + node.value.length()); 
    _lru_index.erase(node.key);

    if( node.next != nullptr ){
        if(node.prev != nullptr ){ //in the middle
            SimpleLRU::lru_node * ptr = node.prev;
            ptr->next = std::move(node.next);
            ptr->next->prev = ptr;
        }
        else{ //head, not tail
            _lru_head = std::move(_lru_head->next);
            _lru_head->prev = nullptr;
        }
    }
    else{
        if(node.prev != nullptr ){ // tail, not head
            _lru_tail = _lru_tail->prev;
            node.prev->next = nullptr;
        }
        else{ //head and tail at the same time
            _lru_head = nullptr;
            _lru_tail = nullptr;
        }
    }

    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) { 
    
    auto found = _lru_index.find(key);

    if(found == _lru_index.end()){
        return add_element(key, value);
    }
    else{
        SimpleLRU::lru_node &node = found->second.get();
        return update_element(node, value);
    }

}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) { 
    
    auto found = _lru_index.find(key);
    if(found != _lru_index.end()){
        return false;
    }
    else{
        return add_element(key, value);
    }
    
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    
    auto found = _lru_index.find(key);

    if(found == _lru_index.end()){
        return false;
    }
    else{
        SimpleLRU::lru_node &node = found->second.get();
        return update_element(node, value);
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) { 
    
    auto found = _lru_index.find(key);

    if(found == _lru_index.end()){
        return false;
    }
    else{
        SimpleLRU::lru_node &node = found->second.get();
        return delete_node(node);
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) { 
    
    auto found = _lru_index.find(key);

    if(found == _lru_index.end()){
        return false;
    }
    else{
        value = found->second.get().value;
        move_tail(found->second.get());
        return true;
    }
}

} // namespace Backend
} // namespace Afina
