#include "Down.h"
#include "Symbols.h"
#include <iostream>

#pragma warning(disable:4996)


// 参考使用了几个pdb 解析库感觉都不趁手 ，就有了这个 
// 参考：git clone  git@github.com:Kwansy98/EasyPdb.git
// SymbolicAccessKm git clone git@github.com:Air14/SymbolicAccess.git
// SymbolicAccessKm 和 DownloadSymbols 下载到c盘 , 直接SymbolicAccessKm 解析pdb
// 如果不使用 DownloadSymbols 下载 ，SymbolicAccessKm默认使用下载微软链接符号
//


//LoadSymBol 修复 包括验证pdb 

// 参数 下载的模块 服务器选项 
bool DownloadSymbols(const std::vector<std::wstring>& modules, int SvrCom)
{
	std::wstring Out;
	const std::wstring systemDir = L"C:\\Windows\\System32\\";
	const std::wstring symbolDir = L"C:\\Symbols\\";
	bool loaderror = false;
	for (const auto& mod : modules) {
		// 更新当前下载的模块名（用于UI显示）
	//	modText = mod;

		// 构建完整模块路径
		std::wstring fullModulePath = systemDir + mod;

		// 下载PDB
		if (!DownloadSymbolPDB(fullModulePath, symbolDir, &Out, SvrCom))
		{
			// 处理失败情况 - 可以选择继续或退出
			std::wstring errorMsg = L"下载失败: " + mod;
			
			OutputDebugStringA("符号下载失败");
		
			return loaderror; // 或者 continue; 继续下一个
		}
		loaderror = true;
	}
	OutputDebugStringA("符号下载完成！\r\n");

	return loaderror;
}

int main() {
	const std::wstring modulePath = L"C:\\Windows\\System32\\ntoskrnl.exe";
	const std::wstring symbolDir = L"C:\\Symbols\\";
	std::wstring pdbPath;
	int serverChoice = 3; // 选择符号服务器

	try {
		// 下载PDB文件
		if (DownloadSymbolPDB(modulePath, symbolDir, &pdbPath, serverChoice)) {
			// 打印路径
			std::wcout << L"PDB path: " << pdbPath << std::endl;

			// 创建解析器
			PdbParser parser(pdbPath);

			// 获取函数地址
			uintptr_t rva = parser.getRvaByName("DbgkDebugObjectType");
			std::cout << "address: 0x" << std::hex << rva << std::endl;


			DWORD offset = parser.getStructMemberOffset("_EPROCESS", "DebugPort");
			std::cout << "_EPROCESS.DebugPort 偏移: 0x" << std::hex << offset << std::endl;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	system("pause");
	return 0;
}


//int main()
//{
//	
//	//DownloadSymbol();
//	
//	std::vector<std::wstring> kernelModules = 
//	{
//		L"ntoskrnl.exe",
//		L"win32kbase.sys",
//		L"win32kfull.sys",
//	};
//
//	bool LoadSymbol =  DownloadSymbols(kernelModules, MS_SYMBOL_SERVER);
//
//	if (LoadSymbol == true)
//	{
//		
//
//		//printf("符号下载完成 \r\n");
//	}
//	else {
//		printf("符号下载失败 \r\n");
//	}
//
//	system("pause");
//	return 0;
//}

