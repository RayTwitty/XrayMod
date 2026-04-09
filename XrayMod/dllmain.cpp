#define _CRT_SECURE_NO_WARNINGS 1

#include "stdio.h"
#include <windows.h>
#include "MinHook\MinHook.h"

#include "Utils.h"
#include "Hooks.h"

#define COMMANDS "restore_deleted_commands"
bool cheats = true;
bool fov = true;

#ifndef _WIN64
typedef HMODULE(WINAPI* _LoadLibraryA)(LPCSTR lpLibFileName);
typedef void (__thiscall* _CCC_Mask)(void* _this, const char* name, void* value, u32 mask);
typedef void (__thiscall* _CCC_Float)(void* _this, const char* name, float* value, float min, float max);
typedef void (__thiscall* _AddCommand)(void* _this, void* cmd);

void* LoadLibraryA_Address = nullptr;
_LoadLibraryA LoadLibraryA_Orig = nullptr;
#else
// Enhanced Edition
typedef void* (__fastcall* _CCC_Mask)(void* _this, const char* name, void* value, u32 mask);
typedef void (__fastcall* _AddCommand)(void* _this, void* cmd);

void* AddCommand_Address = nullptr;
_AddCommand AddCommand_Orig = nullptr;
#endif

#ifndef _WIN64
void RestoreCMD()
{
	MODULEINFO miXRGame = Patcher::GetModuleData("xrGame.dll");

	// 83 0d ? ? ? ? ? f6 05
	DWORD instr_psActorFlags = Patcher::FindPattern((DWORD)miXRGame.lpBaseOfDll, miXRGame.SizeOfImage,
		"\x83\x0d\x00\x00\x00\x00\x00\xf6\x05", "xx?????xx");

	if (instr_psActorFlags == NULL) {
		// 09 3d ? ? ? ? 33 db
		instr_psActorFlags = Patcher::FindPattern((DWORD)miXRGame.lpBaseOfDll, miXRGame.SizeOfImage,
			"\x09\x3d\x00\x00\x00\x00\x33\xdb", "xx????xx");
	}

	void* psActorFlags = *(void**)(instr_psActorFlags + 2);
	void* Console			= *(void**)GetProcAddress(NULL, "?Console@@3PAVCConsole@@A");
	_CCC_Mask CCC_Mask		= (_CCC_Mask)GetProcAddress(NULL, "??0CCC_Mask@@QAE@PBDPAU?$_flags@I@@I@Z");
	_CCC_Float CCC_Float	= (_CCC_Float)GetProcAddress(NULL, "??0CCC_Float@@QAE@PBDPAMMM@Z");
	_AddCommand AddCommand	= (_AddCommand)GetProcAddress(NULL, "?AddCommand@CConsole@@QAEXPAVIConsole_Command@@@Z");

	if (cheats) {
		void* cmd_g_god = _aligned_malloc(0x14, 0x4);
		void* cmd_g_unlimitedammo = _aligned_malloc(0x14, 0x4);

		CCC_Mask(cmd_g_god, "g_god", psActorFlags, 1);
		CCC_Mask(cmd_g_unlimitedammo, "g_unlimitedammo", psActorFlags, 8);

		AddCommand(Console, cmd_g_god);
		AddCommand(Console, cmd_g_unlimitedammo);
	}

	// В патче 1.0002 команды fov и hud_fov есть, поэтому проверяем их существование
	if (fov && Patcher::FindPattern((DWORD)miXRGame.lpBaseOfDll, miXRGame.SizeOfImage, "hud_fov", "xxxxxxxx") == NULL)
	{
		float* psHUD_FOV = (float*)GetProcAddress(NULL, "?psHUD_FOV@@3MA");
		float* g_fov;

		// C6 81 ? ? ? ? ? F3 0F 10 05
		DWORD instr_g_fov = Patcher::FindPattern((DWORD)miXRGame.lpBaseOfDll, miXRGame.SizeOfImage,
			"\xC6\x81\x00\x00\x00\x00\x00\xF3\x0F\x10\x05", "xx?????xxxx");

		if (instr_g_fov == NULL) {
			// F3 0F 10 05 ? ? ? ? C6 81
			g_fov = *(float**)(Patcher::FindPattern((DWORD)miXRGame.lpBaseOfDll, miXRGame.SizeOfImage,
				"\xF3\x0F\x10\x05\x00\x00\x00\x00\xC6\x81", "xxxx????xx") + 4);
		} else {
			g_fov = *(float**)(instr_g_fov + 11);
		}

		void* cmd_fov		= _aligned_malloc(0x18, 0x4);
		void* cmd_hud_fov	= _aligned_malloc(0x18, 0x4);

		// В версиях 1.0000 и 1.0001 g_fov защищён от записи. Снимаем защиту.
		DWORD OldFovProtect = NULL;
		VirtualProtect(g_fov, sizeof(float), PAGE_READWRITE, &OldFovProtect);

		CCC_Float(cmd_fov,		"fov",		g_fov,		5.0f, 180.0f);
		CCC_Float(cmd_hud_fov,	"hud_fov",	psHUD_FOV,	0.1f, 1.0f);

		AddCommand(Console, cmd_fov);
		AddCommand(Console, cmd_hud_fov);
	}
}

