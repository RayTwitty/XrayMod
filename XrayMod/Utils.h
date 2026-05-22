#pragma once
#include "Patcher.h"
#include <cmath>
#include <math.h>
#include <stdio.h>
#include <stdint.h>

typedef unsigned u32;
typedef unsigned long long u64;
typedef char string256[256];

enum EGameFlags
{
	HUD_BOBBING = (1 << 0),
};

#ifndef _WIN64
typedef bool (__cdecl* _isActorAccelerated)(u32 mstate, bool ZoomMode);
typedef bool (__thiscall* _CActorCondition_IsLimping)(DWORD _this);

enum PATCH
{
	UNKNOWN,
	p_1_0000,
	p_1_0001,
	p_1_0002,
	p_1_0003,
	p_1_0004,
	p_1_0005,
	p_1_0006,
};
#else
typedef bool (__fastcall* _isActorAccelerated)(u32 mstate, bool ZoomMode);
typedef bool (__fastcall* _CActorCondition_IsLimping)(DWORD64 _this);
#endif

class Utils : public Patcher
{
public:
	Utils();

#ifndef _WIN64
	static PATCH Patch;
	static _isActorAccelerated isActorAccelerated_Address;
#else
	static _isActorAccelerated isActorAccelerated;
#endif

	static DWORD fTimeDelta_offset;
	static DWORD mstate_real_offset;
	static DWORD m_entity_condition_offset;
	static DWORD m_bZoomAimingMode_offset;

	static void InitOffsets();
	static u32 get_state();
	static bool IsLimping();
	static bool IsZoomAimingMode();
	static float GetFTimeDelta();

#ifndef _WIN64
	static DWORD GetAddrFromRelativeInstr(DWORD instr_addr, int instr_len, int rel_offset);
	static void InitVersion();
	static bool __cdecl isActorAccelerated(u32 mstate, bool ZoomMode);

	static DWORD device;
	static DWORD* g_actor;
	static float* fTimeDelta;

#else

	static DWORD64 GetAddrFromRelativeInstr(DWORD64 instr_addr, int instr_len, int rel_offset);

	static DWORD64* g_actor;
	static float* fTimeDelta;
#endif

	static void GetString(const char* section_name, const char* str_name, const char* default_str, char* result, DWORD size);
	static bool GetBool(const char* section_name, const char* bool_name, bool default_bool);
	static float GetFloat(const char* section_name, const char* param_name, float param_default);
};

