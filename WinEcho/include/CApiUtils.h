#pragma once
#include <memory>

template<typename T>
std::shared_ptr<T>* getHandle(void* ptr) {
	if(ptr == nullptr)
		return nullptr;

	return (std::shared_ptr<T>*)ptr;
}

template<typename T>
void* genHandle(T* ptr) {
	return (void*)new std::shared_ptr<T>(ptr);
}

template<typename T>
void* genHandleCopy(std::shared_ptr<T> ptr) {
	std::shared_ptr<T>* handle = new std::shared_ptr<T>(nullptr);
	(*handle) = ptr;
	return (void*)handle;
}