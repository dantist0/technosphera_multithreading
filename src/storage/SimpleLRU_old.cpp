#include "SimpleLRU.h"
#include <exception>
#include <iostream>

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    if (_lru_index.find(key) == _lru_index.end()) {
        return PutIfAbsent(key, value);
    } else {
        return Set(key, value);
	}
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    if (_lru_index.find(key) != _lru_index.end())
        return false;

	std::size_t size = key.size() + value.size();

	if (size > _max_size)
		return false;

	_current_size += size;
        while (_current_size > _max_size)
            if (!_delete_old())
                return false;
            

    lru_node *node = new lru_node(key, value);
      
    
	if (_lru_tail == nullptr)
        _lru_tail = node;

	_add_to_head(node);

	_lru_index.insert(
            std::pair<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>>(node->key, *node));
   

    return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    if (_lru_index.find(key) == _lru_index.end())
        return false;

    Delete(key);

    return PutIfAbsent(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    if (_lru_index.find(key) == _lru_index.end())
        return false;

    lru_node &node = _lru_index.at(key);
    std::size_t size = node.key.size() + node.value.size();

    _remove_from_list(&node);
    _lru_index.erase(key);

    delete &node;

    _current_size -= size;

    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {

    if (_lru_index.find(key) == _lru_index.end())
        return false;

    lru_node &getted_node = _lru_index.at(key).get();

    value = getted_node.value;

    if (&getted_node != _lru_head) {
        _remove_from_list(&getted_node);
        _add_to_head(&getted_node);
    }

    return true;
}

bool SimpleLRU::_delete_old() {
    if (_lru_tail == nullptr)
        return false;

    _current_size -= _lru_tail->key.size() + _lru_tail->value.size();

    _lru_index.erase(_lru_tail->key);

	if (_lru_tail->next) {
		_lru_tail = _lru_tail->next;
		_lru_tail->prev.reset();
    } else {
		delete _lru_tail;
        _lru_tail = nullptr;
        _lru_head = nullptr;
	}

	return true;
}

void SimpleLRU::_add_to_head(lru_node *node) {
    if (_lru_head == nullptr) {
        _lru_head = node;
        return;
	}

    _lru_head->next = node;
    node->prev = std::unique_ptr<lru_node>(_lru_head);
    _lru_head = node;
}

void SimpleLRU::_remove_from_list(lru_node *node) {
    lru_node *next = node->next;
    std::unique_ptr<lru_node> &prev = node->prev;

	if (next == nullptr && prev == nullptr) {
        _lru_head = nullptr;
        _lru_tail = nullptr;
	}else if (next == nullptr) {
        _lru_head = node->prev.get();
        node->prev->next = nullptr;
    } else if (prev == nullptr) {
        _lru_tail = node->next;
        node->next->prev = nullptr;
    } else {
        prev->next = next;
        next->prev.swap(prev);
        prev.release();
    }
}

} // namespace Backend
} // namespace Afina
