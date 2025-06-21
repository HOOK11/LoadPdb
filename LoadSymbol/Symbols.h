#include "Verify.h"
#include "Parse.h"

#ifndef _SYMBOLS_H
#define _SYMBOLS_H

struct PDBHeader7
{
	char signature[0x20];
	int page_size;
	int allocation_table_pointer;
	int file_page_count;
	int root_stream_size;
	int reserved;
	int root_stream_page_number_list_number;
};

struct RootStream7
{
	int num_streams;
	int stream_sizes[1]; //num_streams
};

struct GUID_StreamData
{
	int ver;
	int date;
	int age;
	GUID guid;
};

struct PdbInfo
{
	DWORD	Signature;
	GUID	Guid;
	DWORD	Age;
	char	PdbFileName[1];
};



bool DownloadSymbolPDB(const std::wstring& szModulePath, const std::wstring& path, std::wstring* pdb_path_out, int selectSvr);
// 回调函数
HRESULT OnProgress(
	unsigned long ulProgress,
	unsigned long ulProgressMax,
	unsigned long ulStatusCode,
	LPCWSTR       szStatusText
);


// 符号服务器枚举
enum SymbolServer {

	CUSTOM_SERVER = 1,

	MS_SYMBOL_SERVER = 2,    // 微软官方符号服务器

	SZDYG_SERVER = 3
};



#endif // !_SYMBOLS_H