HMODULE WINAPI LoadLibraryA_Hook(LPCSTR lpLibFileName)
{
	HMODULE result = LoadLibraryA_Orig(lpLibFileName);

	if (strcmp(lpLibFileName, "xrGame.dll") == 0) {
		RestoreCMD();
		MH_RemoveHook(LoadLibraryA_Address);
	}

	return result;
}

#else

void RestoreCMD(void* _this)
{
	// Enhanced Edition

	// 83 0D ? ? ? ? ? 4C 8D 2D
	DWORD64 instr_psActorFlags = Patcher::FindPatternInEXE("\x83\x0D\x00\x00\x00\x00\x00\x4C\x8D\x2D", "xx?????xxx");
	void* psActorFlags = (void*)Utils::GetAddrFromRelativeInstr(instr_psActorFlags, 7, 2);
	_CCC_Mask CCC_Mask = (_CCC_Mask)GetProcAddress(NULL, "??0CCC_Mask@@QEAA@PEBDPEAU?$_flags@I@@I@Z");

	void* cmd_g_god = _aligned_malloc(0x44, 0x8);
	void* cmd_g_unlimitedammo = _aligned_malloc(0x44, 0x8);

	CCC_Mask(cmd_g_god, "g_god", psActorFlags, 1);
	CCC_Mask(cmd_g_unlimitedammo, "g_unlimitedammo", psActorFlags, 8);

	AddCommand_Orig(_this, cmd_g_god);
	AddCommand_Orig(_this, cmd_g_unlimitedammo);
}

void __fastcall AddCommand_Hook(void* _this, void* cmd)
{
	RestoreCMD(_this);
	AddCommand_Orig(_this, cmd);
	MH_RemoveHook(AddCommand_Address);
}
#endif

DWORD WINAPI InstallThread(void*)
{
#ifndef _WIN64
	HMODULE xrGame = NULL;
	HMODULE xrGameSpy = NULL;

snova_game:
	if (!(xrGame = GetModuleHandle("xrGame.dll")))
		goto snova_game;

snova_spy:
	if (!(xrGameSpy = GetModuleHandle("xrGameSpy.dll")))
		goto snova_spy;
#endif

	Utils::Utils();
	Hooks::Hooks();

	return NULL;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
	if(reason == DLL_PROCESS_ATTACH)
	{
		//AllocConsole();
		//freopen("CONOUT$", "w", stdout);

		Beep(1000, 200);
		MH_Initialize();

#ifndef _WIN64
		fov = Utils::GetBool(COMMANDS, "fov", true);
#else
		fov = false;
#endif

		cheats = Utils::GetBool(COMMANDS, "cheats", true);

		if (cheats || fov) {
#ifndef _WIN64
			// Команды нужно регистрировать сразу как только загружена длл, иначе значения команд будут сбрасываться при запуске, поэтому делаем это в хуке.
			HMODULE kernel32 = GetModuleHandle("Kernel32.dll");
			LoadLibraryA_Address = (void*)GetProcAddress(kernel32, "LoadLibraryA");
			Hooks::SetHook("LoadLibraryA", LoadLibraryA_Address, &LoadLibraryA_Hook, &LoadLibraryA_Orig);
#else
			// В Enhanced Edition ситуация похожая, только тут Console на момент загрузки нашей длл ещё не инициализирован, поэтому хукаемся в AddCommand.
			AddCommand_Address = GetProcAddress(NULL, "?AddCommand@CConsole@@QEAAXPEAVIConsole_Command@@@Z");
			Hooks::SetHook("AddCommand", AddCommand_Address, &AddCommand_Hook, &AddCommand_Orig);
#endif
		}

		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)InstallThread, NULL, NULL, NULL);
	}
	
	return TRUE;
}

typedef HRESULT(WINAPI* _DirectInput8Create)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter);

#pragma comment(linker, "/export:DirectInput8Create@20=DirectInput8Create")
extern "C" __declspec(dllexport) HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter)
{
	char path[MAX_PATH];
	GetSystemDirectory(path, MAX_PATH);
	strcat(path, "\\dinput8.dll");

	HMODULE dinput8 = LoadLibrary(path);
	_DirectInput8Create DirectInput8Create_O = (_DirectInput8Create)GetProcAddress(dinput8, "DirectInput8Create");

	return DirectInput8Create_O(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}
