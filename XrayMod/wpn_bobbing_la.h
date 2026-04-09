#include "Utils.h"
#include "MinHook\MinHook.h"

#define BOBBING_SECT "wpn_bobbing_effector"

struct Fvector
{
	float x, y, z;

	void set(const Fvector& other);
};

struct Fmatrix
{
	union {
		struct {
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
		struct {
			Fvector i;	float	_14_;
			Fvector j;	float	_24_;
			Fvector k;	float	_34_;
			Fvector c;	float	_44_;
		};
		float m[4][4];
	};

	void mul(const Fmatrix& a, const Fmatrix& b);
	void setHPB(float h, float p, float b);
};

class CWeaponBobbing
{
public:
	CWeaponBobbing();
	virtual ~CWeaponBobbing();
	void Load();
	void Update(Fmatrix& m);
	void CheckState();

private:
	float	fTime;
	Fvector	vAngleAmplitude;
	float	fYAmplitude;
	float	fSpeed;

	u32		dwMState;
	float	fReminderFactor;
	bool	is_limping;
	bool	m_bZoomMode;

	float	m_fAmplitudeRun;
	float	m_fAmplitudeWalk;
	float	m_fAmplitudeLimp;

	float	m_fSpeedRun;
	float	m_fSpeedWalk;
	float	m_fSpeedLimp;
};
