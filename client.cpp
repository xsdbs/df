#include "includes.h"

Client g_cl { };

// loader will set this fucker.
#define FRAMERATE_AVG_FRAC 0.9

// init routine.
ulong_t __stdcall Client::init( void* arg )
{
	// if not in interwebz mode, the driver will not set the username.
	g_cl.m_user = XOR( "supremacy" );

	//AllocConsole();
	//freopen_s( (FILE**)stdout, "CONOUT$", "w", stdout );
	//freopen_s( (FILE**)stderr, "CONOUT$", "w", stderr );
	//freopen_s( (FILE**)stdin, "CONIN$", "r", stdin );
	//SetConsoleTitleA( "Bro test Debug" );

	//int Bro = MessageBox( NULL, XOR( "are desync files present????? sir This is Crucial" ), XOR( "" ), MB_YESNO | MB_ICONQUESTION );
	//
	//if ( Bro == 6 )
	//	g_skins.m_desync = true;

	// stop here if we failed to acquire all the data needed from csgo.
	if ( !g_csgo.init( ) )
		return 0;

	g_cl.UnlockHiddenConvars( );

	// welcome the user.
	g_notify.add( tfm::format( XOR( "welcome to %s\n" ), g_cl.m_user ) );

	return 1;
}

void Client::BuyBot( ) {
	if ( !m_local )
		return;

	if ( !g_menu.main.misc.autobuy.get( ) )
		return;

	static auto bSkipFirst = true;
	if ( bSkipFirst ) {
		bSkipFirst = false;
		return;
	}

	static auto flSpawnTime = 0.0f;
	if ( flSpawnTime == m_local->m_flSpawnTime( ) ) {
		bSkipFirst = true;
		return;
	}

	flSpawnTime = m_local->m_flSpawnTime( );

	if ( g_menu.main.misc.buy3.get( 3 ) )
	{
		if ( g_csgo.m_gamerules->m_totalRoundsPlayed( ) == 0 || g_csgo.m_gamerules->m_totalRoundsPlayed( ) == g_csgo.mp_maxrounds->GetInt( ) / 2 )
			return;
	}

	int team = m_local->m_iTeamNum( );

	switch ( g_menu.main.misc.buy1.get( ) ) {
	case 0:
		g_csgo.m_engine->ExecuteClientCmd( "buy awp" ); break;
	case 1:
		if ( team == TEAM_COUNTERTERRORISTS )
			g_csgo.m_engine->ExecuteClientCmd( "buy scar20" );
		else if ( team == TEAM_TERRORISTS )
			g_csgo.m_engine->ExecuteClientCmd( "buy g3sg1" );

		break;
	case 2:
		g_csgo.m_engine->ExecuteClientCmd( "buy ssg08" ); break;
	default:
		break;
	}

	switch ( g_menu.main.misc.buy2.get( ) ) {
	case 1:
		g_csgo.m_engine->ExecuteClientCmd( "buy deagle" ); break;
	default:
		break;
	}

	if ( g_menu.main.misc.buy3.get( 1 ) ) {
		g_csgo.m_engine->ExecuteClientCmd( "buy vest" );
		g_csgo.m_engine->ExecuteClientCmd( "buy vesthelm" );
	}

	if ( g_menu.main.misc.buy3.get( 0 ) ) {
		g_csgo.m_engine->ExecuteClientCmd( "buy molotov" );
		g_csgo.m_engine->ExecuteClientCmd( "buy incgrenade" );
		g_csgo.m_engine->ExecuteClientCmd( "buy hegrenade" );
		g_csgo.m_engine->ExecuteClientCmd( "buy smokegrenade" );
	}

	if ( g_menu.main.misc.buy3.get( 2 ) ) {
		g_csgo.m_engine->ExecuteClientCmd( "buy taser" );
	}

	g_csgo.m_engine->ExecuteClientCmd( "buy defuser" ); // whatever
}

void Client::DrawHUD( ) {
	if ( !g_csgo.m_engine->IsInGame( ) )
		return;

	if ( !g_menu.main.config.watermark.get( ) )
		return;

	// get round trip time in milliseconds.
	int ms = std::max( 0, ( int )std::round( m_latency * 1000.f ) );

	std::string text = tfm::format( XOR( "supremacy | rtt: %ims" ), ms );
	render::FontSize_t size = render::hud.size( text );

	// background.
	render::rect_filled( m_width - size.m_width - 20, 10, size.m_width + 10, size.m_height + 2, { 240, 110, 140, 130 } );

	// text.
	render::hud.string( m_width - 15, 10, { 240, 160, 180, 250 }, text, render::ALIGN_RIGHT );
}

