# Third-party code

| Component | Version | Location | Licence | Used for |
|---|---|---|---|---|
| [SDL3](https://libsdl.org) | 3.4.20 | fetched by CMake (`cmake/CadvSDL3.cmake`), statically linked | zlib | window, GL context, input, audio, main loop |
| [Unity](https://github.com/ThrowTheSwitch/Unity) | 2.6.1 | `third_party/unity/` | MIT (`unity/LICENSE.txt`) | unit-test framework (tests only) |
| [glad](https://github.com/Dav1dde/glad) | 2.0.8 generator | `third_party/glad/` | generated code: WTFPL OR CC0-1.0; Khronos headers: Apache-2.0 / MIT-style (see file headers) | OpenGL 3.3 core loader (desktop only) |

Regenerating glad: `pip install glad2==2.0.8 && glad --api gl:core=3.3 --extensions='' --out-path third_party/glad c`

Third-party code is excluded from the coverage gate and compiled without the
project's strict warning flags.
