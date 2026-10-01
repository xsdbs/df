#pragma once

class ShotRecord;
class LagCompensation;
class Resolver
{
public:
	enum Modes : size_t
	{
		RESOLVE_NONE,
		RESOLVE_AIR,
		RESOLVE_STAND,
		RESOLVE_UPDATE_AIR,
		RESOLVE_UPDATE, RESOLVE_THRESHOLD = RESOLVE_UPDATE,
		RESOLVE_MOVE,
	};

public:
	void OnBodyUpdate( Player* player, float value );

public:
	float SnapToNearest( float angle, std::vector<float> angles );
	std::pair<float, bool> FreestandYaw( LagCompensation* data, Player* from_player, std::vector<AdaptiveAngle> angles, bool get_worst = false ); // freestand angle & whether that angle is actually hidden or not.

public:
	void CheckAnimations( LagRecord& record, LagCompensation* data );

	int GetTimingOffset( LagCompensation* data );

	void SetConfidence( LagCompensation* data, LagRecord& record, bool balance_adjust, int clamp_preference );
	void Resolve( LagCompensation* data, LagRecord& record );

public:
	void SetFacingPlayer( LagCompensation* data, LagRecord& record, LagRecord* previous );

public:
	enum ClampPreference_t
	{
		unsure = 0,
		bias_clamped,
		bias_unclamped,
		neutral
	};

	void ResolveStand( LagCompensation* data, LagRecord& record );
	void ResolveWalk( LagCompensation* data, LagRecord& record, LagRecord* previous );
	void ResolveAir( LagCompensation* data, LagRecord& record );

	void CheckUnclamped( LagCompensation* data, float angle );

	std::vector<float> m_test_angles;

public:
	void process_override();
};

extern Resolver g_resolver;