void Client::UnlockHiddenConvars( )
{
	if ( !g_csgo.m_cvar )
		return;

	auto p = **reinterpret_cast< ConVar*** >( g_csgo.m_cvar + 0x34 );
	for ( auto c = p->m_next; c != nullptr; c = c->m_next )
	{
		c->m_flags &= ~FCVAR_DEVELOPMENTONLY;
		c->m_flags &= ~FCVAR_HIDDEN;
	}
}

void Client::KillFeed( )
{
	if ( !g_menu.main.misc.preserve_killfeed.get( ) )
		return;

	if ( !g_csgo.m_engine->IsInGame( ) )
		return;

	// get the addr of the killfeed.
	KillFeed_t* feed = ( KillFeed_t* )g_csgo.m_hud->FindElement( HASH( "SFHudDeathNoticeAndBotStatus" ) );
	if ( !feed )
		return;

	int size = feed->notices.Count( );
	if ( !size )
		return;

	for ( int i { }; i < size; ++i )
	{
		NoticeText_t* notice = &feed->notices[ i ];

		// this is a local player kill, delay it.
		if ( notice->fade == 1.5f )
			notice->fade = FLT_MAX;
	}
}

void Client::OnPaint( )
{
	// update screen size.
	g_csgo.m_engine->GetScreenSize( m_width, m_height );

	g_hvh.m_should_listen = !g_csgo.m_client->IsChatRaised( ) && !g_csgo.m_engine->Con_IsVisible( );

	// render stuff.
	g_visuals.think( );
	g_grenades.paint( );
	g_notify.think( );

	DrawHUD( );

	KillFeed( );

	// menu goes last.
	g_gui.think( );
}

void Client::OnMapload( )
{
	// store class ids.
	g_netvars.SetupClassData( );

	// createmove will not have been invoked yet.
	// but at this stage entites have been created.
	// so now we can retrive the pointer to the local player.
	m_local = g_csgo.m_entlist->GetClientEntity< Player* >( g_csgo.m_engine->GetLocalPlayer( ) );

	// FIX THIS SHIT LOL.
	// 
	//callbacks::SetPostProcess();
	// i can add stuff for dis ting dw
	// 
	// world materials.
	m_change_world = true;

	/* clear the data */
	m_smooth_data = { };

	g_visuals.ModulateWorld( );

	// init knife shit.
	g_skins.load( );

	// clear networking shit
	m_sequences.clear( );
	m_sent_cmds.clear( );

	//g_csgo.m_engine->ExecuteClientCmd( g_Variables.m_strings[ "autoexec" ].c_str( ) );

	for ( int i = 1; i <= g_csgo.m_entlist->GetHighestEntityIndex( ); i++ ) {
		Entity* ent = g_csgo.m_entlist->GetClientEntity( i );
		if ( !ent )
			continue;

		if ( ent->GetClientClass( )->m_ClassID == 36 )
			if ( reinterpret_cast< CPlayerResource* >( ent ) != player_resource )
				player_resource = reinterpret_cast< CPlayerResource* >( ent );
	}

	// handle maxunlags
	{
		static std::vector<std::pair<const char*, float>> servers;
		servers.push_back( { "185.29.166.133:27015", 0.4f } ); // nexus
		servers.push_back( { "74.91.113.127:27015", 0.5f } ); // gametime
		servers.push_back( { "74.91.113.151:27015", 0.5f } ); // gametime
		servers.push_back( { "45.45.237.162:27015", 0.5f } ); // dick's

		const char* sv = g_csgo.m_cl->m_net_channel->GetAddress( );
		for ( auto& curr : servers )
		{
			if ( strcmp( sv, curr.first ) == 0 ) // we've found a server, strcmp returns 1 if its not what we want for whatever reason
			{
				g_csgo.sv_maxunlag->SetValue( curr.second );
				g_notify.add( tfm::format( "max ping spike is %.fms\n", curr.second * 1000 ) );
				break;
			}
		}

		servers.clear( );
	}

	// allocate bone matrixes
	{
		CREATE_IF_NULL( g_chams.local_hidden, ( BoneArray* )g_csgo.m_mem_alloc->Alloc( sizeof( BoneArray ) * 128 ) );
		CREATE_IF_NULL( g_server_animations.local_matrix, ( BoneArray* )g_csgo.m_mem_alloc->Alloc( sizeof( BoneArray ) * 128 ) );
		CREATE_IF_NULL( g_server_animations.local_render_matrix, ( BoneArray* )g_csgo.m_mem_alloc->Alloc( sizeof( BoneArray ) * 128 ) );
	}

	// if the INetChannelInfo pointer has changed, store it for later.
	g_csgo.m_net = g_csgo.m_engine->GetNetChannelInfo( );

	if ( g_csgo.m_net )
	{
		g_hooks.m_net_channel.reset( );
		g_hooks.m_net_channel.init( g_csgo.m_net );
		g_hooks.m_net_channel.add( INetChannel::PROCESSPACKET, util::force_cast( &Hooks::ProcessPacket ) );
		g_hooks.m_net_channel.add( INetChannel::SENDDATAGRAM, util::force_cast( &Hooks::SendDatagram ) );
		g_hooks.m_net_channel.add( INetChannel::SENDNETMSG, util::force_cast( &Hooks::SendNetMsg ) );
	}
}

