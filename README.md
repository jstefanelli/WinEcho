# WinEcho
A 0-dependency C++/C windows library for handling WASAPI loopback audio devices

## Overview

WinEcho is a C++ windows library (With an extra C interface) that allows playback of WASAPI loopback render devices on other render devices.

## Usage

First, get a list of devices using the `winecho::MyMMDeviceEnumerator` class. This class accepts a `EDataFlow` parameter, but the library is set up to handle only `EDataFlow::eRender` devices.

When you find you **capture** (device that you want to record the audio) and **render** (device that you want to playback the audio on) devices, you can generate a `winecho::StreamInstance` class.

The parameters are:

- Capture device
- Render devce
- Buffer size (with high latency, a large buffer may be required)
- Desired latency (in milliseconds. This is only a hint and is not guaranteed)

The `winecho::StreamInstance::Start()` method spawns the threads that handle capture and render.

The `winecho::StreamInstance::Stop(bool wait)` methods stops the threads (and with the `wait` parameter you choose wether to wait for the threads to finish.

## TODO

The library is missing proper error handling in the C api and in the worker threads.

The C interface is only implements `winecho::MyMMDeviceEnumerator`, `winecho::MyMMDevice` and `winecho::StreamInstance`.
