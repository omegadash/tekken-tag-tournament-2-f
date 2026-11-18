#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "tekken_force_settings.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iterator>
#include <cwchar>

namespace tekken_force {
float LoadCameraPullbackPercent()
{
    std::filesystem::path path;
    if (const char* overridePath = std::getenv("TTT2_TF_CONFIG"); overridePath && *overridePath)
        path = std::filesystem::absolute(overridePath);
    else
    {
        wchar_t executable[32768]{};
        GetModuleFileNameW(nullptr, executable, DWORD(std::size(executable)));
        path = std::filesystem::path(executable).parent_path() / L"tekken-force.ini";
    }
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES)
        WritePrivateProfileStringW(L"Camera", L"PullbackPercent", L"50", path.c_str());
    wchar_t text[64]{};
    GetPrivateProfileStringW(L"Camera", L"PullbackPercent", L"50", text, DWORD(std::size(text)), path.c_str());
    wchar_t* end = nullptr;
    float percent = std::wcstof(text, &end);
    while (end && (*end == L' ' || *end == L'\t')) ++end;
    if (end == text || (end && *end) || !std::isfinite(percent)) percent = 50.0f;
    percent = std::clamp(percent, 0.0f, 200.0f);
    std::fprintf(stderr, "[tekken-force] camera pullback %.1f%% (%ls)\n", percent, path.c_str());
    return percent;
}
}
