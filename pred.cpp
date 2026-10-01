#include "includes.h"
#include "pred.h"

InputPrediction g_inputpred {};

void InputPrediction::GetDatamap( datamap_t*& map )
{
	if ( !map )
	{
		g_cl.m_local->PrepareDatamap( g_cl.m_local->GetPredDescMap( ) );
		map = ( datamap_t* )g_csgo.m_mem_alloc->Alloc( std::max( g_cl.m_local->GetPredDescMap( )->m_nPackedSize, 4 ) );
	}

	CPredictionCopy CopyHelper( 2, ( byte* )map, true, ( const byte* )g_cl.m_local, false, CPredictionCopy::TRANSFERDATA_COPYONLY );
	CopyHelper.TransferData( "InputPrediction::GetDatamap", g_cl.m_local->index( ), g_cl.m_local->GetPredDescMap( ) );
}

void InputPrediction::SetDatamap( datamap_t*& map )
{
	CPredictionCopy CopyHelper( 2, ( byte* )g_cl.m_local, false, ( const byte* )map, true, CPredictionCopy::TRANSFERDATA_COPYONLY );
	CopyHelper.TransferData( "InputPrediction::SetDatamap", g_cl.m_local->index( ), g_cl.m_local->GetPredDescMap( ) );
}

void InputPrediction::update( ) {
	m_disable_setup_bones = true;
	int delta_tick = g_csgo.m_cl->m_delta_tick;
	int last_command_ack = g_csgo.m_cl->m_last_command_ack;

	int outgoing_command = g_csgo.m_cl->m_last_outgoing_command + g_csgo.m_cl->m_choked_commands;

	// previous start frame
	g_csgo.m_prediction->startframe( ) = delta_tick - 1;

	g_csgo.m_prediction->Update(
		delta_tick,        // startframe
		true,    // validframe
		last_command_ack,  // incoming_acknowledged
		outgoing_command   // outgoing_command
	);
	m_disable_setup_bones = false;
}

void InputPrediction::store( )
{
	m_curtime = g_csgo.m_globals->m_curtime;
	m_frametime = g_csgo.m_globals->m_frametime;
}

// Nice.
static float bone_yaw_pose = 0.f;
static bool apply_foot_yaw = true;

void InputPrediction::simulate_animations( CUserCmd* cmd ) {

	CCSGOPlayerAnimState* state = g_cl.m_local->m_PlayerAnimState( );
	if ( !state )
		return;

	if ( g_csgo.m_cl->m_delta_tick == -1 )
		return g_cl.m_update_records.clear( );

	if ( g_cl.m_update_records.empty( ) )
		return;

	for ( int i = 0; i < g_cl.m_update_records.size( ); i++ )
	{
		auto& current = g_cl.m_update_records[ i ];
		if ( current.m_cmd_num != cmd->m_command_number )
			continue;

		g_cl.m_angle = cmd->m_view_angles;

		auto new_state = g_server_animations.PredictAnimationState( g_cl.m_local, cmd->m_view_angles ).first;

		g_server_animations.DoPreAnimation( new_state, cmd, current.m_viewmodel_act );

		if ( state->m_last_update_frame == g_csgo.m_globals->m_frame )
			state->m_last_update_frame--;

		C_AnimationLayer maintain_layers[ 13 ];
		g_cl.m_local->GetAnimLayers( maintain_layers );

		state->update( cmd->m_view_angles );

		g_netdata.MaintainClientLayers( maintain_layers );

		g_cl.m_abs_yaw = state->m_abs_yaw;

		// predict without the foot yaw check, like how our enemys will.
		if ( state->m_on_ground )
		{
			if ( state->m_velocity_length_xy > 0.1f )
			{
				g_cl.m_enemy_body_pred = g_csgo.m_globals->m_curtime + 0.22f;
			}
			else if ( g_csgo.m_globals->m_curtime > g_cl.m_enemy_body_pred )
			{
				g_cl.m_enemy_body_pred = g_csgo.m_globals->m_curtime + 1.1f;
			}
		}

		switch ( current.m_hide_angle ) {
		case 0: {
			if ( !apply_foot_yaw ) {
				const bool moving = g_cl.m_local->m_vecVelocity( ).length( ) > 0.1f;

				if ( moving ) {
					g_cl.m_render_abs = g_cl.m_abs_yaw;
					bone_yaw_pose = g_cl.m_local->m_flPoseParameter( )[ 11 ];
					apply_foot_yaw = true;
				}
				else if ( auto* state = g_cl.m_local->m_PlayerAnimState( ) ) {
					if ( state->m_abs_yaw == state->m_abs_yaw_last )
						apply_foot_yaw = true;
				}
			}

			if ( apply_foot_yaw ) {
				g_cl.m_render_abs = g_cl.m_abs_yaw;
				bone_yaw_pose = g_cl.m_local->m_flPoseParameter( )[ 11 ];
			}
			break;
		}

		case 1:
			apply_foot_yaw = false;
			break;

		case 2:
			apply_foot_yaw = false;
			g_bones.SetupBonesSimple( g_cl.m_local, BONE_USED_BY_ANYTHING & ~BONE_USED_BY_BONE_MERGE, g_chams.local_hidden );
			g_chams.last_hidden_time = g_csgo.m_globals->m_curtime;
			break;
		}
		break;
	}
}