void Client::HandleFakeLatency( ) {
	if ( g_csgo.m_globals->m_tick_count % 2 != 0 )
		return;

	if ( g_aimbot.m_fake_latency ) {
		if ( m_fakelatency_amt > g_menu.main.misc.alt_fake_latency_amt.get( ) ) {
			m_fakelatency_amt = g_menu.main.misc.alt_fake_latency_amt.get( );
			m_fakelatency_amt = std::clamp( m_fakelatency_amt, 0.f, g_csgo.sv_maxunlag->GetFloat( ) * 1000.f );
			return;
		}

		if ( m_fakelatency_amt < std::min( g_csgo.sv_maxunlag->GetFloat( ) * 1000.f, g_menu.main.misc.alt_fake_latency_amt.get( ) ) )
			m_fakelatency_amt += 20.f;
	}
	else
		m_fakelatency_amt = g_menu.main.misc.fake_latency_amt.get( );
}

bool Client::InstantActions( bool weapon_switch, int new_buttons ) {
	bool shouldnt_run = g_cl.m_shot 
		|| !g_menu.main.aimbot.automatic_stop.get( 0 ) 
		|| !g_menu.main.aimbot.automatic_stop.get( 3 );

	if ( shouldnt_run )
		return false;

	int curr_lag = g_cl.m_lag;

	if ( curr_lag < 0 )
		return false;

	int start_tick = g_cl.m_cmd->m_command_number;
	int end_tick = 0;
	int command_number = 0;

	int iterated_lag = 0;

	for ( end_tick = start_tick - curr_lag;; ++end_tick ) {
		command_number = end_tick % 150;

		if ( end_tick != start_tick )
			break;

	iterate_check:
		if ( ++iterated_lag > curr_lag )
			return false;
	}

	// validity check
	if ( ( ( new_buttons & IN_ATTACK2 ) == 0 )
		&& ( ( new_buttons & IN_RELOAD ) == 0 )
		&& !weapon_switch ) {
		goto iterate_check;
	}

	int remainder = abs( start_tick - end_tick );

	CUserCmd* cmd = &g_csgo.m_input->m_commands[ command_number ];
	CVerifiedUserCmd* verified_cmd = &g_csgo.m_input->m_verified_commands[ command_number ];

	cmd->m_buttons |= new_buttons;
	verified_cmd->m_cmd.m_buttons |= new_buttons;

	if ( weapon_switch ) {
		int weapon_select = g_cl.m_cmd->m_weapon_select;
		int weapon_subtype = g_cl.m_cmd->m_weapon_subtype;

		cmd->m_weapon_select = weapon_select;
		cmd->m_weapon_subtype = weapon_subtype;

		verified_cmd->m_cmd.m_weapon_select = weapon_select;
		verified_cmd->m_cmd.m_weapon_subtype = weapon_subtype;
	}

	if ( remainder <= 0 )
		return false;

	g_csgo.m_prediction->servercmds( ) = INT_MAX;

	verified_cmd->m_cmd.m_predicted = false;
	cmd->m_predicted = false;

	static auto cl_showerror = g_csgo.m_cvar->FindVar( HASH( "cl_showerror" ) );
	if ( cl_showerror->GetInt( ) > 0 ) {
		g_cl.print( tfm::format( "instant actions reclaimed %d\n", curr_lag ) );
	}

	g_inputpred.update( );
	return true;
}

