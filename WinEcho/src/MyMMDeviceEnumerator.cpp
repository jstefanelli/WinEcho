#include "MyMMDeviceEnumerator.h"
#include "CApiUtils.h"
#include <stdexcept>

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);

namespace winecho {

	MyMMDeviceEnumerator::MyMMDeviceEnumerator(EDataFlow flow) {
		enumerator = nullptr;
		IMMDeviceCollection* c = nullptr;
		HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&enumerator));

		if (FAILED(hr)) {
			throw std::runtime_error("Failed to get IMMDeviceEnumerator");
		}

		hr = enumerator->EnumAudioEndpoints(EDataFlow::eRender, DEVICE_STATE_ACTIVE, &c);
		if (FAILED(hr) || c == nullptr) {
			throw std::runtime_error("IMMDeviceEnumerator::EnumAudioEnpoints() failed");
		}

		UINT deviceAmount = 0;
		hr = c->GetCount(&deviceAmount);
		if (FAILED(hr)) {
			throw std::runtime_error("IMMDeviceCollection::GetCount() failed");
		}

		for (UINT i = 0; i < deviceAmount; i++) {
			IMMDevice* dev;
			hr = c->Item(i, &dev);

			if (SUCCEEDED(hr) && dev != nullptr) {
				devices.push_back(std::make_shared<winecho::MyMMDevice>(dev));
			}
		}

		c->Release();
	}

	MyMMDeviceEnumerator::~MyMMDeviceEnumerator() {
		if (enumerator != nullptr) {
			enumerator->Release();
			enumerator = nullptr;
		}
	}

	size_t MyMMDeviceEnumerator::GetDeviceCount() const {
		return devices.size();
	}

	std::shared_ptr<MyMMDevice> MyMMDeviceEnumerator::GetDeviceByIndex(size_t index) const {
		if(index >= devices.size())
			return nullptr;
		return devices[index];
	}

	std::shared_ptr<MyMMDevice> MyMMDeviceEnumerator::GetDeviceByName(std::wstring name) const {
		for each (auto dev in devices)
		{
			std::wstring fullName = dev->Description() + L" (" + dev->Name() + L")";

			if (fullName == name) {
				return std::move(dev);
			}
		}
		return nullptr;
	}
}

extern "C" {
	void* genDeviceEnumerator(int dataFlow) {
		if (dataFlow != EDataFlow::eRender && dataFlow != EDataFlow::eCapture && dataFlow != EDataFlow::eAll) {
			return nullptr;
		}

		return genHandle<winecho::MyMMDeviceEnumerator>(new winecho::MyMMDeviceEnumerator((EDataFlow)dataFlow));
	}

	void releaseDeviceEnumerator(void* deviceEnumerator) {
		if (deviceEnumerator == nullptr) {
			return;
		}

		auto* handle = getHandle<winecho::MyMMDeviceEnumerator>(deviceEnumerator);
		delete handle;
	}


	size_t deviceEnumeratorGetDeviceCount(void* deviceEnumerator) {
		if (deviceEnumerator == nullptr) {
			return 0;
		}

		auto* handle = getHandle<winecho::MyMMDeviceEnumerator>(deviceEnumerator);
		return (*handle)->GetDeviceCount();
	}


	void* deviceEnumeratorGetDeviceByIndex(void* deviceEnumerator, size_t index) {
		if (deviceEnumerator == nullptr) {
			return nullptr;
		}

		auto* handle = getHandle<winecho::MyMMDeviceEnumerator>(deviceEnumerator);

		auto device = (*handle)->GetDeviceByIndex(index);
		if (device == nullptr) {
			return nullptr;
		}

		return genHandleCopy<winecho::MyMMDevice>(device);
	}


	void* deviceEnumeratorGetDeviceByName(void* deviceEnumerator, LPWSTR name) {
		if (deviceEnumerator == nullptr || name == nullptr) {
			return nullptr;
		}

		auto* handle = getHandle<winecho::MyMMDeviceEnumerator>(deviceEnumerator);
		std::wstring str(name);

		auto device = (*handle)->GetDeviceByName(str);
		if (device == nullptr) {
			return nullptr;
		}

		return genHandleCopy<winecho::MyMMDevice>(device);
	}
}