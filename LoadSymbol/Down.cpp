#include "Down.h"
#include "Symbols.h"
#include <iostream>

#pragma warning(disable:4996)


// �ο�ʹ���˼���pdb ������о��������� ����������� 
// �ο���git clone  git@github.com:Kwansy98/EasyPdb.git
// SymbolicAccessKm git clone git@github.com:Air14/SymbolicAccess.git
// SymbolicAccessKm �� DownloadSymbols ���ص�c�� , ֱ��SymbolicAccessKm ����pdb
// �����ʹ�� DownloadSymbols ���� ��SymbolicAccessKmĬ��ʹ������΢�����ӷ���
//


//LoadSymBol �޸� ������֤pdb 

// ���� ���ص�ģ�� ������ѡ�� 
bool DownloadSymbols(const std::vector<std::wstring>& modules, int SvrCom)
{
	std::wstring Out;
	const std::wstring systemDir = L"C:\\Windows\\System32\\";
	const std::wstring symbolDir = L"C:\\Symbols\\";
	bool loaderror = false;
	for (const auto& mod : modules) {
		// ���µ�ǰ���ص�ģ����������UI��ʾ��
	//	modText = mod;

		// ��������ģ��·��
		std::wstring fullModulePath = systemDir + mod;

		// ����PDB
		if (!DownloadSymbolPDB(fullModulePath, symbolDir, &Out, SvrCom))
		{
			// ����ʧ����� - ����ѡ��������˳�
			std::wstring errorMsg = L"����ʧ��: " + mod;
			
			OutputDebugStringA("��������ʧ��");
		
			return loaderror; // ���� continue; ������һ��
		}
		loaderror = true;
	}
	OutputDebugStringA("����������ɣ�\r\n");

	return loaderror;
}

int main() {
	const std::wstring modulePath = L"C:\\Windows\\System32\\ntoskrnl.exe";
	const std::wstring symbolDir = L"C:\\Symbols\\";
	std::wstring pdbPath;
	int serverChoice = 3; // ѡ����ŷ�����

	try {
		// ����PDB�ļ�
		if (DownloadSymbolPDB(modulePath, symbolDir, &pdbPath, serverChoice)) {
			// ��ӡ·��
			std::wcout << L"PDB path: " << pdbPath << std::endl;

			// ����������
			PdbParser parser(pdbPath);

			// ��ȡ������ַ
			uintptr_t rva = parser.getRvaByName("DbgkDebugObjectType");
			std::cout << "address: 0x" << std::hex << rva << std::endl;


			DWORD offset = parser.getStructMemberOffset("_EPROCESS", "DebugPort");
			std::cout << "_EPROCESS.DebugPort ƫ��: 0x" << std::hex << offset << std::endl;
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
//		//printf("����������� \r\n");
//	}
//	else {
//		printf("��������ʧ�� \r\n");
//	}
//
//	system("pause");
//	return 0;
//}