void Client::StartMove( CUserCmd* cmd )
{
	m_cmd = cmd;
	m_tick = cmd->m_tick;
	m_view_angles = cmd->m_view_angles;
	m_buttons = cmd->m_buttons;
	m_pressing_move = ( m_buttons & ( IN_LEFT ) || m_buttons & ( IN_FORWARD ) || m_buttons & ( IN_BACK ) ||
		m_buttons & ( IN_RIGHT ) || m_buttons & ( IN_MOVELEFT ) || m_buttons & ( IN_MOVERIGHT ) ||
		m_buttons & ( IN_JUMP ) );

	m_local = g_csgo.m_entlist->GetClientEntity< Player* >( g_csgo.m_engine->GetLocalPlayer( ) );
	if ( !m_local )
		return;

	m_hide_angle = 0;

	m_max_lag = g_csgo.sv_maxusrcmdprocessticks->GetInt( ) - 1;

	m_lag = g_csgo.m_cl->m_choked_commands;
	m_lerp = game::GetClientInterpAmount( );
	m_latency = g_csgo.m_net->GetLatency( INetChannel::FLOW_OUTGOING );
	m_in_latency = g_csgo.m_net->GetLatency( INetChannel::FLOW_INCOMING );

	m_tickrate = int( 1.f / g_csgo.m_globals->m_interval );
	m_framerate = FRAMERATE_AVG_FRAC * m_framerate + ( 1.f - FRAMERATE_AVG_FRAC ) * g_csgo.m_globals->m_abs_frametime;
	if ( m_framerate <= 0.0f )
		m_framerate = 1.0f;

	m_landed_on_anim_update = false;

	// exponential moving averages
	m_avg_frametime = 0.9f * m_avg_frametime + 0.1f * g_csgo.m_globals->m_abs_frametime;
	m_avg_fps = 1.0f / m_avg_frametime;

	// instant values
	float cur_frametime = g_csgo.m_globals->m_abs_frametime;
	float cur_fps = 1.0f / cur_frametime;

	// check if current FPS is way lower than smoothed average
	m_throttled = cur_fps < m_avg_fps * 0.5f || cur_fps <= m_tickrate;

	// lelelele bro
	//waif (g_Variables.m_ints["buybot_primary"] == 1)
	//wa	BuyAwpBeforeKamiDoes(cmd->m_tick);

	HandleFakeLatency( );
	HandleKillsays( );

	m_processing = m_local && m_local->alive( ) && m_local->m_iHealth( ) > 0;
	if ( !m_processing )
	{
		return;
	}

	// store our prediction data.
	g_inputpred.store( );

	int new_buttons = 0;

	// fast scope
	if ( ( m_cmd->m_buttons & IN_ATTACK2 ) != 0 && ( m_old_buttons & IN_ATTACK2 ) == 0 )
	{
		new_buttons = IN_ATTACK2;

		Weapon* weapon = m_local->GetActiveWeapon( );
		if ( weapon )
		{
			WeaponInfo* weapon_info = weapon->GetWpnData( );
			if ( weapon_info ) {
				int weapon_type = weapon_info->m_weapon_type;
				if ( weapon_type == WEAPONTYPE_KNIFE || weapon_type == WEAPONTYPE_GRENADE )
					new_buttons = 0;
			}
		}
	}
	
	// fast reload
	if ( ( m_cmd->m_buttons & IN_RELOAD ) != 0 && ( m_old_buttons & IN_RELOAD ) == 0 )
		new_buttons |= IN_RELOAD;

	// fast weapon switch
	bool weapon_switch = false;
	if ( m_cmd->m_weapon_select > 0 )
		weapon_switch = m_cmd->m_weapon_select != m_weapon_select;

	if ( !new_buttons && !weapon_switch || !InstantActions( weapon_switch, new_buttons ) )
		g_inputpred.update( );

	g_inputpred.GetDatamap( g_inputpred.cm_start );

	m_weapon_select = cmd->m_weapon_select;
	m_flags = m_local->m_fFlags( );
	m_shot = false;
}

void Client::BackupPlayers( bool restore )
{
	// we want to check localplayer here in order to SuperSetupBones.
	// restore stuff.
	for ( int i : m_sorted_players )
	{
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >( i );

		if ( restore )
			g_aimbot.m_backup[ i - 1 ].restore( player );
		else
			g_aimbot.m_backup[ i - 1 ].store( player );
	}
}

