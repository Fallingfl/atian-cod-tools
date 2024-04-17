#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#ifdef ACTS_COMMON_DLL
# define ACTS_COMMON_API extern "C" __declspec(dllexport)
#else
# define ACTS_COMMON_API extern "C" __declspec(dllimport)
#endif

/*
 * Init ACTS common
 * @param argc main argc
 * @param argv main argv
 * @param hInstance hInstance, if non null, ui otherwise CLI
 * @return ret code
 */
ACTS_COMMON_API int MainActs(int argc, const char* argv[], HINSTANCE hInstance = nullptr, int nShowCmd = 0);