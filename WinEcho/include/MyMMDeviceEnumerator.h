#pragma once
#include <vector>
#include "MyMMDevice.h"
#include "WinEchoApi.h"

#ifdef __cplusplus
namespace winecho {
	 class WINECHO_API MyMMDeviceEnumerator {
		protected:
			IMMDeviceEnumerator* enumerator;
			std::vector<std::shared_ptr<MyMMDevice>> devices;
		public:
			MyMMDeviceEnumerator(EDataFlow flow);
			~MyMMDeviceEnumerator();

			MyMMDeviceEnumerator(const MyMMDeviceEnumerator&) = delete;
			MyMMDeviceEnumerator& operator=(const MyMMDeviceEnumerator&) = delete;

			size_t GetDeviceCount() const;
			std::shared_ptr<MyMMDevice> GetDeviceByIndex(size_t idx) const;
			std::shared_ptr<MyMMDevice> GetDeviceByName(std::wstring name) const;
	};
}

extern "C" {
#endif
	extern WINECHO_API void* genDeviceEnumerator(int dataFlow);
	extern WINECHO_API void releaseDeviceEnumerator(void* deviceEnumerator);

	extern WINECHO_API size_t deviceEnumeratorGetDeviceCount(void* deviceEnumerator);
	extern WINECHO_API void* deviceEnumeratorGetDeviceByIndex(void* deviceEnumerator, size_t index);
	extern WINECHO_API void* deviceEnumeratorGetDeviceByName(void* deviceEnumerator, LPWSTR name);
#ifdef __cplusplus
}
#endif