void Client::DoMove( ) {
	// backup strafe angles (we need them for input prediction)
	m_strafe_angles = m_cmd->m_view_angles;

	// run movement code before input prediction
	g_movement.JumpRelated( );
	g_movement.AutoStrafe( );
	g_movement.AutoPeek( );
	g_movement.FakeWalk( );
	g_movement.ProcessAutoStop( m_cmd );

	// transition flick for anti-aim
	g_movement.m_transition_flick = false;
	if ( g_menu.main.antiaim.twist_move.get( 1 )
		&& !g_input_system.GetKeyState( g_menu.main.movement.fakewalk.get( ) )
		&& g_hvh.m_manual_direction == ManualAAMode::NONE ) {

		if ( m_pressing_move && g_movement.m_transition_reset ) {
			m_cmd->m_forward_move = m_cmd->m_side_move = 0.0f;
			g_movement.m_transition_flick = true;

			if ( !g_movement.m_transition_stage.first ) {
				m_cmd->m_forward_move = ( m_cmd->m_tick % 2 == 0 ) ? -10.f : 10.f;
			}
		}
		else {
			g_movement.m_transition_stage.first = false;
			g_movement.m_transition_stage.second = false;
		}
	}

	// predict 1 tick ahead
	g_inputpred.predict( );
	g_inputpred.GetDatamap( g_inputpred.og_predicted );

	// edge jump
	if ( g_input_system.GetKeyState( g_menu.main.movement.edge_jump.get( ) ) ) {
		g_movement.EdgeJump( );
	}

	// cmd gonna get animated
	if ( !m_lag && g_csgo.m_cl->m_delta_tick > -1 ) {
		m_pred_aim_matrix_width_range = g_server_animations.GetAimMatrixWidthRange( );
	}

	// restore original angles after input prediction
	m_cmd->m_view_angles = m_view_angles;

	// convert viewangles to directional forward vector
	math::AngleVectors( m_view_angles, &m_forward_dir );

	// store values after input prediction
	m_shoot_pos = m_local->GetShootPosition( );

	// reset weapon data
	m_weapon = nullptr;
	m_weapon_info = nullptr;
	m_weapon_id = -1;
	m_weapon_type = WEAPONTYPE_UNKNOWN;
	m_weapon_fire = false;
	m_can_reclaim = false;

	// fetch weapon info
	m_weapon = m_local->GetActiveWeapon( );
	if ( m_weapon ) {
		m_weapon_info = m_weapon->GetWpnData( );
		m_weapon_id = m_weapon->m_iItemDefinitionIndex( );
		m_weapon_type = m_weapon_info->m_weapon_type;

		// update spread values if not a grenade
		if ( m_weapon_type != WEAPONTYPE_GRENADE )
			m_weapon->UpdateAccuracyPenalty( );

		penetration::PenetrationOutput_t tmp_pen_data{ };

		// penetration crosshair logic
		if ( !m_forward_dir.is_zero( )
			&& m_weapon_type != WEAPONTYPE_KNIFE
			&& m_weapon_type != WEAPONTYPE_C4
			&& m_weapon_type != WEAPONTYPE_GRENADE ) {

			penetration::PenetrationInput_t in;
			in.m_from = m_local;
			in.m_target = nullptr;
			in.m_pos = m_shoot_pos + ( m_forward_dir * m_weapon_info->m_range );
			in.m_damage = 1.f;
			in.m_can_pen = true;

			// run autowall.
			penetration::run( &in, &tmp_pen_data );
		}

		// set pen data for penetration crosshair.
		m_pen_data = tmp_pen_data;

		m_player_fire = g_csgo.m_globals->m_curtime >= m_local->m_flNextAttack( ) && !g_csgo.m_gamerules->m_bFreezePeriod( ) && !( m_flags & FL_FROZEN );

		m_can_reclaim = m_weapon_id == REVOLVER && m_weapon->m_Activity( ) == ACT_VM_HAULBACK && m_weapon->m_iClip1( ) > 0;

		AutoRevolver( );
		m_weapon_fire = CanFireWeapon( );
	}

	//if ( ( m_buttons & IN_ATTACK ) && m_weapon_type != WEAPONTYPE_GRENADE )
	//{
	//	if ( !m_weapon_fire ) {
	//		m_buttons &= ~IN_ATTACK;
	//		m_cmd->m_buttons &= ~IN_ATTACK;
	//	}
	//}

	// last tick defuse
	// todo - dex: figure out CPlantedC4::Use() range, add indicator if valid
	if ( g_input_system.GetKeyState( g_menu.main.misc.last_tick_defuse.get( ) ) && g_visuals.m_c4_planted ) {
		float defuse_time = ( m_local->m_bHasDefuser( ) ) ? 5.f : 10.f;
		float remaining = g_visuals.m_planted_c4_explode_time - g_csgo.m_globals->m_curtime;
		float dt = remaining - defuse_time - ( m_latency / 2.f );

		m_cmd->m_buttons &= ~IN_USE;
		if ( dt <= game::TICKS_TO_TIME( 2 ) )
			m_cmd->m_buttons |= IN_USE;
	}

	BuyBot( );

	// run grenade prediction
	g_grenades.think( );

	g_resolver.process_override( );

	// clear debug overlay
	g_aimbot.m_debug_overlay.clear( );

	// handle anti-aim, sendpacket, and aimbot
	g_hvh.SendPacket( );
	g_aimbot.think( );

	g_hvh.AntiAim( );
}

