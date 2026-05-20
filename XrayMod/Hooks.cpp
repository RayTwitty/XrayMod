#include "Hooks.h"
#include "Utils.h"

bool teleport = true;
bool wpn_bobbing = true;

CWeaponBobbing* g_pWpnBobbing = nullptr;
void* CWeaponHUD_UpdatePosition_Orig = nullptr;

void** g_pGameLevel = nullptr;
void* CDemoRecord_IR_OnKeyboardPress_Address;
_IR_OnKeyboardPress CDemoRecord_IR_OnKeyboardPress_Orig = nullptr;

Hooks::Hooks()
{
	teleport = Utils::GetBool("other", "teleport", true);

	if (wpn_bobbing) {
		g_pWpnBobbing = new CWeaponBobbing;
	}

#ifndef _WIN64
	bool old = false;

	MODULEINFO miXRGame = GetModuleData("xrGame.dll");

	// F3 0F 10 41 ? F3 0F 10 49 ? 8B 42 48 - 1.0004 - 1.0006
	void* CWeaponHUD_UpdatePosition_Address = (void*)FindPattern(
		(DWORD)miXRGame.lpBaseOfDll,
		miXRGame.SizeOfImage,
		"\xF3\x0F\x10\x41\x00\xF3\x0F\x10\x49\x00\x8B\x42\x48",
		"xxxx?xxxx?xxx");

	if (CWeaponHUD_UpdatePosition_Address == NULL) {
		old = true;

		// 8B 54 24 04 8B 41 48 - 1.0000 - 1.0003
		CWeaponHUD_UpdatePosition_Address = (void*)FindPattern(
			(DWORD)miXRGame.lpBaseOfDll,
			miXRGame.SizeOfImage,
			"\x8B\x54\x24\x04\x8B\x41\x48",
			"xxxxxxx");
	}

	if (wpn_bobbing) {
		SetHook("CWeaponHUD::UpdatePosition", (void*)CWeaponHUD_UpdatePosition_Address, (old ? (void*)&CWeaponHUD_UpdatePosition_HookOld : (void*)&CWeaponHUD_UpdatePosition_HookNew),
			(void**)&CWeaponHUD_UpdatePosition_Orig);
	}

	g_pGameLevel = (void**)GetProcAddress(NULL, "?g_pGameLevel@@3PAVIGame_Level@@A");

	if (!old) {
		// 55 8B EC 83 E4 F8 8B 45 08 83 EC 44 - 1.0004 - 1.0006
		CDemoRecord_IR_OnKeyboardPress_Address = (void*)FindPatternInEXE("\x55\x8B\xEC\x83\xE4\xF8\x8B\x45\x08\x83\xEC\x44", "xxxxxxxxxxxx");
	} else {
		// 8B 44 24 04 83 EC 40 83 F8 39 - 1.0000 - 1.0003
		CDemoRecord_IR_OnKeyboardPress_Address = (void*)FindPatternInEXE("\x8B\x44\x24\x04\x83\xEC\x40\x83\xF8\x39", "xxxxxxxxxx");
	}
#else
	// Enhanced Edition

	if (wpn_bobbing) {
		// 48 8B C4 48 89 58 08 57 48 81 EC ? ? ? ? 4C 8B 05
		DWORD64 CWeaponHUD_UpdatePosition_Address = FindPatternInEXE("\x48\x8B\xC4\x48\x89\x58\x08\x57\x48\x81\xEC\x00\x00\x00\x00\x4C\x8B\x05", "xxxxxxxxxxx????xxx");
		SetHook("CWeaponHUD::UpdatePosition", (void*)CWeaponHUD_UpdatePosition_Address, &CWeaponHUD_UpdatePosition_HookEE, (void**)&CWeaponHUD_UpdatePosition_Orig);
	}

	g_pGameLevel = (void**)GetProcAddress(NULL, "?g_pGameLevel@@3PEAVIGame_Level@@EA");

	// 48 89 5C 24 ? 48 89 74 24 ? 55 57 41 56 48 8D 6C 24 ? 48 81 EC ? ? ? ? 8B DA
	CDemoRecord_IR_OnKeyboardPress_Address = (void*)FindPatternInEXE(
		"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x55\x57\x41\x56\x48\x8D\x6C\x24\x00\x48\x81\xEC\x00\x00\x00\x00\x8B\xDA", "xxxx?xxxx?xxxxxxxx?xxx????xx");
#endif

	if (teleport) {
		SetHook("CDemoRecord::IR_OnKeyboardPress", (void*)CDemoRecord_IR_OnKeyboardPress_Address, &CDemoRecord_IR_OnKeyboardPress_Hook, (void**)&CDemoRecord_IR_OnKeyboardPress_Orig);
	}
}

