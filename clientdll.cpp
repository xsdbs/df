#include "includes.h"

void Hooks::Shutdown( )
{
	g_threading.shutdown( );

	// i think this is needed to pause the main thread so we can make sure others exit in time.
	// we (ideally) wont have queued tasks here, though.
	g_threading.join_all( );

	g_hooks.m_client.GetOldMethod<void( __thiscall* )( void* )>( CHLClient::SHUTDOWN )( this );
}

void Hooks::LevelInitPreEntity( const char* map )
{
	float rate { 1.f / g_csgo.m_globals->m_interval };

	// set rates when joining a server.
	g_csgo.cl_updaterate->SetValue( rate );
	g_csgo.cl_cmdrate->SetValue( rate );

	g_aimbot.reset( );
	g_visuals.m_hit_start = 0.f;
	g_shots.m_pre_hitmarker_impacts.clear( );
	g_shots.m_post_hitmarker_impacts.clear( );

	const auto engine_no_focus_sleep = g_csgo.m_cvar->FindVar( HASH( "engine_no_focus_sleep" ) );
	engine_no_focus_sleep->SetValue( 0 );

	const auto dsp_slow_cpu = g_csgo.m_cvar->FindVar( HASH( "dsp_slow_cpu" ) );
	dsp_slow_cpu->m_flags &= ~FCVAR_CHEAT;
	dsp_slow_cpu->SetValue( 1 );

	const auto dsp_enhance_stereo = g_csgo.m_cvar->FindVar( HASH( "dsp_enhance_stereo" ) );
	dsp_enhance_stereo->m_flags &= ~FCVAR_ARCHIVE;
	dsp_enhance_stereo->SetValue( 0 );

	const auto fps_max = g_csgo.m_cvar->FindVar( HASH( "fps_max" ) );
	if ( fps_max->GetInt( ) == 0 || fps_max->GetInt( ) > 500 ) {

		fps_max->m_flags &= ~FCVAR_ARCHIVE;
		fps_max->SetValue( 500 );
	}

	// invoke original method.
	g_hooks.m_client.GetOldMethod< LevelInitPreEntity_t >( CHLClient::LEVELINITPREENTITY )( this, map );
}

void Hooks::LevelInitPostEntity( )
{
	g_cl.OnMapload( );

	// invoke original method.
	g_hooks.m_client.GetOldMethod< LevelInitPostEntity_t >( CHLClient::LEVELINITPOSTENTITY )( this );
}

void Hooks::LevelShutdown( )
{
	g_aimbot.reset( );

	g_cl.m_local = nullptr;
	g_cl.m_weapon = nullptr;
	g_cl.m_processing = false;
	g_cl.m_weapon_info = nullptr;
	g_cl.m_round_end = false;

	// clear networking shit
	g_cl.m_sequences.clear( );
	g_cl.m_sent_cmds.clear( );

	// invoke original method.
	g_hooks.m_client.GetOldMethod< LevelShutdown_t >( CHLClient::LEVELSHUTDOWN )( this );
}

void DrawServerHitboxes( int player_index ) {
	if ( g_csgo.m_cl->m_net_channel && g_csgo.m_engine->IsInGame( ) && g_csgo.m_cl->m_net_channel->IsLoopback( ) ) {

		// server hitboxes.
		float fDuration = -1.f;

		static auto DrawServerHitboxes = pattern::find( g_csgo.m_server_dll, XOR( "E8 ? ? ? ? F6 83 ? ? ? ? ? 0F 84 ? ? ? ? 33 FF 39 BB" ) ).rel32( 0x1 ).as < void* >( );
		static auto UTIL_PlayerByIndex = pattern::find( g_csgo.m_server_dll, XOR( "85 C9 7E 2A A1" ) ).as < void* ( __fastcall* ) ( int ) >( );

		//for (int i = 1; i <= 32; ++i) {
			//auto e = g_csgo.m_entlist->GetClientEntity(i);
			//
			//if (!e)
			//	continue;

		auto ent = ( Player* )UTIL_PlayerByIndex( player_index );

		if ( !ent )
			return;

		__asm {
			pushad
			movss xmm1, fDuration
			push 1 //bool monoColor
			mov ecx, ent
			call DrawServerHitboxes
			popad
		}
		//}
	}
}

struct var_map_entry_t
{
public:
	unsigned short type;
	unsigned short need_interp;
	void* data;
	void* watcher;
};

struct varmapping_t
{
	varmapping_t( )
	{
		interp_entries = 0;
	}

	var_map_entry_t* entries;
	int interp_entries;
	float last_interp_time;
};

varmapping_t* get_var_map( void* base_ent )
{
	return reinterpret_cast< varmapping_t* >( ( DWORD )base_ent + 0x24 ); //0x4C );
}

void disable_interpolation( Player* ent )
{
	if ( !ent )
		return;

	varmapping_t* map = get_var_map( ent );

	if ( !map )
		return;

	for ( int i = 0; i < map->interp_entries; i++ )
	{
		var_map_entry_t* e = &map->entries[ i ];
		e->need_interp = false;
	}
}

void Hooks::FrameStageNotify( Stage_t stage )
{
	if ( stage != FRAME_START )
		g_cl.m_stage = stage;

	g_cl.m_local = g_csgo.m_entlist->GetClientEntity< Player* >( g_csgo.m_engine->GetLocalPlayer( ) );
	g_cl.m_processing = g_cl.m_local && g_cl.m_local->alive( ) && g_cl.m_local->m_iHealth( ) > 0;

	if ( stage )
	{
		if ( stage == FRAME_RENDER_START )
			g_visuals.DrawBeams( );
	}
	else {
		if ( g_cl.m_local && g_csgo.m_engine && g_csgo.m_engine->IsInGame( ) ) {
			if ( g_csgo.m_net && !g_csgo.m_net->IsLoopback( ) )
			{
				int client_tick = g_csgo.m_cl->m_client_tick;
				g_csgo.NET_RunFrame( g_csgo.Plat_FloatTime( ) );
				g_csgo.CL_ReadPackets( true );
				g_csgo.m_cl->m_client_tick = client_tick;
			}
		}
	}

	g_hooks.m_client.GetOldMethod< FrameStageNotify_t >( CHLClient::FRAMESTAGENOTIFY )( this, stage );

	g_cl.m_local = g_csgo.m_entlist->GetClientEntity< Player* >( g_csgo.m_engine->GetLocalPlayer( ) );
	g_cl.m_processing = g_cl.m_local && g_cl.m_local->alive( ) && g_cl.m_local->m_iHealth( ) > 0;
	switch ( stage )
	{
	case FRAME_RENDER_START:
		break;

	case FRAME_NET_UPDATE_POSTDATAUPDATE_START:
		if ( g_csgo.sv_cheats->GetInt( ) != 1 )
			g_csgo.sv_cheats->SetValue( 1 );
		break;

	case FRAME_NET_UPDATE_POSTDATAUPDATE_END:
		g_visuals.NoSmoke( );
		g_visuals.ExtendedDormancy( );
		break;
	default:
		break;
	}
}