void Client::UpdateAnimRecord( CUserCmd* cmd ) {
	if ( g_csgo.m_cl->m_delta_tick == -1 ) {
		m_update_records.clear( );
		return;
	}

	auto PushUpdate = [ & ]( ) {
		const int last_update = m_update_records.empty( ) ? 0 : m_update_records.front( ).m_cmd_num;

		m_update_records.push_front( {
			cmd->m_command_number,
			m_hide_angle,
			g_server_animations.m_viewmodel_act,
			} );

		g_server_animations.m_viewmodel_act = 0;
		};

	// reset after respawn
	if ( m_last_spawn_time != m_local->m_flSpawnTime( ) ) {
		m_update_records.clear( );
		if ( auto state = m_local->m_PlayerAnimState( ) ) {
			game::ResetAnimationState( state );

			for ( int i = 0; i < 24; i++ ) {
				if ( i < 13 )
					m_local->m_AnimOverlay( )[ i ] = {};

				m_local->m_flPoseParameter( )[ i ] = 0.f;
			}

			PushUpdate( );
		}
		m_last_spawn_time = m_local->m_flSpawnTime( );
		return;
	}

	if ( !m_lag ) {
		if ( g_hvh.m_mode == WALK )
			g_hvh.m_lastmove_yaw = cmd->m_view_angles.y;

		if ( !m_hide_angle )
			g_hvh.m_real_angles = cmd->m_view_angles;

		PushUpdate( );
	}

	// clean up old records
	while ( m_update_records.size( ) > 1 && m_update_records.back( ).m_cmd_num < g_csgo.m_cl->m_last_command_ack ) {
		m_update_records.pop_back( );
	}
}

void Client::EndMove( CUserCmd* cmd )
{
	if ( !*m_packet && g_csgo.m_cl->m_choked_commands == 0 )
		last_choked_angles = cmd->m_view_angles;
	else if ( *m_packet && g_csgo.m_cl->m_choked_commands > 0 )
		m_sent_angles = last_choked_angles;
	else if ( *m_packet && g_csgo.m_cl->m_choked_commands == 0 )
		m_sent_angles = cmd->m_view_angles;

	UpdateAnimRecord( cmd );

	// we shouldnt need to do pitch clamp
	// since we never set it outside 89 degree range
	// we aint playin with VAC anyways.
	math::NormalizeAngle( cmd->m_view_angles.y );

	// fix our movement.
	g_movement.FixMove( cmd, m_strafe_angles );

	// this packet will be sent.
	if ( *m_packet )
	{
		// we are sending a packet, so this will be reset soon.
		// store the old value.
		m_old_lag = m_lag;

		// get current origin.
		vec3_t cur = m_local->m_vecOrigin( );

		// get prevoius origin.
		vec3_t prev = m_net_pos.empty( ) ? cur : m_net_pos.front( ).m_pos;

		// check if we broke lagcomp.
		m_lagcomp = ( cur - prev ).length_sqr( ) > 4096.f;

		// save sent origin and time.
		m_net_pos.emplace_front( g_csgo.m_globals->m_curtime, cur );

		//m_sent_angles = cmd->m_view_angles;

		if ( m_local->m_fFlags( ) & FL_ONGROUND && m_cmd->m_buttons & IN_JUMP )
			m_landed_on_anim_update = true;
	}

	static auto cl_showerror = g_csgo.m_cvar->FindVar( HASH( "cl_showerror" ) );

	if ( m_lag && m_weapon_id == REVOLVER && m_can_reclaim ) {
		int iterated_lag = 0;
		int start_tick = m_cmd->m_command_number - m_lag;

		int user_cmd_buttons = 0;
		CUserCmd* user_cmd = nullptr;
		CVerifiedUserCmd* verified_cmd = nullptr;
		while ( true ) {
			int command_number = ( iterated_lag + start_tick ) % 150;
			user_cmd = &g_csgo.m_input->m_commands[ command_number ];
			verified_cmd = &g_csgo.m_input->m_verified_commands[ command_number ];
			user_cmd_buttons = user_cmd->m_buttons;

			if ( user_cmd_buttons & IN_ATTACK ) {
				break;
			}

			if ( ++iterated_lag >= m_lag )
			{
				break;
			}
		}
	
		if ( iterated_lag >= m_lag )
		{
			if ( cl_showerror->GetInt( ) > 3 ) {
				print( tfm::format( "couldn't reclaim any ticks for lag %d\n", m_lag ) );
			}
		}
		else {
			user_cmd->m_buttons &= ~IN_ATTACK;
			verified_cmd->m_cmd.m_buttons &= ~IN_ATTACK;
			m_cmd->m_buttons |= IN_ATTACK;

			if ( cl_showerror->GetInt( ) > 2 )
				print( tfm::format( "RECLAIMED %d ticks/%d\n", m_lag - iterated_lag, m_lag ) );

			g_csgo.m_prediction->servercmds( ) = INT_MAX;
			g_inputpred.update( );	
		}
	}

	m_old_packet = *m_packet;
	m_old_shot = m_shot;
	m_old_buttons = m_cmd->m_buttons;
}