void Hooks::SetHook(char* hookName, void* pTarget, void* pDetour, void* ppOriginal)
{
	MH_STATUS status = MH_CreateHook(pTarget, pDetour, (void**)ppOriginal);

	if (status == MH_OK) {
		if (MH_EnableHook(pTarget) != MH_OK) {
			MessageBox(NULL, "MH_EnableHook() != MH_OK", hookName, MB_OK | MB_ICONERROR);
		}
	} else {
		printf("MH_STATUS = %i\n", status); // TEST
		MessageBox(NULL, "MH_CreateHook() != MH_OK", hookName, MB_OK | MB_ICONERROR);
	}
}

#ifndef _WIN64
// 1.0004 - 1.0006
void __fastcall Hooks::CWeaponHUD_UpdatePosition_HookNew(Fmatrix& trans, void* _this)
{
	g_pWpnBobbing->Update(trans);
	((UpdatePositionNew)CWeaponHUD_UpdatePosition_Orig)(trans, _this);
}

// 1.0000 - 1.0003
void* __fastcall Hooks::CWeaponHUD_UpdatePosition_HookOld(void* _this, void* _unused, Fmatrix& trans)
{
	g_pWpnBobbing->Update(trans);
	return ((UpdatePositionOld)CWeaponHUD_UpdatePosition_Orig)(_this, trans);
}

void __fastcall Hooks::CDemoRecord_IR_OnKeyboardPress_Hook(DWORD _this, void* _unused, int dik)
{
	if (dik == 28) { // ENTER
		DWORD pCurrentEntity = *((DWORD*)*g_pGameLevel + 8);

		if (pCurrentEntity) {
			// Получаем vtable
			DWORD* vtable = *(DWORD**)pCurrentEntity;

			// Достаём метод по индексу (смещение 80 байт / размер указателя 4 = 20 функция)
			_ForceTransform ForceTransform = (_ForceTransform)vtable[20];

			Fmatrix* m_Camera = (Fmatrix*)(_this + 0x24);
			ForceTransform(pCurrentEntity, m_Camera);

			*((DWORD*)_this - 1) = -1; // fLifeTime = -1; // выход из режима полёта
		}

		return;
	}

	CDemoRecord_IR_OnKeyboardPress_Orig(_this, dik);
}

#else

void __fastcall Hooks::CWeaponHUD_UpdatePosition_HookEE(void* _this, Fmatrix& trans)
{
	g_pWpnBobbing->Update(trans);
	((UpdatePositionEE)CWeaponHUD_UpdatePosition_Orig)(_this, trans);
}

void __fastcall Hooks::CDemoRecord_IR_OnKeyboardPress_Hook(DWORD64 _this, int dik)
{
	if (dik == 28) { // ENTER
		DWORD64 pCurrentEntity = *((DWORD64*)*g_pGameLevel + 19);

		if (pCurrentEntity) {
			// Получаем vtable
			DWORD64* vtable = *(DWORD64**)pCurrentEntity;

			// Достаём метод по индексу (смещение 200 байт / размер указателя 8 = 25 функция)
			_ForceTransform ForceTransform = (_ForceTransform)vtable[25];

			Fmatrix* m_Camera = (Fmatrix*)(_this + 0x38);
			ForceTransform(pCurrentEntity, m_Camera);

			*((DWORD*)_this - 3) = -1; // fLifeTime = -1; // выход из режима полёта
		}

		return;
	}

	/*if (dik == 78) { // NUMPAD PLUS
		for (int i = 0; i < 100; i++) {
			Fmatrix* m_Camera = (Fmatrix*)(_this + i);
			// camera pos должен быть на самой последней строчке. значения _14, _24, _34 всегда 0, значение _44 всегда 1
			printf("===============================\n");
			printf("OFFSET = %i\n", i);
			printf("%f, %f, %f, %f\n", m_Camera->_11, m_Camera->_12, m_Camera->_13, m_Camera->_14);
			printf("%f, %f, %f, %f\n", m_Camera->_21, m_Camera->_22, m_Camera->_23, m_Camera->_24);
			printf("%f, %f, %f, %f\n", m_Camera->_31, m_Camera->_32, m_Camera->_33, m_Camera->_34);
			printf("%f, %f, %f, %f\n", m_Camera->_41, m_Camera->_42, m_Camera->_43, m_Camera->_44);
			printf("===============================\n");
		}
		return;
	}*/

	CDemoRecord_IR_OnKeyboardPress_Orig(_this, dik);
}
#endif