void InputPrediction::run( Entity* ent, CUserCmd* cmd, IMoveHelper* movehelper )
{
	// familyhuk mim mim mim
	if ( cmd->m_tick == INT_MAX )                   // [2]?
		return;

	Player* player = reinterpret_cast< Player* >( ent );
	if ( !player || !player->m_bIsLocalPlayer( ) || !player->alive( ) )
		return g_hooks.m_prediction.GetOldMethod< Hooks::RunCommand_t >( CPrediction::RUNCOMMAND )( g_csgo.m_prediction, ent, cmd, movehelper );

	auto backup_cmd = g_cl.m_cmd;

	g_cl.m_cmd = cmd;

	// we want to call runcommand lulmem
	if ( !m_in_pred ) {
		g_netdata.RestoreAnims( cmd->m_command_number - 1 );
		g_netdata.SetViewmodel( cmd->m_command_number - 1 );
	}

	int weapon_accuracy_nospread = g_csgo.weapon_accuracy_nospread->GetInt( );
	int sv_showimpacts = g_csgo.sv_showimpacts->GetInt( );
	int pre_pred_flags = player->m_fFlags( );

	if ( g_menu.main.visuals.bullet_impacts.get( ) )
		g_csgo.sv_showimpacts->SetValue( 1 );

	g_csgo.weapon_accuracy_nospread->SetValue( 1 );

	g_hooks.m_prediction.GetOldMethod< Hooks::RunCommand_t >( CPrediction::RUNCOMMAND )( g_csgo.m_prediction, ent, cmd, movehelper );

	g_csgo.weapon_accuracy_nospread->SetValue( weapon_accuracy_nospread );
	g_csgo.sv_showimpacts->SetValue( sv_showimpacts );

	// lol mim
	CCSGOPlayerAnimState* state = g_cl.m_local->m_PlayerAnimState( );

	if ( ( pre_pred_flags & 1 ) != 0
		&& ( player->m_fFlags( ) & 1 ) == 0
		&& ( cmd->m_buttons & IN_JUMP ) != 0 )
	{
		if ( state ) {
			g_server_animations.DoAnimationEvent(
				state,
				PLAYERANIMEVENT_JUMP, g_cl.m_local->m_AnimOverlay( )
			);
		}
	}

	if ( !m_in_pred ) {
		g_cl.m_local->m_vphysicsCollisionState( ) = 0;

		simulate_animations( cmd );
		
		if( !m_disable_setup_bones )
			setup_bones( );

		g_netdata.StoreAnims( cmd->m_command_number );
		g_netdata.StoreViewmodel( cmd->m_command_number );
	}

	g_cl.m_cmd = backup_cmd;
}

