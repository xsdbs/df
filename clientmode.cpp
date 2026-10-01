#include "includes.h"

bool Hooks::ShouldDrawParticles( )
{
	return g_hooks.m_client_mode.GetOldMethod< ShouldDrawParticles_t >( IClientMode::SHOULDDRAWPARTICLES )( this );
}

bool Hooks::ShouldDrawFog( )
{
	// remove fog.
	if ( g_menu.main.visuals.nofog.get( ) )
		return false;

	return g_hooks.m_client_mode.GetOldMethod< ShouldDrawFog_t >( IClientMode::SHOULDDRAWFOG )( this );
}

void Hooks::OverrideView( CViewSetup* view )
{
	// damn son.
	g_cl.m_local = g_csgo.m_entlist->GetClientEntity< Player* >( g_csgo.m_engine->GetLocalPlayer( ) );

	if ( g_menu.main.visuals.fov.get( ) )
	{
		float player_fov = g_menu.main.visuals.fov_amt.get( );

		if ( player_fov < 1.0f )
			player_fov = 1.0f;

		if ( g_cl.m_local && g_cl.m_processing )
		{
			if ( g_cl.m_weapon )
			{
				if ( g_cl.m_local->m_bIsScoped( ) )
				{
					player_fov += ( view->m_fov - player_fov ) * g_menu.main.visuals.fov_scoped_amt.get( ) * 0.01f;
				}
			}
		}

		view->m_fov = player_fov;
	}

	g_visuals.ThirdpersonThink( );

	// call original.
	g_hooks.m_client_mode.GetOldMethod< OverrideView_t >( IClientMode::OVERRIDEVIEW )( this, view );

	if ( g_cl.m_local ) {
		static auto DisablePostProcess = *pattern::find( g_csgo.m_client_dll, XOR( "80 3D ? ? ? ? ? 53 56 57 0F 85" ) ).add( 0x2 ).as<bool**>( );
		if ( g_menu.main.visuals.noscope.get( ) ) {
			if ( *DisablePostProcess == false )
				*DisablePostProcess = true;
		}
		else {
			if ( *DisablePostProcess == true )
				*DisablePostProcess = false;
		}
	}
}

float Hooks::GetViewmodelFOV( )
{
	if ( !g_menu.main.visuals.viewmodel_fov.get( ) )
		return g_hooks.m_client_mode.GetOldMethod< GetViewModelFov_t >( IClientMode::GETVIEWMODELFOV )( this );

	return g_menu.main.visuals.viewmodel_fov_amt.get( );
}

bool Hooks::CreateMove( float time, CUserCmd* cmd )
{
	// called from CInput::ExtraMouseSample -> return original.
	if ( !cmd || !cmd->m_command_number )
		return g_hooks.m_client_mode.GetOldMethod< CreateMove_t >( IClientMode::CREATEMOVE )( this, time, cmd );

	// if we arrived here, called from -> CInput::CreateMove
	// call EngineClient::SetViewAngles according to what the original returns.
	if ( g_hooks.m_client_mode.GetOldMethod< CreateMove_t >( IClientMode::CREATEMOVE )( this, time, cmd ) )
		g_csgo.m_engine->SetViewAngles( cmd->m_view_angles );

	// random_seed isn't generated in ClientMode::CreateMove yet, we must set generate it ourselves.
	cmd->m_random_seed = g_csgo.MD5_PseudoRandom( cmd->m_command_number ) & 0x7fffffff;

	// get bSendPacket off the stack.
	const auto ebp = *reinterpret_cast< std::uintptr_t* >( reinterpret_cast< std::uintptr_t >( _AddressOfReturnAddress( ) ) - sizeof( std::uintptr_t* ) );
	g_cl.m_packet = reinterpret_cast< bool* >( ebp - 0x1c );
	g_cl.m_final_packet = reinterpret_cast< bool* >( ebp - 0x1b ); // the .local function breaks in debug for some reason. idc.

	if ( g_gui.m_open )
	{
		// are we IN_ATTACK?
		if ( cmd->m_buttons & IN_ATTACK )
			cmd->m_buttons &= ~IN_ATTACK;

		// are we IN_ATTACK2?
		if ( cmd->m_buttons & IN_ATTACK2 )
			cmd->m_buttons &= ~IN_ATTACK2;
	}

	// invoke move function.

	g_cl.OnTick( cmd );

	// sendpacket false
	if ( *g_cl.m_packet )
		g_cl.m_sent_cmds.push_back( cmd->m_command_number );

	return false;
}

bool Hooks::DoPostScreenSpaceEffects( CViewSetup* setup )
{
	g_visuals.RenderGlow( );
	return g_hooks.m_client_mode.GetOldMethod< DoPostScreenSpaceEffects_t >( IClientMode::DOPOSTSPACESCREENEFFECTS )( this, setup );
}