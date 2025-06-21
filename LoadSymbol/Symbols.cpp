#include "Down.h"
#include "Symbols.h"

#include <urlmon.h>
#include <functional>
#pragma comment(lib, "urlmon.lib")


CVerify VerifPDB;

CConsole callback;

bool DownloadSymbolPDB(const std::wstring& szModulePath, const std::wstring& path, std::wstring* pdb_path_out, int selectSvr)
{

	auto FileDeleter = [](std::ifstream* f) { if (f && f->is_open()) f->close(); };
	
	std::unique_ptr<std::ifstream, decltype(FileDeleter)> File(new std::ifstream(szModulePath, std::ios::binary | std::ios::ate), FileDeleter);

	if (!File->good())
	{

		return false;
	}

	auto FileSize = File->tellg();
	if (!FileSize)
	{
		
		return false;
	}

	auto pRawData = std::make_unique<BYTE[]>(static_cast<size_t>(FileSize));
	File->seekg(0, std::ios::beg);
	File->read(reinterpret_cast<char*>(pRawData.get()), FileSize);
	File.reset();  // 显式关闭文件

	IMAGE_DOS_HEADER* pDos = reinterpret_cast<IMAGE_DOS_HEADER*>(pRawData.get());
	if (pDos->e_magic != IMAGE_DOS_SIGNATURE)
	{

		return false;
	}

	IMAGE_NT_HEADERS* pNT = reinterpret_cast<IMAGE_NT_HEADERS*>(pRawData.get() + pDos->e_lfanew);
	if (pNT->Signature != IMAGE_NT_SIGNATURE)
	{

		return false;
	}

	IMAGE_FILE_HEADER* pFile = &pNT->FileHeader;
	IMAGE_OPTIONAL_HEADER64* pOpt64 = nullptr;
	IMAGE_OPTIONAL_HEADER32* pOpt32 = nullptr;
	bool x86 = false;

	if (pFile->Machine == IMAGE_FILE_MACHINE_AMD64)
	{
		pOpt64 = reinterpret_cast<IMAGE_OPTIONAL_HEADER64*>(&pNT->OptionalHeader);
	}
	else if (pFile->Machine == IMAGE_FILE_MACHINE_I386)
	{
		pOpt32 = reinterpret_cast<IMAGE_OPTIONAL_HEADER32*>(&pNT->OptionalHeader);
		x86 = true;
	}
	else
	{
		
		return false;
	}

	DWORD ImageSize = x86 ? pOpt32->SizeOfImage : pOpt64->SizeOfImage;



	auto pLocalImageBase = std::unique_ptr<BYTE, std::function<void(BYTE*)>>(
		reinterpret_cast<BYTE*>(VirtualAlloc(nullptr, ImageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)),
		[](BYTE* ptr) {
			if (ptr) VirtualFree(ptr, 0, MEM_RELEASE);
		}
	);

	if (!pLocalImageBase)
	{
		return false;
	}

	memcpy(pLocalImageBase.get(), pRawData.get(), x86 ? pOpt32->SizeOfHeaders : pOpt64->SizeOfHeaders);

	auto* pCurrentSectionHeader = IMAGE_FIRST_SECTION(pNT);
	for (UINT i = 0; i != pFile->NumberOfSections; ++i, ++pCurrentSectionHeader)
	{
		if (pCurrentSectionHeader->SizeOfRawData)
		{
			memcpy(pLocalImageBase.get() + pCurrentSectionHeader->VirtualAddress,
				pRawData.get() + pCurrentSectionHeader->PointerToRawData,
				pCurrentSectionHeader->SizeOfRawData);
		}
	}

	IMAGE_DATA_DIRECTORY* pDataDir = &(x86 ?
		pOpt32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG] :
		pOpt64->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG]);

	if (!pDataDir->Size || pDataDir->VirtualAddress == 0)
	{
		
		return false;
	}

	IMAGE_DEBUG_DIRECTORY* pDebugDir = reinterpret_cast<IMAGE_DEBUG_DIRECTORY*>(
		pLocalImageBase.get() + pDataDir->VirtualAddress);

	if (IMAGE_DEBUG_TYPE_CODEVIEW != pDebugDir->Type)
	{
		
		return false;
	}

	PdbInfo* pdb_info = reinterpret_cast<PdbInfo*>(pLocalImageBase.get() + pDebugDir->AddressOfRawData);
	if (pdb_info->Signature != 0x53445352)
	{
		
		return false;
	}

	VerifPDB.g_szPdbPath = path;
	if (VerifPDB.g_szPdbPath.back() != L'\\') VerifPDB.g_szPdbPath += L'\\';



	// 创建基础目录
	if (!VerifPDB.CreateIfNotExists(VerifPDB.g_szPdbPath.c_str()))
	{
		
		return false;
	}

	// 转换PDB文件名
	size_t len = strlen(pdb_info->PdbFileName);
	auto PdbFileNameW = std::make_unique<wchar_t[]>(len + 1);
	size_t SizeOut = 0;
	if (mbstowcs_s(&SizeOut, PdbFileNameW.get(), len + 1, pdb_info->PdbFileName, len) != 0 || !SizeOut)
	{
		
		return false;
	}

	VerifPDB.g_szPdbPath += PdbFileNameW.get();
	if (!VerifPDB.CreateIfNotExists(VerifPDB.g_szPdbPath.c_str()))
	{
		
		return false;
	}

	wchar_t w_GUID[100]{};
	if (StringFromGUID2(pdb_info->Guid, w_GUID, 100) == 0)
	{
		
		return false;
	}

	// 过滤GUID
	std::wstring guid_filtered;
	for (int i = 0; w_GUID[i]; ++i)
	{
		if (iswalnum(w_GUID[i])) guid_filtered += w_GUID[i];
	}

	// 添加GUID和age到路径
	VerifPDB.g_szPdbPath += L'\\' + guid_filtered + std::to_wstring(pdb_info->Age) + L'\\';
	
	if (!VerifPDB.CreateIfNotExists(VerifPDB.g_szPdbPath.c_str()))
	{
		
		return false;
	}

	VerifPDB.g_szPdbPath += PdbFileNameW.get();

	GUID guid = pdb_info->Guid;
	DWORD age = pdb_info->Age;
	std::wstring pdbPath = VerifPDB.g_szPdbPath; // 比如：L"C:\\symbols\\mydll.pdb"
	//bool 

	// 先检查本地是否已经有匹配的 PDB 文件
	if (!VerifPDB.ShouldRedownloadPdb(pdb_info->Guid, pdb_info->Age, pdbPath)) {
		// 本地文件已存在且验证通过，跳过下载

		if (pdb_path_out)
		{
			*pdb_path_out = VerifPDB.g_szPdbPath;
		}

		OutputDebugStringA("符号信息找到 ,朓过下载");
		
		return true;
	}


	if (!VerifPDB.SavePdbVerificationInfo(guid, age, pdbPath)) {
		// 写入失败，记录日志
		
		OutputDebugStringA("写入失败");
	}

	// 构建下载URL

	std::string url;
	
	if (selectSvr == 1) url = "http://msdl.blackint3.com:88/download/symbols/";

	else if (selectSvr == 2) url = "https://msdl.microsoft.com/download/symbols/"; //微软服务器

	else if (selectSvr == 3)url = "https://msdl.szdyg.cn/download/symbols/";
	
	else {

		OutputDebugStringA("无效的服务器配置");
		return false;
	}

	url += pdb_info->PdbFileName;

	// 修复类型不匹配问题
	std::string narrow_guid(guid_filtered.begin(), guid_filtered.end());
	url += '/';
	url += narrow_guid;
	url += std::to_string(pdb_info->Age);
	url += '/';
	url += pdb_info->PdbFileName;


	// 转换URL
	auto UrlW = std::make_unique<wchar_t[]>(url.size() + 1);
	if (mbstowcs_s(&SizeOut, UrlW.get(), url.size() + 1, url.c_str(), url.size()) != 0)
	{
		
		return false;
	}


	const int maxRetries = 3;
	for (int attempt = 0; attempt < maxRetries; ++attempt) {
		HRESULT hr = URLDownloadToFileW(nullptr, UrlW.get(), VerifPDB.g_szPdbPath.c_str(), 0, &callback);

		if (SUCCEEDED(hr)) break;


		if (attempt == maxRetries - 1) {
	
			return false;
		}

		Sleep(1000 * (attempt + 1)); // 指数退避
	}


	// 验证
	if (!VerifPDB.VerifyExistingPdb(pdb_info->Guid))
	{
		DeleteFile(VerifPDB.g_szPdbPath.c_str());
		
		OutputDebugStringA("%s[%d] 下载的PDB文件验证失败! ");
		return false;
	}
	else {
		OutputDebugStringA("%s[%d] 下载的PDB文件验证成功! ");
		//验证成功之后写入信息
		if (!VerifPDB.SavePdbVerificationInfo(pdb_info->Guid, pdb_info->Age, VerifPDB.g_szPdbPath)) {

			OutputDebugStringA("%s[%d] 写入验证信息失败");
		}
	}


	if (pdb_path_out) *pdb_path_out = VerifPDB.g_szPdbPath;


	return true;
}