float InputPrediction::get_lowest_inaccuracy( Weapon* weapon )
{
	const auto id = weapon->m_iItemDefinitionIndex( );

	const auto scoped = ( g_menu.main.aimbot.automatic_stop.get( 5 ) || weapon->m_zoomLevel( ) )
		&& ( id == AWP || id == G3SG1 || id == SCAR20 || id == SSG08 );

	const auto crouched = g_cl.m_local->m_fFlags( ) & FL_DUCKING;
	const auto wpn_data = weapon->GetWpnData( );

	if ( crouched )
		return scoped ? wpn_data->m_inaccuracy_crouch_alt : wpn_data->m_inaccuracy_crouch;

	return scoped ? wpn_data->m_inaccuracy_stand_alt : wpn_data->m_inaccuracy_stand;
}

void InputPrediction::setup_bones( )
{
	// setup shoot matrix
	float backup_pitch = g_cl.m_local->m_flPoseParameter( )[ 12 ];
	g_cl.m_local->m_flPoseParameter( )[ 12 ] = 0.5f;
	g_bones.SetupBonesSimple( g_cl.m_local, BONE_USED_BY_HITBOX, g_server_animations.local_matrix );
	g_cl.m_local->m_flPoseParameter( )[ 12 ] = backup_pitch;

	g_cl.m_local->m_flPoseParameter( )[ 11 ] = bone_yaw_pose;

	// setup render matrix
	g_cl.m_local->SetAbsAngles( ang_t( 0.f, g_cl.m_render_abs, 0.f ) );
	g_bones.SetupBonesSimple( g_cl.m_local, BONE_USED_BY_ANYTHING, g_server_animations.local_render_matrix );
	g_server_animations.last_render_setup = g_cl.m_local->GetAbsOrigin( );
}

void InputPrediction::predict( ) {
	CUserCmd m_dummy_cmd;
	std::memcpy( &m_dummy_cmd, g_cl.m_cmd, sizeof( CUserCmd ) );

	g_csgo.m_prediction->in_prediction( ) = true;
	g_csgo.m_prediction->first_time_predicting( ) = false;

	int prev_tickbase = g_cl.m_local->m_nTickBase( );
	m_in_pred = true;

	g_csgo.m_move_helper->SetHost( g_cl.m_local );
	run( g_cl.m_local, &m_dummy_cmd, g_csgo.m_move_helper );
	g_csgo.m_move_helper->SetHost( nullptr );

	m_in_pred = false;

	g_cl.m_local->m_nTickBase( ) = prev_tickbase;
}

void InputPrediction::restore( )
{
	g_csgo.m_prediction->in_prediction( ) = false;
	g_csgo.m_prediction->first_time_predicting( ) = true;

	*g_csgo.m_nPredictionRandomSeed = -1;
	g_csgo.m_pPredictionPlayer = nullptr;

	g_csgo.m_globals->m_curtime = m_curtime;
	g_csgo.m_globals->m_frametime = m_frametime;
}

CPredictionCopy::CPredictionCopy( int type, byte* dest, bool dest_packed, const byte* src, bool src_packed, optype_t opType, FN_FIELD_COMPARE func )
{
	m_OpType = opType;
	m_nType = type;
	m_pDest = dest;
	m_pSrc = src;
	m_nDestOffsetIndex = dest_packed ? TD_OFFSET_PACKED : TD_OFFSET_NORMAL;
	m_nSrcOffsetIndex = src_packed ? TD_OFFSET_PACKED : TD_OFFSET_NORMAL;

	m_nErrorCount = 0;
	m_nEntIndex = -1;

	m_pWatchField = nullptr;
	m_FieldCompareFunc = func;
}

int CPredictionCopy::TransferData( const char* operation, int entindex, datamap_t* dmap )
{
	using TransferDataFn = int( __thiscall* )( CPredictionCopy*, const char*, int, datamap_t* );
	static auto pat = pattern::find( g_csgo.m_client_dll, XOR( "55 8B EC 8B 45 10 53 56 8B F1 57" ) ).as< TransferDataFn >( );
	return pat( this, operation, entindex, dmap );
}