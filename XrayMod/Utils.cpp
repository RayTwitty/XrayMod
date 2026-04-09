#include "Utils.h"

DWORD Utils::fTimeDelta_offset;
DWORD Utils::mstate_real_offset;
DWORD Utils::m_entity_condition_offset;
DWORD Utils::m_bZoomAimingMode_offset;

#ifndef _WIN64

PATCH Utils::Patch;
_isActorAccelerated Utils::isActorAccelerated_Address;

DWORD Utils::device;
DWORD* Utils::g_actor;
float* Utils::fTimeDelta;

#else

_isActorAccelerated Utils::isActorAccelerated;

DWORD64* Utils::g_actor;
float* Utils::fTimeDelta;

#endif

Utils::Utils()
{
#ifndef _WIN64
	InitVersion();
	InitOffsets();

	device = (DWORD)GetProcAddress(GetModuleHandle(NULL), "?Device@@3VCRenderDevice@@A");
	fTimeDelta = (float*)(device + fTimeDelta_offset);

	MODULEINFO miXRGame = GetModuleData("xrGame.dll");

	// E8 ? ? ? ? F3 0F 10 66 ? F3 0F 5E 66 - 1.0004
	DWORD call_isActorAccelerated = FindPattern((DWORD)miXRGame.lpBaseOfDll, miXRGame.SizeOfImage,
		"\xE8\x00\x00\x00\x00\xF3\x0F\x10\x66\x00\xF3\x0F\x5E\x66", "x????xxxx?xxxx");

	if (call_isActorAccelerated == NULL) { // old
		// 83 3D ? ? ? ? ? 0F 84 ? ? ? ? 53 56 E8 ? ? ? ? 8B 88 ? ? ? ? E8 - 1.0000 - 1.0003
		g_actor = *(DWORD**)(FindPattern(
			(DWORD)miXRGame.lpBaseOfDll,
			miXRGame.SizeOfImage,
			"\x83\x3D\x00\x00\x00\x00\x00\x0F\x84\x00\x00\x00\x00\x53\x56\xE8\x00\x00\x00\x00\x8B\x88\x00\x00\x00\x00\xE8",
			"xx?????xx????xxx????xx????x") + 2);

		// A1 ? ? ? ? C1 E8 02
		isActorAccelerated_Address = (_isActorAccelerated)FindPattern((DWORD)miXRGame.lpBaseOfDll, miXRGame.SizeOfImage,
			"\xA1\x00\x00\x00\x00\xC1\xE8\x02", "x????xxx");
	} else {
		// FF 24 8D ? ? ? ? A1 - 1.0004 - 1.0006
		g_actor = *(DWORD**)(FindPattern((DWORD)miXRGame.lpBaseOfDll, miXRGame.SizeOfImage, "\xFF\x24\x8D\x00\x00\x00\x00\xA1", "xxx????x") + 8);
		isActorAccelerated_Address = (_isActorAccelerated)GetAddrFromRelativeInstr(call_isActorAccelerated, 5, 1);
	}

#else
	// Enhanced Edition
	InitOffsets();

	// c5 fa 5c 0d ? ? ? ? c5 fa 11 49 ? b8
	DWORD64 vsubss_fTimeDelta = FindPatternInEXE("\xc5\xfa\x5c\x0d\x00\x00\x00\x00\xc5\xfa\x11\x49\x00\xb8", "xxxx????xxxx?x");
	fTimeDelta = (float*)GetAddrFromRelativeInstr(vsubss_fTimeDelta, 8, 4);

	// 4c 8b 05 ? ? ? ? 48 8b fa c5
	DWORD64 mov_g_actor = FindPatternInEXE("\x4c\x8b\x05\x00\x00\x00\x00\x48\x8b\xfa\xc5", "xxx????xxxx");
	g_actor = (DWORD64*)GetAddrFromRelativeInstr(mov_g_actor, 7, 3);

	// 8B 05 ? ? ? ? C1 E8 02 F6 C1 20 74 02 F6 D0 24 01 F7 C1
	isActorAccelerated = (_isActorAccelerated)FindPatternInEXE("\x8B\x05\x00\x00\x00\x00\xC1\xE8\x02\xF6\xC1\x20\x74\x02\xF6\xD0\x24\x01\xF7\xC1", "xx????xxxxxxxxxxxxxx");
#endif
}

u32 Utils::get_state()
{
	// оффсет можно найти где-то там по "effector_psy_health"
	return *(u32*)(*g_actor + mstate_real_offset); // mstate_real
}

bool Utils::IsLimping()
{
	// оффсет можно найти в CActor::CanMove по строке "cant_walk_weight"
#ifndef _WIN64
	DWORD conditions = *(DWORD*)(*g_actor + m_entity_condition_offset); // m_entity_condition
	DWORD* conditions_vtable = *(DWORD**)conditions;
#else
	DWORD64 conditions = *(DWORD64*)(*g_actor + m_entity_condition_offset); // m_entity_condition
	DWORD64* conditions_vtable = *(DWORD64**)conditions;
#endif

	_CActorCondition_IsLimping IsLimping = (_CActorCondition_IsLimping)conditions_vtable[5];

	return IsLimping(conditions);
}

bool Utils::IsZoomAimingMode()
{
	return *(bool*)(*g_actor + m_bZoomAimingMode_offset); // m_bZoomAimingMode
}

float Utils::GetFTimeDelta()
{
	return *fTimeDelta;
}

