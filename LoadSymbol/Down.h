#pragma once

#ifndef _WIN_MAIN_H
#define _WIN_MAIN_H

#include <Windows.h>
#include <tchar.h>
#include <DbgHelp.h>
#include <psapi.h>
#include <shlwapi.h>
#include <string>

#include <vector>
#include <unordered_set>
#include <algorithm>
#include <fstream>
#include <process.h>
#include <assert.h>
#include <map>
#include <sstream>
#include <mutex>
#include <strsafe.h>
#include <intrin.h>
#include <winternl.h>


//PDBœ¬‘ÿ

bool DownloadSymbols(const std::vector<std::wstring>& modules, int Svr);

#endif // !_WIN_MAIN_H