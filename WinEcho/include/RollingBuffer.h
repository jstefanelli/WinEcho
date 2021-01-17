#pragma once

#include <vector>
#include <memory>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "WinEchoApi.h"

namespace winecho {
	class WINECHO_API RollingBuffer {
		protected:
			std::vector<unsigned char> data;

			size_t bufferSize;
			std::vector<unsigned char>::iterator pointer;
			size_t availableData;
			
			size_t waitReadAmount;
			size_t waitWriteAmount;
			HANDLE readSemaphore;
			HANDLE writeSemaphore;
			HANDLE dataMutex;
		public:
			RollingBuffer();
			RollingBuffer(size_t bufferSize); 
			~RollingBuffer();

			RollingBuffer(const RollingBuffer&) = delete;
			RollingBuffer& operator=(const RollingBuffer&) = delete;

			inline size_t BufferSize() const { return bufferSize; }
			inline size_t AvailableData() const { return availableData; }

			bool Read(unsigned char* buffer, size_t amount, DWORD timeoutMs);
			bool Write(unsigned char* buffer, size_t amount, DWORD timeoutMs);
			bool WriteValue(unsigned char val, size_t amount, DWORD timeoutMs);
	};
}
