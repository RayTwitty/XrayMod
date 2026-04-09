// https://github.com/joye-ramone/xray_xp_dev/commit/77d8bb876df84e69ee589232577fc1b7886b3663#diff-38bdb068bbc18c1790e04671a77c5407

#include "wpn_bobbing_la.h"

/* some X-Ray specific types */
float _cos(float a) { return cos(a); }
float _sin(float a) { return sin(a); }
float _abs(float a) { return fabs(a); }

void Fvector::set(const Fvector& other)
{
	x = other.x;
	y = other.y;
	z = other.z;
}

void Fmatrix::mul(const Fmatrix &a, const Fmatrix &b)
{
	_11 = a._11*b._11 + a._21*b._12 + a._31*b._13 + a._41*b._14;
	_12 = a._12*b._11 + a._22*b._12 + a._32*b._13 + a._42*b._14;
	_13 = a._13*b._11 + a._23*b._12 + a._33*b._13 + a._43*b._14;
	_14 = a._14*b._11 + a._24*b._12 + a._34*b._13 + a._44*b._14;

	_21 = a._11*b._21 + a._21*b._22 + a._31*b._23 + a._41*b._24;
	_22 = a._12*b._21 + a._22*b._22 + a._32*b._23 + a._42*b._24;
	_23 = a._13*b._21 + a._23*b._22 + a._33*b._23 + a._43*b._24;
	_24 = a._14*b._21 + a._24*b._22 + a._34*b._23 + a._44*b._24;

	_31 = a._11*b._31 + a._21*b._32 + a._31*b._33 + a._41*b._34;
	_32 = a._12*b._31 + a._22*b._32 + a._32*b._33 + a._42*b._34;
	_33 = a._13*b._31 + a._23*b._32 + a._33*b._33 + a._43*b._34;
	_34 = a._14*b._31 + a._24*b._32 + a._34*b._33 + a._44*b._34;

	_41 = a._11*b._41 + a._21*b._42 + a._31*b._43 + a._41*b._44;
	_42 = a._12*b._41 + a._22*b._42 + a._32*b._43 + a._42*b._44;
	_43 = a._13*b._41 + a._23*b._42 + a._33*b._43 + a._43*b._44;
	_44 = a._14*b._41 + a._24*b._42 + a._34*b._43 + a._44*b._44;

	return;
}

void Fmatrix::setHPB(float h, float p, float b)
{
	float sh = std::sin(h);
	float ch = std::cos(h);
	float sp = std::sin(p);
	float cp = std::cos(p);
	float sb = std::sin(b);
	float cb = std::cos(b);

#if 1
	_11 = ch*cb - sh*sp*sb;
	_12 = -cp*sb;
	_13 = ch*sb*sp + sh*cb;
	_14 = 0;

	_21 = sp*sh*cb + ch*sb;
	_22 = cb*cp;
	_23 = sh*sb - sp*ch*cb;
	_24 = 0;

	_31 = -cp*sh;
	_32 = sp;
	_33 = ch*cp;
	_34 = 0;

	_41 = 0;
	_42 = 0;
	_43 = 0;
	_44 = float(1);
#else
	i.set(ch*cb - sh*sp*sb, -cp*sb, ch*sb*sp + sh*cb); _14 = 0;
	j.set(sp*sh*cb + ch*sb, cb*cp, sh*sb - sp*ch*cb); _24 = 0;
	k.set(-cp*sh, sp, ch*cp); _34 = 0;
	c.set(0, 0, 0); _44 = T(1);
#endif

	return;
}

typedef enum
{
	mcFwd		= (1ul << 0ul),
	mcBack		= (1ul << 1ul),
	mcLStrafe	= (1ul << 2ul),
	mcRStrafe	= (1ul << 3ul),
	mcCrouch	= (1ul << 4ul),
	mcAccel		= (1ul << 5ul),
	mcTurn		= (1ul << 6ul),
	mcJump		= (1ul << 7ul),
	mcFall		= (1ul << 8ul),
	mcLanding	= (1ul << 9ul),
	mcLanding2	= (1ul << 10ul),
	mcClimb		= (1ul << 11ul),
	mcSprint	= (1ul << 12ul),
	mcLLookout	= (1ul << 13ul),
	mcRLookout	= (1ul << 14ul),
	mcAnyMove	= (mcFwd | mcBack | mcLStrafe | mcRStrafe),
	mcAnyAction = (mcAnyMove | mcJump | mcFall | mcLanding | mcLanding2), //mcTurn|
	mcAnyState	= (mcCrouch | mcAccel | mcClimb | mcSprint),
	mcLookout	= (mcLLookout | mcRLookout),
} ACTOR_DEFS;

