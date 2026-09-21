// A native bootstrap: unpack the embedded runtime into a private temporary
// directory, run the Qt application, and remove the runtime after it exits.
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <objbase.h>
#include <filesystem>
#include <string>
#include <stdexcept>
#include "payload.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR arguments, int show) {
    std::filesystem::path directory;
    int result = 1;
    try {
        wchar_t temp[MAX_PATH + 1];
        const DWORD tempLength = GetTempPathW(MAX_PATH + 1, temp);
        if (!tempLength || tempLength > MAX_PATH)
            throw std::runtime_error("Cannot locate the temporary folder.");
        GUID guid;
        wchar_t identifier[40];
        if (FAILED(CoCreateGuid(&guid)) || !StringFromGUID2(guid, identifier, 40))
            throw std::runtime_error("Cannot create a runtime identifier.");
        const auto candidate = std::filesystem::path(temp)
                / (std::wstring(L"TIATracker-") + identifier);
        if (!CreateDirectoryW(candidate.c_str(), nullptr))
            throw std::runtime_error("Cannot create the temporary runtime folder.");
        directory = candidate; // Only clean up a directory we created ourselves.
        for (const auto &entry : payload) {
            const auto filename = directory / entry.path;
            std::filesystem::create_directories(filename.parent_path());
            HRSRC resource = FindResourceW(instance, MAKEINTRESOURCEW(entry.id), RT_RCDATA);
            HGLOBAL loaded = resource ? LoadResource(instance, resource) : nullptr;
            const void *bytes = loaded ? LockResource(loaded) : nullptr;
            const DWORD size = resource ? SizeofResource(instance, resource) : 0;
            if (!bytes || !size)
                throw std::runtime_error("An embedded runtime file is missing.");
            HANDLE file = CreateFileW(filename.c_str(), GENERIC_WRITE, 0, nullptr,
                                      CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file == INVALID_HANDLE_VALUE)
                throw std::runtime_error("Cannot extract the application runtime.");
            DWORD written = 0;
            const bool success = WriteFile(file, bytes, size, &written, nullptr) && written == size;
            CloseHandle(file);
            if (!success)
                throw std::runtime_error("Cannot write the application runtime (disk full?).");
        }
        const auto executable = directory / L"TIATracker.exe";
        std::wstring command = L"\"" + executable.wstring() + L"\" " + arguments;
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        startup.dwFlags = STARTF_USESHOWWINDOW;
        startup.wShowWindow = static_cast<WORD>(show);
        PROCESS_INFORMATION process{};
        if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE,
                            0, nullptr, nullptr, &startup, &process))
            throw std::runtime_error("Cannot start TIATracker.");
        CloseHandle(process.hThread);
        WaitForSingleObject(process.hProcess, INFINITE);
        DWORD exitCode = 1;
        GetExitCodeProcess(process.hProcess, &exitCode);
        CloseHandle(process.hProcess);
        result = static_cast<int>(exitCode);
    } catch (const std::exception &error) {
        MessageBoxA(nullptr, error.what(), "TIATracker startup failed", MB_OK | MB_ICONERROR);
    }
    if (!directory.empty()) {
        std::error_code ignored;
        std::filesystem::remove_all(directory, ignored);
    }
    return result;
}
