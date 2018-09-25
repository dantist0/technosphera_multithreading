#include "SimpleLRU.h"
#include <exception>

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    
	int data_size = key.size() + value.size();
    if (data_size > _max_size)
        return false;

	_free_bytes -= data_size;

	if (_free_bytes < 0)
		_free_bytes += _release_memory(-_free_bytes);

    auto node = new lru_node();
    node->key = key;
    node->value = value;

    if (_lru_head.get() == nullptr) {
       
        node->next = nullptr;
        node->prev = nullptr;

        _lru_head = std::unique_ptr<lru_node>(node);
        _lru_tail = node;
        return true;
    }

	_lru_tail->prev = node;
    auto tmp = _lru_tail;
	_lru_tail = node;
    _lru_tail->next = std::unique_ptr<lru_node>(tmp);

	_lru_index.emplace(std::cref(key), std::ref(*node));

    return true;
}


// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    if (_lru_index.find(std::ref(key)) == _lru_index.end())
        return Put(key, value);
    else 
		return false; 
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) { 
	int data_size = key.size() + value.size();
    if (data_size > _max_size)
        return false;

	if (Delete(key))
        return Put(key, value);
    else
        return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    if (_lru_index.find(std::ref(key)) == _lru_index.end())
        return false;

    lru_node &node = _lru_index.at(std::cref(key)).get();
    _lru_index.erase(std::cref(key));

    int data_size = node.key.size() + node.value.size();

    std::unique_ptr<lru_node> &next = node.next;
    lru_node *prev = node.prev;

	if (prev)
		prev->next = std::move(next);

    if (next)
		next->prev = prev;

	_free_bytes += data_size;

	return true;

}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {

    if (_lru_index.find(std::ref(key)) == _lru_index.end())
        return false;

	lru_node &node = _lru_index.at(std::cref(key)).get();
    value = node.value; // WHY _lru_index[std::ref(key)] DON'T WORK???????????????????????????????

	//нужно доделать перемещение обьекта в tale


	std::unique_ptr<lru_node> &next = node.next;
    lru_node *prev = node.prev;

    if (prev)
        prev->next = std::move(next);

    if (next)
        next->prev = prev;

	_lru_tail->prev = &node;
    auto tmp = _lru_tail;
    _lru_tail = &node;
    _lru_tail->next = std::unique_ptr<lru_node>(tmp);




    return true;
}

} // namespace Backend
} // namespace Afina
