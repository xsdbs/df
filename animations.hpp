#pragma once

struct AnimationBackup_t
{
	vec3_t           m_abs_origin;
	vec3_t           m_velocity, m_abs_velocity;
	int              m_flags, m_eflags;
	float            m_duck;
	bool             m_strafing, m_walking;

	__forceinline void store( Player* player )
	{
		m_strafing = player->m_bStrafing( );
		m_walking = player->m_bIsWalking( );
		m_abs_origin = player->GetAbsOrigin( );
		m_velocity = player->m_vecVelocity( );
		m_abs_velocity = player->m_vecAbsVelocity( );
		m_flags = player->m_fFlags( );
		m_eflags = player->m_iEFlags( );
		m_duck = player->m_flDuckAmount( );
	}

	__forceinline void restore( Player* player )
	{
		player->m_bStrafing( ) = m_strafing;
		player->m_bIsWalking( ) = m_walking;
		player->SetAbsOrigin( m_abs_origin );
		player->m_vecVelocity( ) = m_velocity;
		player->m_vecAbsVelocity( ) = m_abs_velocity;
		player->m_fFlags( ) = m_flags;
		player->m_iEFlags( ) = m_eflags;
		player->m_flDuckAmount( ) = m_duck;
	}
};

class LagRecord {
public:
	bool  m_fake_walk;

	bool  m_hittable = false;
	int   m_hittable_tick = 0;
	bool  m_broke_lc;
	float m_interp_time;

	bool m_first_out_of_pvs;
	int m_receive_tick;
	float m_time_dilation;
	int m_mode;
	int m_lag;
	float m_duck;
	int m_flags;
	float m_anim_time;
	float m_body;
	float m_backwards_angle, m_freestand_angle;
	ang_t m_eye_angles;
	BoneArray* m_bones;
	C_AnimationLayer m_layers[ 13 ];
	float m_poses[ 24 ];
	vec3_t m_velocity, m_estimate_velocity, m_anim_velocity;
	float m_sim_time;
	int m_lby_offset;
	CCSGOPlayerAnimState m_old_state;

	float m_max_feet_yaw, m_min_feet_yaw;

	vec3_t m_mins, m_maxs, m_origin;
	ang_t m_abs_ang;

	// ctor.
	__forceinline LagRecord( ) :
		m_bones {}
	{
	}

	__forceinline bool valid( float custom_time = 0.f, float custom_timebase = 0.f ) {
		float simtime = custom_time == 0.f ? m_sim_time : custom_time;
		float curtime = custom_timebase == 0.f ? g_csgo.m_globals->m_curtime : custom_timebase;

		float correct = g_csgo.m_net->GetLatency( INetChannel::FLOW_OUTGOING );
		correct += g_csgo.m_net->GetLatency( INetChannel::FLOW_INCOMING );
		correct += g_cl.m_lerp;

		correct = std::clamp( correct, 0.f, g_csgo.sv_maxunlag->GetFloat( ) );

		return fabsf( correct - ( curtime - simtime ) ) <= 0.2f;
	}

	__forceinline bool can_delay_shot( ) const
	{
		const auto rtt = g_cl.m_in_latency + g_cl.m_latency;
		return this->m_lag > game::TIME_TO_TICKS( rtt );
	}

	__forceinline int ticks_behind( ) const
	{
		return std::clamp( g_csgo.m_cl->m_server_tick - this->m_receive_tick, 0, g_cl.m_max_lag + 1 );
	}

	__forceinline bool delay_shot( ) const
	{
		const auto rtt = g_cl.m_in_latency + g_cl.m_latency;
		return game::TIME_TO_TICKS( rtt ) + ticks_behind( ) > this->m_lag;
	}

	__forceinline void store( Player* player )
	{
		m_bones = ( BoneArray* )g_csgo.m_mem_alloc->Alloc( sizeof( BoneArray ) * 128 );

		m_receive_tick = g_csgo.m_cl->m_server_tick;

		m_duck = player->m_flDuckAmount( );
		m_body = player->m_flLowerBodyYawTarget( );

		m_eye_angles = player->m_angEyeAngles( );
		m_flags = player->m_fFlags( );
		m_anim_time = m_sim_time = player->m_flSimulationTime( );

		player->GetAnimLayers( m_layers );
		m_velocity = player->m_vecVelocity( );

		m_mins = player->m_vecMins( );
		m_maxs = player->m_vecMaxs( );
		m_origin = player->m_vecOrigin( );
	}

