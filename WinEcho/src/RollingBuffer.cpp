#include "RollingBuffer.h"
#include <iostream>

namespace winecho {
	RollingBuffer::RollingBuffer(size_t bufferSize) : bufferSize(bufferSize) {
		data.resize(bufferSize, 0);

		availableData = 0;
		pointer = data.begin();

		readSemaphore = CreateSemaphore(NULL, 0, 1, NULL);
		writeSemaphore = CreateSemaphore(NULL, 0, 1, NULL);
		waitReadAmount = 0;
		waitWriteAmount = 0;
		dataMutex = CreateMutex(NULL, FALSE, NULL);
	}

	RollingBuffer::RollingBuffer() : RollingBuffer(1024 * 256) {
		
	}

	RollingBuffer::~RollingBuffer() {
		CloseHandle(readSemaphore);
		CloseHandle(writeSemaphore);
		CloseHandle(dataMutex);
	}

	bool RollingBuffer::Read(unsigned char* buffer, size_t amount, DWORD timeout) {
		if(amount > bufferSize)
			return false;

		while (amount > availableData) {
			waitReadAmount = amount;
			//std::cout << "Locking read" << std::endl;
			DWORD waitRes = WaitForSingleObject(readSemaphore, timeout);
			if (waitRes != WAIT_OBJECT_0) {
				waitReadAmount = 0;
				return false;
			}
		}
		waitReadAmount = 0;

		DWORD waitRes = 0;
		do {
			waitRes = WaitForSingleObject(dataMutex, timeout);
		} while (waitRes != WAIT_OBJECT_0);

		size_t myAvailableData = availableData;
		auto myPointer = pointer;

		size_t readData = 0;

		ReleaseMutex(dataMutex);

		if (data.end() - amount < myPointer) {
			size_t currentAmount = data.end() - myPointer;

			memcpy_s(buffer, currentAmount, &(*myPointer), currentAmount);
			
			buffer += currentAmount;
			readData += currentAmount;
			myPointer = data.begin();
			amount -= currentAmount;
		}
		
		memcpy_s(buffer, amount, &(*myPointer), amount);
		readData += amount;

		waitRes = 0;
		do {
			waitRes = WaitForSingleObject(dataMutex, timeout);
		} while (waitRes != WAIT_OBJECT_0);

		availableData -= readData;
		pointer = data.end() - readData <= pointer ? data.begin() + (readData - (data.end() - pointer)) : pointer + readData;

		ReleaseMutex(dataMutex);

		if (waitWriteAmount < bufferSize - availableData && waitWriteAmount > 0) {
			ReleaseSemaphore(writeSemaphore, 1, NULL);
		}

		return true;
	}

	bool RollingBuffer::Write(unsigned char* buffer, size_t amount, DWORD timeout) {
		if(amount > bufferSize)
			return false;

		while (amount >= bufferSize - availableData) {
			waitWriteAmount = amount;
			//std::cout << "Locking write" << std::endl;
			DWORD waitRes = WaitForSingleObject(writeSemaphore, timeout);
			if (waitRes != WAIT_OBJECT_0) {
				waitWriteAmount = 0;
				return false;
			}
		}
		waitWriteAmount = 0;

		DWORD waitRes = 0;
		do {
			waitRes = WaitForSingleObject(dataMutex, timeout);
		} while (waitRes != WAIT_OBJECT_0);

		size_t myAvailableData = availableData;
		auto myPointer = pointer;

		ReleaseMutex(dataMutex);

		size_t writtenData = 0;
		auto freeData = myPointer;
		if(data.end() - myPointer <= myAvailableData){
			freeData = data.begin() + (myAvailableData - (data.end() - myPointer));
		}
		else {
			freeData = myPointer + myAvailableData;
		}

		size_t freeDataOffset = freeData - data.begin();

		if (data.end() - amount <= freeData) {
			size_t currentAmount = data.end() - freeData;

			memcpy_s(&(*freeData), currentAmount, buffer, currentAmount);

			freeData = data.begin();
			writtenData += currentAmount;
			buffer += currentAmount;
			amount -= currentAmount;
		}

		memcpy_s(&(*freeData), amount, buffer, amount);
		writtenData += amount;

		waitRes = 0;
		do {
			waitRes = WaitForSingleObject(dataMutex, timeout);
		} while (waitRes != WAIT_OBJECT_0);

		availableData += writtenData;

		ReleaseMutex(dataMutex);

		if (availableData >= waitReadAmount && waitReadAmount > 0) {
			ReleaseSemaphore(readSemaphore, 1, NULL);
		}

		return true;
	}

	bool RollingBuffer::WriteValue(unsigned char value, size_t amount, DWORD timeout) {
		if (amount > bufferSize)
			return false;

		while (amount > bufferSize - availableData) {
			waitWriteAmount = amount;
			//std::cout << "Locking zero" << std::endl;
			DWORD waitRes = WaitForSingleObject(writeSemaphore, timeout);
			if (waitRes != WAIT_OBJECT_0) {
				waitWriteAmount = 0;
				return false;
			}
		}
		waitWriteAmount = 0;

		DWORD waitRes = 0;
		do {
			waitRes = WaitForSingleObject(dataMutex, timeout);
		} while (waitRes != WAIT_OBJECT_0);

		size_t myAvailableData = availableData;
		auto myPointer = pointer;

		ReleaseMutex(dataMutex);

		size_t writtenData = 0;
		auto freeData = myPointer;
		if (data.end() - myPointer <= myAvailableData) {
			freeData = data.begin() + (myAvailableData - (data.end() - myPointer));
		}
		else {
			freeData = myPointer + myAvailableData;
		}

		if (data.end() - amount <= freeData) {
			size_t currentAmount = data.end() - freeData;

			memset(&(*freeData), value, currentAmount);

			freeData = data.begin();
			writtenData += currentAmount;
			amount -= currentAmount;
		}

		memset(&(*freeData), value, amount);
		writtenData += amount;

		waitRes = 0;
		do {
			waitRes = WaitForSingleObject(dataMutex, timeout);
		} while (waitRes != WAIT_OBJECT_0);

		availableData += writtenData;

		ReleaseMutex(dataMutex);

		if (availableData >= waitReadAmount && waitReadAmount > 0) {
			ReleaseSemaphore(readSemaphore, 1, NULL);
		}

		return true;
	}
}