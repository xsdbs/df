#pragma once

class CMoveData
{
public:
	bool        m_bFirstRunOfFunctions : 1;
	bool        m_bGameCodeMovedPlayer : 1;
	bool        m_bNoAirControl : 1;
	CBaseHandle m_nPlayerHandle;
	int         m_nImpulseCommand;
	ang_t       m_vecViewAngles;
	ang_t       m_vecAbsViewAngles;
	int         m_nButtons;
	int         m_nOldButtons;
	float       m_flForwardMove;
	float       m_flSideMove;
	float       m_flUpMove;
	float       m_flMaxSpeed;
	float       m_flClientMaxSpeed;
	vec3_t      m_vecVelocity;
	vec3_t      m_vecOldVelocity;
	float       m_unknown;
	ang_t       m_vecAngles;
	ang_t       m_vecOldAngles;
	float       m_outStepHeight;
	vec3_t      m_outWishVel;
	vec3_t      m_outJumpVel;
	vec3_t      m_vecConstraintCenter;
	float       m_flConstraintRadius;
	float       m_flConstraintWidth;
	float       m_flConstraintSpeedFactor;
	bool        m_bConstraintPastRadius;
	vec3_t      m_vecAbsOrigin;
};

class CGameTrace;
class IMoveHelper
{
public:
	// indexes for virtuals and hooks.
	enum indices : size_t
	{
		PROCESSIMPACTS = 4,
	};

	// Methods associated with a particular entity
	virtual	char const* GetName( int handle ) const = 0;

	// sets the entity being moved
	virtual void	SetHost( Entity* host ) = 0;

	// Adds the trace result to touch list, if contact is not already in list.
	virtual void	ResetTouchList( void ) = 0;
	virtual bool	AddToTouched( const CGameTrace& tr, const vec3_t& impactvelocity ) = 0;
	virtual void	ProcessImpacts( void ) = 0;

	// Numbered line printf
	virtual void	Con_NPrintf( int idx, const char* fmt, ... ) = 0;

	// These have separate server vs client impementations
	virtual void	StartSound( const vec3_t& origin, int channel, char const* sample, float volume, int soundlevel, int fFlags, int pitch ) = 0;
	virtual void	StartSound( const vec3_t& origin, const char* soundname ) = 0;
	virtual void	PlaybackEventFull( int flags, int clientindex, unsigned short eventindex, float delay, vec3_t& origin, vec3_t& angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 ) = 0;

	// Apply falling damage to m_pHostPlayer based on m_pHostPlayer->m_flFallVelocity.
	virtual bool	PlayerFallingDamage( void ) = 0;

	// Apply falling damage to m_pHostPlayer based on m_pHostPlayer->m_flFallVelocity.
	virtual void	PlayerSetAnimation( int playerAnim ) = 0;

	virtual void* GetSurfaceProps( void ) = 0;
};

class CPrediction
{
public:
	// helper methods.
	template< typename t >
	__forceinline t& get( size_t offset )
	{
		return *( t* )( ( uintptr_t )this + offset );
	}

public:
	// indexes for virtuals and hooks.
	enum indices : size_t
	{
		UPDATE = 3,
		POSTENTITYPACKETRECEIVED = 5,
		POSTNETWORKDATARECEIVED = 6,
		SETLOCALVIEWANGLES = 13,
		INPREDICTION = 14,
		CHECKMOVINGGROUND = 18,
		RUNCOMMAND = 19,
		SETUPMOVE = 20,
		FINISHMOVE = 21
	};

	__forceinline bool& in_prediction( ) {
		return get< bool >( 0x8 );
	}

	__forceinline bool& first_time_predicting( ) {
		return get< bool >( 0x18 );
	}

	__forceinline int& startframe( ) {
		return get< int >( 0xC );
	}

	__forceinline int& servercmds( ) {
		return get< int >( 0x20 );
	}

public:
	// virtual methods
	__forceinline void Update( int startframe, bool validframe, int incoming_acknowledged, int outgoing_command )
	{
		return util::get_method< void( __thiscall* )( void*, int, bool, int, int ) >( this, UPDATE )( this, startframe, validframe, incoming_acknowledged, outgoing_command );
	}

	__forceinline void SetLocalViewAngles( const ang_t& ang )
	{
		return util::get_method< void( __thiscall* )( decltype( this ), const ang_t& ) >( this, SETLOCALVIEWANGLES )( this, ang );
	}

	__forceinline void SetupMove( Entity* player, CUserCmd* cmd, IMoveHelper* helper, CMoveData* data )
	{
		return util::get_method< void( __thiscall* )( decltype( this ), Entity*, CUserCmd*, IMoveHelper*, CMoveData* ) >( this, SETUPMOVE )( this, player, cmd, helper, data );
	}

	__forceinline void FinishMove( Entity* player, CUserCmd* cmd, CMoveData* data )
	{
		return util::get_method< void( __thiscall* )( decltype( this ), Entity*, CUserCmd*, CMoveData* ) >( this, FINISHMOVE )( this, player, cmd, data );
	}

	__forceinline void RunCommand( Entity* player, CUserCmd* cmd, IMoveHelper* helper ) {
		return util::get_method<void( __thiscall* )( decltype( this ), Entity*, CUserCmd*, IMoveHelper* )>( this, RUNCOMMAND )( this, player, cmd, helper );
	}
};

class CGameMovement
{
public:
	// indexes for virtuals and hooks
	enum indices : size_t
	{
		PROCESSMOVEMENT = 1,
		STARTTRACKPREDICTIONERRORS = 3,
		FINISHTRACKPREDICTIONERRORS = 4,
		SETUPMOVEMENTBOUNDS = 12,
		ONLAND = 32
	};

	__forceinline void ProcessMovement( Entity* player, CMoveData* data )
	{
		return util::get_method< void( __thiscall* )( decltype( this ), Entity*, CMoveData* ) >( this, PROCESSMOVEMENT )( this, player, data );
	}

	__forceinline void StartTrackPredictionErrors( Entity* player )
	{
		return util::get_method< void( __thiscall* )( decltype( this ), Entity* ) >( this, STARTTRACKPREDICTIONERRORS )( this, player );
	}

	__forceinline void FinishTrackPredictionErrors( Entity* player )
	{
		return util::get_method< void( __thiscall* )( decltype( this ), Entity* ) >( this, FINISHTRACKPREDICTIONERRORS )( this, player );
	}

	__forceinline void SetupMovementBounds( CMoveData* data ) {
		return util::get_method< void( __thiscall* )( decltype( this ), CMoveData* ) >( this, SETUPMOVEMENTBOUNDS )( this, data );
	}
};