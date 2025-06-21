#include "Verify.h"
#include "Symbols.h"



bool CVerify::CreateIfNotExists(const TCHAR* dir)
{
	if (CreateDirectory(dir, NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
	{
		//OutputDebugStringA("符号目录创建完成");
		return true;
	}
	else {
		OutputDebugStringA("符号目录创建失败");
		return false;
	}
}

//PDB 验证信息
bool CVerify::SavePdbVerificationInfo(const GUID& guid, DWORD age, const std::wstring& pdbPath) {
	std::wstring verifyFilePath = pdbPath + L".verify";

	HANDLE hFile = CreateFileW(verifyFilePath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) return false;

	PdbVerifyInfo info = { guid, age };
	DWORD written = 0;
	BOOL ok = WriteFile(hFile, &info, sizeof(info), &written, nullptr);
	CloseHandle(hFile);
	return ok && written == sizeof(info);
}

bool CVerify::ShouldRedownloadPdb(const GUID& guid, DWORD age, const std::wstring& pdbPath) {
	std::wstring verifyFilePath = pdbPath + L".verify";

	HANDLE hFile = CreateFileW(verifyFilePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) return true;

	PdbVerifyInfo info = {};
	DWORD read = 0;
	BOOL ok = ReadFile(hFile, &info, sizeof(info), &read, nullptr);
	CloseHandle(hFile);

	if (!ok || read != sizeof(info)) return true;

	return !(IsEqualGUID(info.guid, guid) && info.age == age); 
}
bool CVerify::VerifyExistingPdb(const GUID& guid)
{

	std::ifstream f(g_szPdbPath.c_str(), std::ios::binary | std::ios::ate);
	if (f.bad())
	{
		return false;
	}

	size_t size_on_disk = static_cast<size_t>(f.tellg());
	if (!size_on_disk)
	{
		f.close();

		return false;
	}

	char* pdb_raw = new char[size_on_disk];
	if (!pdb_raw)
	{
		f.close();

		return false;
	}

	f.seekg(std::ios::beg);
	f.read(pdb_raw, size_on_disk);
	f.close();

	if (size_on_disk < sizeof(PDBHeader7))
	{
		delete[] pdb_raw;

		return false;
	}

	auto* pPDBHeader = reinterpret_cast<PDBHeader7*>(pdb_raw);

	if (memcmp(pPDBHeader->signature, "Microsoft C/C++ MSF 7.00\r\n\x1A""DS\0\0\0", sizeof(PDBHeader7::signature)))
	{
		delete[] pdb_raw;

		return false;
	}

	if (size_on_disk < (size_t)pPDBHeader->page_size * pPDBHeader->file_page_count)
	{
		delete[] pdb_raw;

		return false;
	}

	int* pRootPageNumber = reinterpret_cast<int*>(pdb_raw + (size_t)pPDBHeader->root_stream_page_number_list_number * pPDBHeader->page_size);
	auto* pRootStream = reinterpret_cast<RootStream7*>(pdb_raw + (size_t)(*pRootPageNumber) * pPDBHeader->page_size);

	std::map<int, std::vector<int>> streams;
	int current_page_number = 0;

	for (int i = 0; i != pRootStream->num_streams; ++i)
	{
		int current_size = pRootStream->stream_sizes[i] == 0xFFFFFFFF ? 0 : pRootStream->stream_sizes[i];

		int current_page_count = current_size / pPDBHeader->page_size;
		if (current_size % pPDBHeader->page_size)
		{
			++current_page_count;
		}

		std::vector<int> numbers;

		for (int j = 0; j != current_page_count; ++j, ++current_page_number)
		{
			numbers.push_back(pRootStream->stream_sizes[pRootStream->num_streams + current_page_number]);
		}

		streams.insert({ i, numbers });
	}

	auto pdb_info_page_index = streams.at(1).at(0);

	auto* stram_data = reinterpret_cast<GUID_StreamData*>(pdb_raw + (size_t)(pdb_info_page_index)*pPDBHeader->page_size);

	int guid_eq = memcmp(&stram_data->guid, &guid, sizeof(GUID));



	delete[] pdb_raw;

	return (guid_eq == 0);
}
 