void Client::OnTick( CUserCmd* cmd )
{
	// store some data and update prediction.
	StartMove( cmd );

	// not much more to do here.
	if ( !m_processing )
		return;

	g_aimbot.SetupTargets( );

	BackupPlayers( false );

	// run all movement related code.
	DoMove( );

	// store stome additonal stuff for next tick
	// sanetize our usercommand if needed and fix our movement.
	EndMove( cmd );

	// restore the players.
	BackupPlayers( true );

	// restore curtime/frametime
	// and prediction seed/player.
	g_inputpred.restore( );
	g_inputpred.SetDatamap( g_inputpred.cm_start );

	if ( g_menu.main.visuals.proj.get( ) )
		g_trajectories.OnTick( cmd );
}

void Client::print( Color text_color, const std::string text, ... )
{
	va_list     list;
	int         size;
	std::string buf;

	if ( text.empty( ) )
		return;

	va_start( list, text );

	// count needed size.
	size = std::vsnprintf( 0, 0, text.c_str( ), list );

	// allocate.
	buf.resize( size );

	// print to buffer.
	std::vsnprintf( buf.data( ), size + 1, text.c_str( ), list );

	va_end( list );

	// print to console.
	g_csgo.m_cvar->ConsoleColorPrintf( g_menu.main.config.menu_color.get( ), XOR( "[supremacy] " ) );
	g_csgo.m_cvar->ConsoleColorPrintf( text_color, buf.c_str( ) );
}

void Client::print( const std::string text, ... )
{
	print( colors::white, text );
}

bool Client::CanFireWeapon( ) {
	// the player cant fire.
	if ( !m_player_fire )
		return false;

	if ( m_weapon_type == WEAPONTYPE_GRENADE )
		return false;

	// if we have no bullets, we cant shoot.
	if ( m_weapon_type != WEAPONTYPE_KNIFE && m_weapon->m_iClip1( ) < 1 )
		return false;

	// do we have any burst shots to handle?
	if ( ( m_weapon_id == GLOCK || m_weapon_id == FAMAS ) && m_weapon->m_iBurstShotsRemaining( ) > 0 ) {
		// new burst shot is coming out.
		if ( g_csgo.m_globals->m_curtime >= m_weapon->m_fNextBurstShot( ) )
			return true;
	}

	// r8 revolver.
	if ( m_weapon_id == REVOLVER ) {
		int act = m_weapon->m_Activity( );

		// mouse1.
		if ( !m_revolver_fire ) {
			if ( ( act == 185 || act == 193 ) && m_revolver_cock == 0 )
				return g_csgo.m_globals->m_curtime >= m_weapon->m_flNextPrimaryAttack( );

			return false;
		}
	}

	// yeez we have a normal gun.
	if ( g_csgo.m_globals->m_curtime >= m_weapon->m_flNextPrimaryAttack( ) )
		return true;

	return false;
}

