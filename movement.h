#pragma once

class Movement
{
public:
	bool m_autopeek_retreating;
	bool m_fakewalking;
	bool m_computing;

	int m_fake_walk_ticks_left;
	bool m_fakewalk_lby_stop;

	bool m_stop_the_fakewalk;

	vec3_t m_autopeek_pos;
	bool m_autopeek_retreat;
	bool m_should_quickswitch;

public:
	bool m_transition_flick;
	std::pair<bool, bool> m_transition_stage;
	bool m_transition_reset;

public:
	void EdgeJump( );
	void AutoStrafe( );
	void JumpRelated( );
	void FixMove( CUserCmd* cmd, const ang_t& old_angles, bool local = true );
	void AutoPeek( );
	void ProcessAutoStop( CUserCmd* cmd );
	void QuickStop( CUserCmd* cmd );
	void StopToSpeed( float speed, CMoveData* mv, Player* player = g_cl.m_local );
	void StopToSpeed( float speed, CUserCmd* cmd, Player* player = g_cl.m_local );
	bool StopUntilAccurate( );
	void FakeWalk( );
};

extern Movement g_movement;