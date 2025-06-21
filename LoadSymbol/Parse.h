#include <windows.h>
#include <DbgHelp.h>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <iomanip>
#include <stdexcept>

#pragma comment(lib, "dbghelp.lib")

class PdbParser {
public:
	PdbParser(const std::wstring& pdbPath) : m_handle(INVALID_HANDLE_VALUE), m_modBase(0) {
		Initialize(pdbPath);
	}

	~PdbParser() {
		if (m_handle != INVALID_HANDLE_VALUE) {
			SymCleanup(m_handle);
			CloseHandle(m_handle);
		}
	}

	// 禁用复制和移动
	PdbParser(const PdbParser&) = delete;
	PdbParser& operator=(const PdbParser&) = delete;

	// 根据符号名称获取RVA
	DWORD getRvaByName(const std::string& name) {
		SYMBOL_INFO_PACKAGE sip = { sizeof(sip) };
		sip.si.SizeOfStruct = sizeof(SYMBOL_INFO);
		sip.si.MaxNameLen = MAX_SYM_NAME;

		if (SymFromName(m_handle, name.c_str(), &sip.si)) {
			return static_cast<DWORD>(sip.si.Address - m_modBase);
		}
		return 0;
	}

	// 根据RVA获取符号名称
	std::string getNameByRva(DWORD rva) {
		DWORD64 address = m_modBase + rva;
		char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)] = { 0 };
		SYMBOL_INFO* symbol = (SYMBOL_INFO*)buffer;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->MaxNameLen = MAX_SYM_NAME;

		if (SymFromAddr(m_handle, address, nullptr, symbol)) {
			return symbol->Name;
		}
		return "";
	}

	// 获取结构体成员偏移
	DWORD getStructMemberOffset(const std::string& structName, const std::string& memberName) {
		SYMBOL_INFO_PACKAGE sip = { sizeof(sip) };
		sip.si.SizeOfStruct = sizeof(SYMBOL_INFO);
		sip.si.MaxNameLen = MAX_SYM_NAME;

		if (!SymGetTypeFromName(m_handle, m_modBase, structName.c_str(), &sip.si)) {
			return 0xFFFFFFFF;
		}

		DWORD childrenCount = 0;
		if (!SymGetTypeInfo(m_handle, m_modBase, sip.si.TypeIndex, TI_GET_CHILDRENCOUNT, &childrenCount)) {
			return 0xFFFFFFFF;
		}

		std::vector<BYTE> buffer(sizeof(TI_FINDCHILDREN_PARAMS) + childrenCount * sizeof(ULONG));
		TI_FINDCHILDREN_PARAMS* childrenParams = reinterpret_cast<TI_FINDCHILDREN_PARAMS*>(buffer.data());
		childrenParams->Count = childrenCount;
		childrenParams->Start = 0;

		if (!SymGetTypeInfo(m_handle, m_modBase, sip.si.TypeIndex, TI_FINDCHILDREN, childrenParams)) {
			return 0xFFFFFFFF;
		}

		for (ULONG i = 0; i < childrenCount; i++) {
			ULONG childId = childrenParams->ChildId[i];
			WCHAR* symName = nullptr;
			DWORD offset = 0;

			if (SymGetTypeInfo(m_handle, m_modBase, childId, TI_GET_OFFSET, &offset) &&
				SymGetTypeInfo(m_handle, m_modBase, childId, TI_GET_SYMNAME, &symName)) {

				// 转换为多字节字符串进行比较
				char mbName[256] = { 0 };
				WideCharToMultiByte(CP_UTF8, 0, symName, -1, mbName, sizeof(mbName), nullptr, nullptr);
				LocalFree(symName);

				if (memberName == mbName) {
					return offset;
				}
			}
		}
		return 0xFFFFFFFF;
	}

private:
	void Initialize(const std::wstring& pdbPath) {
		// 获取当前进程句柄
		m_handle = GetCurrentProcess();

		// 初始化符号处理
		if (!SymInitialize(m_handle, nullptr, FALSE)) {
			throw std::runtime_error("SymInitialize failed: " + std::to_string(GetLastError()));
		}

		// 设置符号选项
		SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);

		// 获取PDB文件大小
		HANDLE hFile = CreateFileW(pdbPath.c_str(), GENERIC_READ, FILE_SHARE_READ,
			nullptr, OPEN_EXISTING, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE) {
			throw std::runtime_error("无法打开 PDB 文件: " + std::to_string(GetLastError()));
		}

		LARGE_INTEGER fileSize;
		if (!GetFileSizeEx(hFile, &fileSize)) {
			CloseHandle(hFile);
			throw std::runtime_error("无法获取 PDB 大小 ： " + std::to_string(GetLastError()));
		}
		CloseHandle(hFile);

		// 加载PDB模块
		m_modBase = SymLoadModuleExW(m_handle, nullptr, pdbPath.c_str(), nullptr,
			0x10000000, fileSize.QuadPart, nullptr, 0);

		if (!m_modBase) {
			throw std::runtime_error("SymLoadModuleEx 失败:  " + std::to_string(GetLastError()));
		}
	}

	HANDLE m_handle;
	DWORD64 m_modBase;
};

