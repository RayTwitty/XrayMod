#pragma once
#include "Patcher.h"
#include "MinHook\MinHook.h"
#include "wpn_bobbing_la.h"

#ifndef _WIN64
typedef void* (__fastcall* UpdatePositionNew)(Fmatrix& trans, void* _this); // 1.0004 - 1.0006
typedef void* (__thiscall* UpdatePositionOld)(void* _this, Fmatrix& trans); // 1.0000 - 1.0003
typedef void (__thiscall* _IR_OnKeyboardPress)(DWORD _this, int dik);
typedef void (__thiscall* _ForceTransform)(DWORD _this, Fmatrix* m);
#else
// Enhanced Edition
typedef void (__fastcall* UpdatePositionEE)(void* _this, Fmatrix& trans);
typedef void (__fastcall* _IR_OnKeyboardPress)(DWORD64 _this, int dik);
typedef void* (__fastcall* _ForceTransform)(DWORD64 _this, Fmatrix* m);
#endif

class Hooks : public Patcher
{
public:
	Hooks();
	static void SetHook(char* hookName, void* pTarget, void* pDetour, void* ppOriginal);

#ifndef _WIN64
	static void __fastcall CWeaponHUD_UpdatePosition_HookNew(Fmatrix& trans, void* _this); // 1.0004 - 1.0006
	static void* __fastcall CWeaponHUD_UpdatePosition_HookOld(void* _this, void* _unused, Fmatrix& trans); // 1.0000 - 1.0003
	static void __fastcall CDemoRecord_IR_OnKeyboardPress_Hook(DWORD _this, void* _unused, int dik);
#else
	// Enhanced Edition
	static void __fastcall CWeaponHUD_UpdatePosition_HookEE(void* _this, Fmatrix& trans);
	static void __fastcall CDemoRecord_IR_OnKeyboardPress_Hook(DWORD64 _this, int dik);
#endif
};