	// dtor.
	~LagRecord( )
	{
		// free heap allocated game mem.
		if ( m_bones )
			g_csgo.m_mem_alloc->Free( m_bones );
	}

	__forceinline void apply( Player* player )
	{
		// get bone cache ptr.
		CBoneCache* cache = &player->m_BoneCache( );

		cache->m_pCachedBones = m_bones;
		cache->m_CachedBoneCount = 128;

		player->m_vecOrigin( ) = m_origin;
		player->SetCollisionBounds( m_mins, m_maxs );

		player->SetAbsAngles( m_abs_ang );
		player->SetAbsOrigin( m_origin );
	}
};

class LagCompensation {
public:
	bool m_defensive;
	bool m_show_backtrack;
	std::deque< LagRecord > m_records {};

	float m_highest_sim_time;
	float m_sim_time;
	float m_spawn;

	AnimationBackup_t m_backup;

	float m_last_shot_time;

	Player* m_player;
	int m_model_index;

	void MatchShot( LagRecord& record, LagRecord* previous );

	bool EstimateVelocity( Player* m_player, LagRecord& record, LagRecord* previous, LagRecord* pre_previous, float max_speed );

	void UpdateAnimations( LagRecord& record );
	void OnNetUpdate( Player* player );
public: // resolver variables
	enum LogReason_t
	{
		DEATH_CLAMPED = 0,
		DEATH_UNCLAMPED,
		MISS,
		HIT,
		OVERLAP
	};

	enum LbySide_t
	{
		EQUAL = 0,
		LEFT,
		RIGHT
	};

	enum LogDelta_t
	{
		LBY_EYE = 0,
		BACKWARDS_EYE,
		FREESTAND_EYE
	};

	struct ResolverLog_t
	{
		int m_reason;
		bool m_on_ground;

		float m_confidence;

		int m_lby_side;

		std::array<float, 3> m_deltas;
	};

	std::vector<ResolverLog_t> m_shot_log;
	std::vector<float> m_blacklisted_angles;

	enum FakeColor_t
	{
		DONT_DRAW = 0,
		WHITE,
		GREEN,
		RED,
	};

	enum ResolverConfidence_t
	{
		VERY_LOW = 0,
		LOW,
		MEDIUM,
		HIGH,
		VERY_HIGH,
		MAX
	};

	// if they flick to feet yaw every time, it will keep delaying over and over again.
	// otherwise its just a static offset.
	enum LBYDelayType_t
	{
		CONSTANT = 0,
		REPEATING
	};

	int m_old_979_count;
	int m_979_count;

	int m_resolver_confidence;

	float m_override;
	bool m_override_agreed;

	Player* m_facing_player;

	float m_velocity_dir;
	float m_air_angle;
	float m_freestand_angle;

	int m_ticks_moved;
	int m_missed_shots;
	bool m_scary_lby;
	float m_last_update_time;
	int m_updates_post_lby;
	int m_logged_timer;

	std::vector<int> m_blacklisted_timings;

	struct
	{
		float m_times_clamped;
		float m_times_unclamped;
	} m_clamp_log;

	// offset, time when logged.
	std::deque<std::pair<int, float>> m_update_offsets;
	float m_lby_timer;
	bool m_cant_predict_lby;

	bool m_detected_desync;

	// data about the LBY proxy.
	float m_body;
	bool m_update_found;
	float m_old_body;
	float m_last_lby_delta;

	bool m_last_change_yaw;
	bool m_lby_change_standing;
	bool m_lby_changed;

	float m_max_feet_yaw;
	float m_min_feet_yaw;
	bool m_went_dormant;

	bool m_get_death_yaw;
	void OnImpact( ImpactRecord* impact );

	void reset( )
	{
		m_highest_sim_time = -1.f;
		m_last_shot_time = 0.f;
		m_sim_time = 0.f;

		m_went_dormant = true;
		m_get_death_yaw = false;
		m_last_lby_delta = 0.f;
		m_last_change_yaw = 0.f;
		m_lby_changed = false;

		m_cant_predict_lby = true;
		m_lby_change_standing = false;
		m_facing_player = nullptr;

		m_records.clear( );
	}
};

class Animations {
public:
	std::array< LagCompensation, 64 > m_players;
};

extern Animations g_lagcompensation;