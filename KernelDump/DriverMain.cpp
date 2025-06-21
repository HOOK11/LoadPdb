#include <ntifs.h>
#include <SymbolicAccess/ModuleExtender/ModuleExtenderFactory.h>


EXTERN_C VOID DriverUnload(PDRIVER_OBJECT pDriver)
{
	DbgPrintEx(77, 0, "驱动卸载完成！\r\n");
}


namespace Offset
{
	size_t DebugPort;
}


EXTERN_C NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pReg_Path)
{

	symbolic_access::ModuleExtenderFactory extenderFactory{};
	
	const auto& moduleExtender = extenderFactory.Create(L"ntoskrnl.exe");
	
	if (!moduleExtender.has_value())
	{
		DbgPrintEx(77, 0, "文件解析失败！\r\n");
		
		return STATUS_UNSUCCESSFUL;
	}
	//解析函数
	POBJECT_TYPE* DbgkDebugObjectType;
	
	DbgkDebugObjectType = moduleExtender->GetPointer<POBJECT_TYPE>("DbgkDebugObjectType");

	DbgPrintEx(77, 0, "DbgkDebugObjectType = %p \r\n", DbgkDebugObjectType);

	//解析结构偏移
	// +0x578 DebugPort        : Ptr64 Void
	Offset::DebugPort = moduleExtender->GetOffset("_EPROCESS", "DebugPort").value_or(0xFFFFFFFF);

	DbgPrintEx(77, 0, "Offset::DebugPort  = %p \r\n", Offset::DebugPort);

	pDriver->DriverUnload = DriverUnload;

	NTSTATUS status = STATUS_SUCCESS;


	DbgPrintEx(77, 0, "驱动加载完成！\r\n");

	return status;
}
