#pragma once

#define	MAX_EDICTS (1 << 11)

class Sequence
{
public:
	float m_time;
	int   m_state;
	int   m_seq;

public:
	__forceinline Sequence( ) : m_time {}, m_state {}, m_seq {} { };
	__forceinline Sequence( float time, int state, int seq ) : m_time { time }, m_state { state }, m_seq { seq } { };
};

class NetPos
{
public:
	float  m_time;
	vec3_t m_pos;

public:
	__forceinline NetPos( ) : m_time {}, m_pos {} { };
	__forceinline NetPos( float time, vec3_t pos ) : m_time { time }, m_pos { pos } { };
};

class Client
{
public:
	// hack thread.
	static ulong_t __stdcall init( void* arg );

	void StartMove( CUserCmd* cmd );
	void EndMove( CUserCmd* cmd );
	void BackupPlayers( bool restore );
	void DoMove( );
	void DrawHUD( );
	void UnlockHiddenConvars( );
	void KillFeed( );
	void BuyBot( );

	void OnPaint( );
	void OnMapload( );
	void OnTick( CUserCmd* cmd );

	// debugprint function.
	void print( const std::string text, ... );
	void print( Color text_color, const std::string text, ... );

	bool CanFireWeapon( );
	void AutoRevolver( );
	void UpdateIncomingSequences( );
	void HandleFakeLatency( );
	bool InstantActions( bool weapon_switch, int new_buttons );
	void HandleKillsays( );
public:
	// local player variables.
	Player* m_local;
	bool	         m_processing;
	int	             m_flags;
	float            m_forward_move;
	float            m_side_move;
	vec3_t	         m_shoot_pos;
	bool	         m_shot;
	bool	         m_old_shot;
	float            m_fakelatency_amt;
	bool             m_change_world;
	float m_avg_frametime;
	int m_avg_fps;
	bool m_throttled;

	bool             m_landed_on_anim_update;
	float            m_last_spawn_time;

	float            m_abs_yaw;
	float            m_render_abs;

	float			 m_left_thickness[ 64 ], m_right_thickness[ 64 ], m_at_target_angle[ 64 ];

	// target sorting meme
	std::deque<int> m_sorted_players;
	std::deque<int> m_unsorted_players;

	// active weapon variables.
	Weapon* m_weapon;
	int         m_weapon_id;
	WeaponInfo* m_weapon_info;
	int         m_weapon_type;
	bool        m_weapon_fire;
	bool        m_player_fire;

	penetration::PenetrationOutput_t m_pen_data;

	// revolver variables.
	int	 m_revolver_cock;
	int	 m_revolver_query;
	bool m_revolver_fire;
	bool m_old_revolver_fire;

	// general game varaibles.
	bool     m_round_end;
	Stage_t	 m_stage;
	int	     m_max_lag;
	int      m_lag;
	int	     m_old_lag;
	bool* m_packet;
	bool* m_final_packet;
	bool	 m_old_packet;
	float	 m_lerp;
	float    m_latency;
	float    m_in_latency;
	int      m_tickrate;
	float      m_framerate;
	int      m_width, m_height;
	int m_weapon_select;
	bool m_pressing_move;
	float m_last_kill_time;

	float m_round_end_time;
	void BuyAwpBeforeKamiDoes( int cur_cmd );

	// usercommand variables.
	CUserCmd* m_cmd;
	int	      m_tick;
	int	      m_rate;
	int	      m_buttons;
	int       m_old_buttons;
	ang_t     m_view_angles;
	ang_t	  m_strafe_angles;
	vec3_t	  m_forward_dir;

	float m_view_yaw_change;

	// instant angleupdate meme
	ang_t m_sent_angles;
	ang_t last_choked_angles;

	// haulback check for revolver
	bool m_can_reclaim;

	std::deque< int >      m_sent_cmds;
	std::deque< Sequence > m_sequences;
	std::deque< NetPos >   m_net_pos;

	float m_last_prediction_error_time;

	CPlayerResource* player_resource;
	// animation variables.
	ang_t  m_angle;
	float  m_enemy_body_pred;
	float  m_body_pred;
	bool   m_lagcomp;

	float  m_pred_aim_matrix_width_range;

	void UpdateAnimRecord( CUserCmd* cmd );

	struct AnimUpdate_t
	{
		int m_cmd_num;
		int m_hide_angle;
		int m_viewmodel_act;
	};
	std::deque<AnimUpdate_t> m_update_records;

	struct AnimSmooth_t
	{
		float m_old_poses[ 24 ];
		float m_goal_poses[ 24 ];

		C_AnimationLayer m_old_layers[ 13 ];
		C_AnimationLayer m_goal_layers[ 13 ];

		float m_old_abs;
		float m_goal_abs;

		float m_old_time;
		float m_time;

		bool m_do_smoothing;
	} m_smooth_data;

	int m_hide_angle;

	bool m_should_read_packets;

	// hack username.
	std::string m_user;
};

extern Client g_cl;