bool fsimilar(float a, float b)
{
	return fabs(a - b) < 0.0000100f; //0.0001;
}

/* bobbing effector class */
#define CROUCH_FACTOR	0.75f
#define SPEED_REMINDER	5.f 

CWeaponBobbing::CWeaponBobbing()
{
	Load();
}

CWeaponBobbing::~CWeaponBobbing()
{
}

/*
// настройки из OGSR Engine
[wpn_bobbing_effector]
run_amplitude			=	0.0075
walk_amplitude			=	0.005
limp_amplitude			=	0.011
run_speed				=	6.74
walk_speed				=   6.26
limp_speed				=	4.6

// настройки из какого-то мода раскачки для шестого патча
[wpn_bobbing_effector]
run_amplitude			=	0.0045
walk_amplitude			=	0.0035
limp_amplitude			=	0.0025
run_speed				=	8.0
walk_speed				=   6.0
limp_speed				=	5.0
*/

void CWeaponBobbing::Load()
{
	fTime			= 0;
	fReminderFactor	= 0;
	is_limping		= false;

	m_fAmplitudeRun		= Utils::GetFloat(BOBBING_SECT, "run_amplitude", 0.0045f); //pSettings->r_float(BOBBING_SECT, "run_amplitude");
	m_fAmplitudeWalk	= Utils::GetFloat(BOBBING_SECT, "walk_amplitude", 0.0035f); //pSettings->r_float(BOBBING_SECT, "walk_amplitude");
	m_fAmplitudeLimp	= Utils::GetFloat(BOBBING_SECT, "limp_amplitude", 0.0025f); //pSettings->r_float(BOBBING_SECT, "limp_amplitude");

	m_fSpeedRun			= Utils::GetFloat(BOBBING_SECT, "run_speed", 8.0f); //pSettings->r_float(BOBBING_SECT, "run_speed");
	m_fSpeedWalk		= Utils::GetFloat(BOBBING_SECT, "walk_speed", 6.0f); //pSettings->r_float(BOBBING_SECT, "walk_speed");
	m_fSpeedLimp		= Utils::GetFloat(BOBBING_SECT, "limp_speed", 5.0f); //pSettings->r_float(BOBBING_SECT, "limp_speed");
}

/*
* в функции CActorCondition::UpdateCondition
* можно найти isActorAccelerated, mstate_real и m_bZoomAimingMode
* а найти её можно по таким строкам
* "effector_alcohol"
* "effector_psy_health"
*/
void CWeaponBobbing::CheckState()
{
	dwMState		= Utils::get_state();
	is_limping		= Utils::IsLimping();
	m_bZoomMode		= Utils::IsZoomAimingMode();
	fTime			+= Utils::GetFTimeDelta();
}

void CWeaponBobbing::Update(Fmatrix &m)
{
	CheckState();
	if (dwMState & ACTOR_DEFS::mcAnyMove)
	{
		if (fReminderFactor < 1.f)
			fReminderFactor += SPEED_REMINDER * Utils::GetFTimeDelta();
		else						
			fReminderFactor = 1.f;
	}
	else
	{
		if (fReminderFactor > 0.f)
			fReminderFactor -= SPEED_REMINDER * Utils::GetFTimeDelta();
		else			
			fReminderFactor = 0.f;
	}
	if (!fsimilar(fReminderFactor, 0))
	{
		Fvector dangle;
		Fmatrix		R, mR;
		float k		= (dwMState & ACTOR_DEFS::mcCrouch) ? CROUCH_FACTOR : 1.f;

		float A, ST;

		if (Utils::isActorAccelerated(dwMState, m_bZoomMode))
		{
			A	= m_fAmplitudeRun * k;
			ST	= m_fSpeedRun * fTime * k;
		}
		else if (is_limping)
		{
			A = m_fAmplitudeLimp * k;
			ST = m_fSpeedLimp * fTime * k;
		}
		else
		{
			A	= m_fAmplitudeWalk * k;
			ST	= m_fSpeedWalk * fTime * k;
		}

		float _sinA	= _abs(_sin(ST) * A) * fReminderFactor;
		float _cosA	= _cos(ST) * A * fReminderFactor;

		m.c.y		+=	_sinA;
		dangle.x	=	_cosA;
		dangle.z	=	_cosA;
		dangle.y	=	_sinA;


		R.setHPB(dangle.x, dangle.y, dangle.z);

		mR.mul		(m, R);

		m.k.set(mR.k);
		m.j.set(mR.j);
	}
}
