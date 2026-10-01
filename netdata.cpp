#include "includes.h"

NetData g_netdata {};;

void NetData::StoreAnims( int cmd_num, bool store_state )
{
	if ( !g_cl.m_processing )
		return;

	PredictionData_t* data = &m_prediction_data[ cmd_num % MULTIPLAYER_BACKUP ];
	data->m_cmd_num = cmd_num;

	g_cl.m_local->GetAnimLayers( data->m_layers );

	if ( store_state )
	{
		CCSGOPlayerAnimState* state = g_cl.m_local->m_PlayerAnimState( );
		if ( state )
		{
			data->m_state = *state;
		}
		data->m_fake_feet_yaw = g_server_animations.m_fake_feet_yaw;
	}

	data->m_timebase = g_csgo.m_globals->m_curtime;
	data->m_shoot_pos = g_cl.m_local->GetShootPosition( );
	data->m_inaccuracy = 1.f;

	Weapon* weapon = g_cl.m_local->GetActiveWeapon( );
	if ( !weapon || weapon != g_cl.m_weapon )
		return;

	if ( !g_cl.CanFireWeapon( ) )
		return;

	data->m_inaccuracy = weapon->GetInaccuracy( );
}

void NetData::RestoreAnims( int cmd_num )
{
	if ( !g_cl.m_processing )
		return;

	PredictionData_t* data = &m_prediction_data[ cmd_num % MULTIPLAYER_BACKUP ];
	if ( data->m_cmd_num != cmd_num )
	{
		return;
	}

	for ( int i = 0; i < ANIMATION_LAYER_COUNT; i++ )
	{
		if ( i != ANIMATION_LAYER_WHOLE_BODY && i != ANIMATION_LAYER_FLASHED && i != ANIMATION_LAYER_FLINCH )
		{
			memcpy( &g_cl.m_local->m_AnimOverlay( )[ i ], &data->m_layers[ i ], sizeof( C_AnimationLayer ) );
		}
	}

	CCSGOPlayerAnimState* state = g_cl.m_local->m_PlayerAnimState( );
	// frame check should work fine.
	if ( state && data->m_state.m_last_update_frame > 0.f )
	{
		*state = data->m_state;
	}
	g_server_animations.m_fake_feet_yaw = data->m_fake_feet_yaw;
}

void NetData::MaintainClientLayers( C_AnimationLayer* layers )
{
	// client does shit on this layer (since normally the client wouldnt play it), but we rebuild it, so dont.
	memcpy( &g_cl.m_local->m_AnimOverlay( )[ ANIMATION_LAYER_ADJUST ], &layers[ ANIMATION_LAYER_ADJUST ], sizeof( C_AnimationLayer ) );
}

float& NetData::GetTimebase( int cmd_num )
{
	return m_prediction_data[ cmd_num % MULTIPLAYER_BACKUP ].m_timebase;
}

vec3_t& NetData::GetShootPos( int cmd_num )
{
	return m_prediction_data[ cmd_num % MULTIPLAYER_BACKUP ].m_shoot_pos;
}

float& NetData::GetInaccuracy( int cmd_num )
{
	return m_prediction_data[ cmd_num % MULTIPLAYER_BACKUP ].m_inaccuracy;
}

bool NetData::FixCompression( int commands_acknowledged ) {
	// grab predicted frame.
	auto pred_frame = g_cl.m_local->GetPredictedFrame( commands_acknowledged  );
	if ( !pred_frame )
		return false;

	bool outside_tolerance = false;

	// recompute packed data.
	auto optimized_datamap = g_cl.m_local->GetPredDescMap( );
	g_cl.m_local->PrepareDatamap( optimized_datamap );

	// PC_NETWORKED_ONLY = 1
	auto& flat = optimized_datamap->m_pOptimizedDataMap->m_Info[ 1 ].m_Flat;

	for ( int i = 0; i < flat.m_Flattened.Count( ); i++ ) {
		auto field = &flat.m_Flattened[ i ];
		if ( field->field_tolerance == 0.f )
			continue;

		const auto normal_offset = field->flat_offset[ TD_OFFSET_NORMAL ];
		const auto packed_offset = field->flat_offset[ TD_OFFSET_PACKED ];

		switch ( field->field_type ) {
		case FIELD_FLOAT: {
			float net = g_cl.m_local->get<float>( normal_offset );
			float pred = *reinterpret_cast< float* >( pred_frame + packed_offset );

			if ( std::abs( net - pred ) <= field->field_tolerance ) {
				g_cl.m_local->set<float>( normal_offset, pred );
			}
			else {
				outside_tolerance = true;
			}
			break;
		}
		case FIELD_VECTOR: {
			vec3_t net = g_cl.m_local->get<vec3_t>( normal_offset );
			vec3_t pred = *reinterpret_cast< vec3_t* >( pred_frame + packed_offset );

			if ( ( std::abs( net.x - pred.x ) <= field->field_tolerance ) &&
				( std::abs( net.y - pred.y ) <= field->field_tolerance ) &&
				( std::abs( net.z - pred.z ) <= field->field_tolerance ) ) {
				g_cl.m_local->set<vec3_t>( normal_offset, pred );
			}
			else {
				outside_tolerance = true;
			}
			break;
		}
		default:
			break;
		}
	}

	if ( outside_tolerance )
		g_cl.m_last_prediction_error_time = g_csgo.m_globals->m_curtime;

	return outside_tolerance;
}

void NetData::StoreViewmodel( int cmd_num )
{
	auto viewmodel = g_csgo.m_entlist->GetClientEntityFromHandle( g_cl.m_local->m_hViewModel( ) );
	if ( viewmodel )
	{
		m_viewmodel_data[ cmd_num % MULTIPLAYER_BACKUP ] = {
			viewmodel->m_flCycle( ),
			viewmodel->m_flAnimTime( )
		};
	}
}

void NetData::SetViewmodel( int cmd_num )
{
	auto viewmodel = g_csgo.m_entlist->GetClientEntityFromHandle( g_cl.m_local->m_hViewModel( ) );
	if ( viewmodel )
	{
		auto bro = &m_viewmodel_data[ cmd_num % MULTIPLAYER_BACKUP ];
		viewmodel->m_flCycle( ) = bro->first;
		viewmodel->m_flAnimTime( ) = bro->second;
	}
}