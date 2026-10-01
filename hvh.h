#pragma once

enum ManualAAMode : int
{
	NONE = -1,
	BACK,
	RIGHT,
	LEFT,
	FORWARD
};

class AdaptiveAngle
{
public:
	float m_yaw;
	float m_dist;
	float m_range;

public:
	// ctor.
	__forceinline AdaptiveAngle( float yaw, float range = 32.f )
	{
		m_yaw = math::NormalizedAngle( yaw );
		m_dist = 0.f;
		m_range = range;
	}
};

enum AntiAimMode : size_t
{
	STAND = 0,
	WALK,
	AIR,
};

class HVH
{
public:
	bool m_should_listen;
	size_t m_mode;

	Player* m_best_target;

	float m_view;
	float m_base_yaw;

	int m_twist_lag;
	int m_updates_cycle;

	bool m_hold_one_tick_shot;

	float m_lastmove_yaw;
	ang_t m_real_angles;

	bool m_lby_timing_broken;

	float m_last_press_time;

	int m_queued_dir;

	int m_manual_direction;
	int m_manual_lastmove;

	bool m_fresh_jump;

	bool m_desync, m_desync_active;
	bool m_update_command[ 150 ];

public:
	void InstantYawUpdate( );
	void ManualAntiaim( );
	float DynamicFreestand( Player* target, std::vector<AdaptiveAngle> angles, bool get_worst = false );
	void SetFreestandInfo( Player* target );

	float  m_left_dist;
	float  m_right_dist;

	float  m_left_close_dist;
	float  m_right_close_dist;

public:
	Player* GetFOVTarget( );
	bool IsYawSideways( vec3_t from, vec3_t to, float yaw );

	void ApplySupression( );
	void DoDesyncJitter( );
	void ApplyTwist( );
	void ApplyFlick( );
	void AntiAim( );
public:
	void SendPacket( );
	void CanHit( );
};

extern HVH g_hvh;