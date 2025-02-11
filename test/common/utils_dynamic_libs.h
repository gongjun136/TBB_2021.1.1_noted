/*
    Copyright (c) 2005-2020 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "oneapi/tbb/version.h"
#include "doctest.h"

#if __TBB_DYNAMIC_LOAD_ENABLED

#if _WIN32 || _WIN64
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

namespace utils {

#if TBB_USE_DEBUG
#define SUFFIX1 "_debug"
#define SUFFIX2
#else
#define SUFFIX1
#define SUFFIX2 "_debug"
#endif /* TBB_USE_DEBUG */

#if _WIN32||_WIN64
#define PREFIX
#define EXT ".dll"
#else
#define PREFIX "lib"
#if __APPLE__
#define EXT ".dylib"
// Android SDK build system does not support .so file name versioning
#elif __FreeBSD__ || __NetBSD__ || __sun || _AIX || __ANDROID__
#define EXT ".so"
#elif __linux__  // Order of these elif's matters!
#define EXT __TBB_STRING(.so.2)
#else
#error Unknown OS
#endif
#endif

// Form the names of the TBB memory allocator binaries.
#define MALLOCLIB_NAME1 PREFIX "tbbmalloc" SUFFIX1 EXT
#define MALLOCLIB_NAME2 PREFIX "tbbmalloc" SUFFIX2 EXT

#if _WIN32 || _WIN64
using LIBRARY_HANDLE = HMODULE;
#else
using LIBRARY_HANDLE = void*;
#endif

#if _WIN32 || _WIN64
#define TEST_LIBRARY_NAME(base) PREFIX base SUFFIX1 ".dll"
#elif __APPLE__
#define TEST_LIBRARY_NAME(base) PREFIX base SUFFIX1 ".dylib"
#else
#define TEST_LIBRARY_NAME(base) PREFIX base SUFFIX1 ".so"
#endif

LIBRARY_HANDLE OpenLibrary(const char *name)
{
#if _WIN32 || _WIN64
#if __TBB_WIN8UI_SUPPORT
    TCHAR wlibrary[MAX_PATH];
    if (MultiByteToWideChar(CP_UTF8, 0, name, -1, wlibrary, MAX_PATH) == 0) return false;
    return ::LoadPackagedLibrary(wlibrary, 0);
#else
    return ::LoadLibrary(name);
#endif
#else
    return dlopen(name, RTLD_NOW|RTLD_GLOBAL);
#endif
}

void CloseLibrary(LIBRARY_HANDLE lib)
{
#if _WIN32 || _WIN64
    BOOL ret = FreeLibrary(lib);
    REQUIRE_MESSAGE(ret, "FreeLibrary must be successful");
#else
    int ret = dlclose(lib);
    REQUIRE_MESSAGE(ret == 0, "dlclose must be successful");
#endif
}

typedef void (*FunctionAddress)();

template <typename FunctionPointer>
void GetAddress(utils::LIBRARY_HANDLE lib, const char *name, FunctionPointer& func)
{
#if _WIN32 || _WIN64
    func = (FunctionPointer)(void*)GetProcAddress(lib, name);
#else
    func = (FunctionPointer)dlsym(lib, name);
#endif
    REQUIRE_MESSAGE(func, "Can't find required symbol in dynamic library");
}

FunctionAddress GetAddress(utils::LIBRARY_HANDLE lib, const char *name)
{
    FunctionAddress func;
    GetAddress(lib, name, func);
    return func;
}

}  // namespace utils

#endif // __TBB_DYNAMIC_LOAD_ENABLED