void Utils::InitOffsets()
{
#ifndef _WIN64
	if (Patch == p_1_0006) {
		fTimeDelta_offset = 0x1F8;
	}
	else if (Patch == p_1_0005 || Patch == p_1_0004) {
		fTimeDelta_offset = 0x1D4;
	}
	else if (Patch == p_1_0003 || Patch == p_1_0002 || Patch == p_1_0001 || Patch == p_1_0000) {
		fTimeDelta_offset = 0x1B0;
	}

	/////////////////////////////////////

	if (Patch == p_1_0006 || Patch == p_1_0003 || Patch == p_1_0002 || Patch == p_1_0001) {
		mstate_real_offset = 0x598;
		m_bZoomAimingMode_offset = 0x5C8;
		m_entity_condition_offset = 0x93C;
	}
	else if (Patch == p_1_0005 || Patch == p_1_0004) {
		mstate_real_offset = 0x52C;
		m_bZoomAimingMode_offset = 0x55C;
		m_entity_condition_offset = 0x90C;
	}
	else if (Patch == p_1_0000) {
		mstate_real_offset = 0x588;
		m_bZoomAimingMode_offset = 0x5B8;
		m_entity_condition_offset = 0x92C;
	}
#else
	// Enhanced Edition
	mstate_real_offset = 0x938;
	m_bZoomAimingMode_offset = 0x96C;
	m_entity_condition_offset = 0xD48;
#endif
}

#ifndef _WIN64

DWORD Utils::GetAddrFromRelativeInstr(DWORD instr_addr, int instr_len, int rel_offset)
{
	return (instr_addr + instr_len + *(int32_t*)(instr_addr + rel_offset));
}

void Utils::InitVersion()
{
	MODULEINFO miXRGameSpy = GetModuleData("xrGameSpy.dll");

	// Т.к. функция проверки версии из xrGameSpy ненадёжна и может брать версию из реестра,
	// версия может отличаться от реальной, поэтому мы будем искать строку версии прямо в дллке.
	DWORD version_addr = FindPattern((DWORD)miXRGameSpy.lpBaseOfDll, miXRGameSpy.SizeOfImage, "1.0006\0", "xxxxxxxx");
	if (version_addr != NULL) {
		Utils::Patch = p_1_0006;
	} else {
		version_addr = FindPattern((DWORD)miXRGameSpy.lpBaseOfDll, miXRGameSpy.SizeOfImage, "1.0005\0", "xxxxxxxx");
		if (version_addr != NULL) {
			Utils::Patch = p_1_0005;
		} else {
			version_addr = FindPattern((DWORD)miXRGameSpy.lpBaseOfDll, miXRGameSpy.SizeOfImage, "1.0004\0", "xxxxxxxx");
			if (version_addr != NULL) {
				Utils::Patch = p_1_0004;
			} else {
				version_addr = FindPattern((DWORD)miXRGameSpy.lpBaseOfDll, miXRGameSpy.SizeOfImage, "1.0003\0", "xxxxxxxx");
				if (version_addr != NULL) {
					Utils::Patch = p_1_0003;
				} else {
					version_addr = FindPattern((DWORD)miXRGameSpy.lpBaseOfDll, miXRGameSpy.SizeOfImage, "1.0002\0", "xxxxxxxx");
					if (version_addr != NULL) {
						Utils::Patch = p_1_0002;
					} else {
						version_addr = FindPattern((DWORD)miXRGameSpy.lpBaseOfDll, miXRGameSpy.SizeOfImage, "1.1009\0", "xxxxxxxx");
						if (version_addr != NULL) {
							Utils::Patch = p_1_0001;
						} else {
							version_addr = FindPattern((DWORD)miXRGameSpy.lpBaseOfDll, miXRGameSpy.SizeOfImage, "0.1009\0", "xxxxxxxx");
							if (version_addr != NULL) {
								Utils::Patch = p_1_0000;
							} else {
								Utils::Patch = UNKNOWN;
								MessageBox(NULL, "Unknown game version.", NULL, MB_OK);
							}
						}
					}
				}
			}
		}
	}
}

__declspec(naked) bool __cdecl Utils::isActorAccelerated(u32 mstate, bool ZoomMode)
{
	if (Patch == p_1_0000 || Patch == p_1_0001 || Patch == p_1_0002 || Patch == p_1_0003) {
		__asm { // old
			jmp Utils::isActorAccelerated_Address
		}
	} else {
		// т.к. в патчах оптимизация поломала соглашение __cdecl у этой функции, то придётся её вызывать через asm
		__asm { // new
			// стек:
			// [esp+4]  = mstate
			// [esp+8]  = ZoomMode

			mov ecx, [esp + 4]   // 1-й аргумент -> ecx
			push [esp + 8]       // 2-й аргумент -> стек

			call Utils::isActorAccelerated_Address

			add esp, 4         // чистим push

			ret
		}
	}
}

#else

DWORD64 Utils::GetAddrFromRelativeInstr(DWORD64 instr_addr, int instr_len, int rel_offset)
{
	return (instr_addr + instr_len + *(int32_t*)(instr_addr + rel_offset));
}

#endif

void Utils::GetString(const char* section_name, const char* str_name, const char* default_str, char* result, DWORD size)
{
	GetPrivateProfileString(section_name, str_name, default_str, result, size, ".\\XrayMod.ini");
}

bool Utils::GetBool(const char* section_name, const char* bool_name, bool default_bool)
{
	string256 str;
	GetString(section_name, bool_name, (default_bool ? "true" : "false"), str, sizeof(str));
	return (strcmp(str, "true") == 0) || (strcmp(str, "yes") == 0) || (strcmp(str, "on") == 0) || (strcmp(str, "1") == 0);
}

float Utils::GetFloat(const char* section_name, const char* param_name, float param_default)
{
	string256 str;
	float param;

	GetString(section_name, param_name, "", str, sizeof(str));
	if (!str[0] || sscanf(str, "%f", &param) != 1)
		return param_default;

	return param;
}
