#pragma once
#include <windows.h>
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
class CVerify
{
public:

	bool CreateIfNotExists(const TCHAR* dir);

	struct PdbVerifyInfo {
		GUID guid;
		DWORD age;
	};
	

	bool SavePdbVerificationInfo(const GUID& guid, DWORD age, const std::wstring& pdbPath);

	bool ShouldRedownloadPdb(const GUID& guid, DWORD age, const std::wstring& pdbPath);

	bool VerifyExistingPdb(const GUID& guid);

	std::wstring g_szPdbPath;
};


class CConsole :public IBindStatusCallback
{
public:

	HWND g_hwnd ;
public:

	virtual HRESULT STDMETHODCALLTYPE OnStartBinding(
		DWORD dwReserved,
		__RPC__in_opt IBinding* pib) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetPriority(
		__RPC__out LONG* pnPriority) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE OnLowResource(
		DWORD reserved) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE OnProgress(  //下载调用
		ULONG ulProgress,
		ULONG ulProgressMax,
		ULONG ulStatusCode,
		__RPC__in_opt LPCWSTR szStatusText) {


		if (ulStatusCode == BINDSTATUS_DOWNLOADINGDATA) {

			if (ulProgressMax != 0) {

				// 计算百分比

				int percent = static_cast<int>((static_cast<double>(ulProgress) / ulProgressMax * 100));

				// 避免频繁更新，例如每10%更新一次，或者每次变化时更新

				// 这里我们每次更新都打印，但可以加条件限制

				// 显示下载信息没有

				//printf("\r下载进度: %d%%", percent);

				//fflush(stdout); // 刷新输出缓冲区，确保立即显示

			}

		}

		return S_OK;

	}


	
	virtual HRESULT STDMETHODCALLTYPE OnStopBinding(
		HRESULT hresult,
		__RPC__in_opt LPCWSTR szError) {
		return E_NOTIMPL;
	}

	virtual  HRESULT STDMETHODCALLTYPE GetBindInfo(
		DWORD* grfBINDF,
		BINDINFO* pbindinfo) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE OnDataAvailable(
		DWORD grfBSCF,
		DWORD dwSize,
		FORMATETC* pformatetc,
		STGMEDIUM* pstgmed) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE OnObjectAvailable(
		__RPC__in REFIID riid,
		__RPC__in_opt IUnknown* punk) {
		return E_NOTIMPL;
	}

	STDMETHOD_(ULONG, AddRef)() { return 0; }
	STDMETHOD_(ULONG, Release)() { return 0; }
	STDMETHOD(QueryInterface)(REFIID riid, void __RPC_FAR* __RPC_FAR* ppvObject) { return S_OK; }

};