void Client::AutoRevolver( ) {
	// default to false.
	m_revolver_fire = false;

	// reset properly.
	if ( m_revolver_cock == -1 )
		m_revolver_cock = 0;

	// we dont have a revolver.
	// we have no ammo.
	// player cant fire
	// we are waiting for we can shoot again.
	if ( m_weapon_id != REVOLVER || m_weapon->m_iClip1( ) < 1 || !m_player_fire || g_csgo.m_globals->m_curtime < m_weapon->m_flNextPrimaryAttack( ) ) {
		// reset.
		m_revolver_cock = 0;
		m_revolver_query = 0;
		return;
	}

	// calculate max number of cocked ticks.
	// round to 6th decimal place for custom tickrates..
	int shoot = ( int )( 0.25f / ( std::round( g_csgo.m_globals->m_interval * 1000000.f ) / 1000000.f ) );

	// amount of ticks that we have to query.
	m_revolver_query = shoot - 1;

	// we held all the ticks we needed to hold.
	if ( m_revolver_query == m_revolver_cock ) {
		// reset cocked ticks.
		m_revolver_cock = -1;

		// we are allowed to fire, yay.
		m_revolver_fire = true;
	}

	else {
		// we still have ticks to query.
		// apply inattack.
		if ( m_revolver_query > m_revolver_cock )
			m_cmd->m_buttons |= IN_ATTACK;

		// count cock ticks.
		// do this so we can also count 'legit' ticks
		// that didnt originate from the hack.
		if ( m_cmd->m_buttons & IN_ATTACK )
			m_revolver_cock++;

		// inattack was not held, reset.
		else m_revolver_cock = 0;
	}

	// remove inattack2 if cocking.
	if ( m_revolver_cock > 0 )
		m_cmd->m_buttons &= ~IN_ATTACK2;
}

void Client::UpdateIncomingSequences( )
{
	if ( !g_csgo.m_net )
		return;

	if ( m_sequences.empty( ) || g_csgo.m_net->m_in_seq > m_sequences.front( ).m_seq )
	{
		// store new stuff.
		m_sequences.emplace_front( g_csgo.m_globals->m_realtime, g_csgo.m_net->m_in_rel_state, g_csgo.m_net->m_in_seq );
	}

	// do not save too many of these.
	while ( m_sequences.size( ) > 2048 )
		m_sequences.pop_back( );
}

void Client::BuyAwpBeforeKamiDoes( int cur_cmd )
{
	//auto end_delay = g_csgo.m_gamerules->get_gamerules();
	static ConVar* mp_round_restart_delay = g_csgo.m_cvar->FindVar( HASH( "mp_round_restart_delay" ) );

	float end_time = m_round_end_time + mp_round_restart_delay->GetFloat( );

	// just add total rtt, since we also receive the event delayed too.
	float time_remaining = g_csgo.m_globals->m_curtime - ( end_time + m_latency + ( g_csgo.m_cl->m_net_channel->GetLatency( INetChannel::FLOW_INCOMING ) ) );

	// dont really need to spam this one million times, should effectively be the same anyways lol.
	constexpr int max_usrcmds = 4;

	static int last_cmd;

	if ( time_remaining < ( max_usrcmds * g_csgo.m_globals->m_interval )
		&& cur_cmd != last_cmd )
	{
		g_csgo.m_engine->ExecuteClientCmd( XOR( "buy awp" ) );
		last_cmd = cur_cmd;
	}
}

const char* bro[ ] = {
	"say Hit bro In Baited Shootpos",
	"say bro be breaking last move + delaying timer and thinks hes cool",
	"say apache(afonso pereira) molested his lil bro but he didnt care and he even tore his anus apart",
	"say I CHOPPED OFF MY BROTHERS DICK AND MADE SOUP @tritium007_",
	"say Achievements disabled : cheats turned on in this app session.",
	"say name got shot in my diabetie leg so i started selling cp for bio leg @tritium007_",
	"say tearing my bros anal dm @tritium007_ i will poke his eyes off for srcs for a dead game",
	"say frotting with my lil bro dm for vid @tritium007_",
	"say i stalked lily's(14yo) insta just to delete it my name is afonso pereira im a creep @tritium007_",
	"say rip afonso junior, i tore his ass apart @tritium007_",
	"say bro is on supremacy flavour mango",
	"say nigga im a backtrack genius"
};

void Client::HandleKillsays( )
{
	if ( !g_menu.main.config.cp_gooka_killsay.get( ) )
		return;

	if ( m_last_kill_time < g_csgo.m_globals->m_curtime && m_last_kill_time > -100.f )
	{
		int rand = g_csgo.RandomInt( 0, _ARRAYSIZE( bro ) - 1 );
		g_csgo.m_engine->ExecuteClientCmd( bro[ rand ] );
		m_last_kill_time = -100.f; // idk just reset this bitty
	}
}