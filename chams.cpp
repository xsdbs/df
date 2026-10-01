#include "includes.h"

Chams g_chams { };;

void Chams::SetColor( int material, Color col )
{
	if ( material > -1 )
		m_materials[ material ]->ColorModulate( col );

	else
		g_csgo.m_render_view->SetColorModulation( col );
}

void Chams::SetAlpha( int material, float alpha )
{
	if ( material > -1 )
		m_materials[ material ]->AlphaModulate( alpha );

	else
		g_csgo.m_render_view->SetBlend( alpha );
}

void Chams::SetupMaterial( int mat, Color col, bool z_flag )
{
	SetColor( mat, col );
	SetAlpha( mat, col.a( ) / 255.f );

	if ( mat == -2 )
		return;

	// mat->SetFlag( MATERIAL_VAR_HALFLAMBERT, flags );
	if ( mat == -1 )
	{
		g_csgo.m_studio_render->ForcedMaterialOverride( nullptr );
		return;
	}

	m_materials[ mat ]->SetFlag( MATERIAL_VAR_ZNEARER, z_flag );
	m_materials[ mat ]->SetFlag( MATERIAL_VAR_NOFOG, z_flag );
	m_materials[ mat ]->SetFlag( MATERIAL_VAR_IGNOREZ, z_flag );

	bool doalpha = false;
	switch ( m_draw_side )
	{
	case drawside_t::local: doalpha = g_menu.main.visuals.chams_local_phong.get( ); break;
	case drawside_t::enemy: doalpha = g_menu.main.players.chams_metallic_enemy_color.get( ); break;
	case drawside_t::friendly: doalpha = g_menu.main.players.chams_metallic_friendly_color.get( ); break;
	}

	if ( mat == materialtype_t::shine )
	{
		if ( !doalpha )
		{ // ghetto fix but idc
			mat++;

			m_materials[ mat ]->SetFlag( MATERIAL_VAR_ZNEARER, z_flag );
			m_materials[ mat ]->SetFlag( MATERIAL_VAR_NOFOG, z_flag );
			m_materials[ mat ]->SetFlag( MATERIAL_VAR_IGNOREZ, z_flag );

			SetColor( mat, col );
			SetAlpha( mat, col.a( ) / 255.f );
		}

		auto reflectivity = m_materials[ mat ]->FindVar( "$envmaptint", nullptr );
		if ( reflectivity )
		{
			Color col = colors::white;
			float ratio = 1.f;

			switch ( m_draw_side )
			{
			case drawside_t::local:
				col = g_menu.main.visuals.chams_metallic_local_color.get( );
				ratio = col.a( ) / 255.f;
				break;
			case drawside_t::enemy:
				col = g_menu.main.players.chams_metallic_enemy_color.get( );
				ratio = col.a( ) / 255.f;
				break;
			case drawside_t::friendly:
				col = g_menu.main.players.chams_metallic_friendly_color.get( );
				ratio = col.a( ) / 255.f;
				break;
			}

			reflectivity->SetVecValue(
				( col.r( ) / 255.f ) * ratio,
				( col.g( ) / 255.f ) * ratio,
				( col.b( ) / 255.f ) * ratio );
		}

		if ( mat == materialtype_t::shine2 )
		{
			auto phongexponent = m_materials[ mat ]->FindVar( "$phongexponent", nullptr );
			if ( phongexponent )
			{
				int amt = 0;

				switch ( m_draw_side )
				{
				case drawside_t::local: amt = g_menu.main.visuals.chams_local_phong.get( ); break;
				case drawside_t::enemy: amt = g_menu.main.players.chams_enemy_phong.get( ); break;
				case drawside_t::friendly: amt = g_menu.main.players.chams_friendly_phong.get( ); break;
				}

				phongexponent->SetIntValue( amt );
			}
		}
	}

	g_csgo.m_studio_render->ForcedMaterialOverride( m_materials[ mat ] );
}

void Chams::init( )
{
	std::ofstream( "csgo\\materials\\shine.vmt" ) << ( R"#("VertexLitGeneric"{
		"$basetexture" "vgui/white_additive"
		"$phong" "1"
		"$phongexponent" "15"
		"$basemapalphaphongmask" "1"
		"basemapluminancephongmask" "1"
		"$normalmapalphaenvmask" "1"
		"$envmap" "editor/cube_vertigo"
		"$nofog" "1"
		"$ingorez" "1"
		"$model" "1"
		"$nocull" "0"
		"$selfillum" "1"
		"$halflambert" "1"
		"$znearer" "0"
		"$flat" "1"
	})#" );

	std::ofstream( "csgo\\materials\\shine2.vmt" ) << ( R"#("VertexLitGeneric"{
		"$basetexture" "vgui/white_additive"
		"$phong" "1"
		"$phongexponent" "15"
		"$phongboost" "0.4"
		"$normalmapalphaenvmask" "1"
		"$envmap" "editor/cube_vertigo"
		"$nofog" "1"
		"$ingorez" "1"
		"$model" "1"
		"$nocull" "0"
		"$selfillum" "1"
		"$halflambert" "1"
		"$znearer" "0"
		"$flat" "1"
	})#" );

	// find stupid materials.
	m_materials[ materialtype_t::textured ] = g_csgo.m_material_system->FindMaterial( XOR( "debug/debugambientcube" ), XOR( "Model textures" ) );
	m_materials[ materialtype_t::textured ]->IncrementReferenceCount( );

	m_materials[ materialtype_t::flat ] = g_csgo.m_material_system->FindMaterial( XOR( "debug/debugdrawflat" ), XOR( "Model textures" ) );
	m_materials[ materialtype_t::flat ]->IncrementReferenceCount( );

	m_materials[ materialtype_t::shine ] = g_csgo.m_material_system->FindMaterial( XOR( "shine" ), XOR( "Model textures" ) );
	m_materials[ materialtype_t::shine ]->IncrementReferenceCount( );

	m_materials[ materialtype_t::shine2 ] = g_csgo.m_material_system->FindMaterial( XOR( "shine2" ), XOR( "Model textures" ) );
	m_materials[ materialtype_t::shine2 ]->IncrementReferenceCount( );
}

bool Chams::GenerateRetrackMatrix( Player* player, int index, BoneArray* out, float& alphamod )
{
	LagRecord* current_record = nullptr;

	LagCompensation* data = &g_lagcompensation.m_players[ index - 1 ];
	if ( !data || data->m_records.size( ) < 2 || !g_cl.m_local || !g_cl.m_local->alive( ) || !data->m_show_backtrack )
		return false;

	for ( auto& curr : data->m_records ) {
		if ( curr.m_broke_lc )
			break;

		if ( !curr.valid( ) || !curr.m_hittable )
			continue;

		if ( !current_record || curr.m_mode > current_record->m_mode )
			current_record = &curr;
	}

	if ( !current_record )
		return false;

	float max_ticks = 32.f;
	float elapsed = max_ticks - abs( current_record->m_hittable_tick - g_csgo.m_globals->m_tick_count );
	alphamod = std::clamp( elapsed / max_ticks, 0.f, 1.f );

	memcpy( out, current_record->m_bones, sizeof( BoneArray ) * 128 );

	return true;
}

bool Chams::GenerateLerpedMatrix( Player* player, int index, BoneArray* out, float& alphamod )
{
	LagRecord* current_record;

	auto data = &g_lagcompensation.m_players[ index - 1 ];
	if ( !data || data->m_records.size( ) < 2 || !g_cl.m_local || !g_cl.m_local->alive( ) || !data->m_show_backtrack )
		return false;

	LagRecord* last_valid = nullptr;
	LagRecord* first_invalid = nullptr;

	bool had_valid = false;

	for ( int i = 0; i < data->m_records.size( ) - 1; i++ )
	{
		if ( data->m_records[ i ].m_broke_lc )
		{
			if ( i > 0 )
			{
				last_valid = &data->m_records[ i - 1 ];
				first_invalid = &data->m_records[ i ];
			}
			break;
		}

		if ( !had_valid )
		{
			if ( data->m_records[ i ].valid( ) )
				had_valid = true;
			else
				continue;
		}

		if ( !data->m_records[ i + 1 ].valid( ) )
		{
			last_valid = &data->m_records[ i ];
			first_invalid = &data->m_records[ i + 1 ];
			break;
		}
	}

	if ( !last_valid || !first_invalid || last_valid->m_broke_lc )
		return false;

	if ( ( last_valid->m_origin - first_invalid->m_origin ).length( ) > 4096.f )
		return false;

	auto flDelta = 1.f - ( g_csgo.m_globals->m_curtime - last_valid->m_interp_time ) / ( last_valid->m_sim_time - first_invalid->m_sim_time );
	if ( flDelta < 0.f || flDelta > 1.f )
	{
		last_valid->m_interp_time = g_csgo.m_globals->m_curtime;
		flDelta = 1.f - ( g_csgo.m_globals->m_curtime - last_valid->m_interp_time ) / ( last_valid->m_sim_time - first_invalid->m_sim_time );
	}

	const auto lerp = math::Interpolate( last_valid->m_origin, first_invalid->m_origin, std::clamp( flDelta, 0.f, 1.f ) );

	constexpr float max_dist = 30.f;
	alphamod = std::min( ( lerp - player->GetAbsOrigin( ) ).length( ), max_dist ) / max_dist;

	memcpy( out, first_invalid->m_bones, sizeof( BoneArray ) * 128 );

	for ( int i = 0; i < 128; i++ )
	{
		const auto matrix_delta = first_invalid->m_bones[ i ].GetOrigin( ) - first_invalid->m_origin;
		out[ i ].SetOrigin( matrix_delta + lerp );
	}

	return true;
}

bool Chams::GetFirstValidMatrix( Player* player, int index, BoneArray* out, float& alphamod )
{
	LagRecord* current_record;

	auto data = &g_lagcompensation.m_players[ index - 1 ];
	if ( !data || data->m_records.size( ) < 2 || !g_cl.m_local || !g_cl.m_local->alive( ) || !data->m_show_backtrack )
		return false;

	bool front_valid = data->m_records.front( ).valid( );
	if ( front_valid )
		return false;

	for ( int i = 0; i < data->m_records.size( ); i++ )
	{
		LagRecord* record = &data->m_records[ i ];
		if ( !record )
			continue;

		if ( record->m_broke_lc )
			break;

		if ( !record->valid( ) )
			continue;

		constexpr float max_dist = 30.f;
		alphamod = std::min( ( record->m_origin - player->GetAbsOrigin( ) ).length( ), max_dist ) / max_dist;

		memcpy( out, record->m_bones, sizeof( BoneArray ) * 128 );
		return true;
	}

	return false;
}

Entity* GetMoveParentEntity( Entity* pEntity )
{
	if ( !pEntity )
		return false;

	if ( !pEntity->moveparent( ).IsValid( ) )
		return false;

	return g_csgo.m_entlist->GetClientEntityFromHandle( pEntity->moveparent( ) );
}

bool AttachmentBelongsToPlayer( Entity* pEntity, Entity* pOwnerEntity )
{
	if ( !pEntity )
		return false;

	if ( !pOwnerEntity )
		return false;

	auto pAttachmentParent = GetMoveParentEntity( pEntity );
	if ( !pAttachmentParent )
		return false;

	return pAttachmentParent->index( ) == pOwnerEntity->index( );
}

bool Chams::OnDrawModelExecute( Player* ent, void* ecx, IMatRenderContext* ctx,
	DrawModelState_t& state, ModelRenderInfo_t& info,
	matrix3x4_t* custom_bones )
{
	if ( !g_cl.m_local )
		return false;

	int class_id = ent->GetClientClass( )->m_ClassID;

	// Skip dormant entities (except gloves)
	if ( class_id != _CEconWearable && ent->dormant( ) )
		return false;

	auto restore_render = [ ]( ) {
		g_csgo.m_studio_render->ForcedMaterialOverride( nullptr );
		g_csgo.m_render_view->SetColorModulation( colors::white );
		g_csgo.m_render_view->SetBlend( 1.f );
		};

	auto draw_with_material = [ & ]( int mat_index, Color col, bool ignore_z, matrix3x4_t* bones = nullptr ) {
		if ( !bones )
			bones = custom_bones;

		if ( !bones )
			return;

		if ( mat_index >= 0 )
			SetupMaterial( mat_index, col, ignore_z );
		oDrawModelExecute( ecx, ctx, state, info, bones );
		restore_render( );
		};

	const bool blend = g_cl.m_local->m_bIsScoped( );

	// Gloves
	if ( class_id == _CEconWearable )
	{
		const auto owner = ent->GetShadowParent( ) ? reinterpret_cast< Player* >( ent->GetShadowParent( )->GetIClientUnknown( )->GetIClientEntity( ) ) : nullptr;
		if ( !owner || !owner->m_bIsLocalPlayer( ) )
			return false;

		//int mat_index = -1;
		//if ( !blend ) {
		//	if ( g_menu.main.visuals.chams_local.get( 3 ) ) mat_index = 0;
		//	else if ( g_menu.main.players.glow.get( 4 ) ) mat_index = 1;
		//	else if ( g_menu.main.players.glow.get( 5 ) ) mat_index = 2;

		//	draw_with_material( mat_index, g_menu.main.visuals.chams_local_color.get( ), false );
		//}
		//else
		//{
		g_csgo.m_render_view->SetBlend( 0.5f );
		oDrawModelExecute( ecx, ctx, state, info, custom_bones );
		restore_render( );
		//}

		return true;
	}

	if ( g_csgo.m_model_render->IsForcedMaterialOverride( ) )
		return false;

	switch ( class_id )
	{
	case _CCSPlayer:
	{
		if ( !ent->alive( ) )
			return false;

		// Local player
		if ( ent->m_bIsLocalPlayer( ) )
		{
			m_draw_side = drawside_t::local;

			int mat_index = -1;
			if ( g_menu.main.visuals.chams_local.get( 1 ) ) mat_index = 0;
			else if ( g_menu.main.visuals.chams_local.get( 2 ) ) mat_index = 1;
			else if ( g_menu.main.visuals.chams_local.get( 3 ) ) mat_index = 2;

			Color col = colors::white;
			if ( g_menu.main.visuals.chams_local.get( 0 ) && !blend ) {
				col = g_menu.main.visuals.chams_local_color.get( );
				draw_with_material( mat_index, col, false );
			}
			else if ( blend ) {
				g_csgo.m_render_view->SetBlend( g_menu.main.visuals.chams_local_scope_blend.get( ) * 0.01f );
				oDrawModelExecute( ecx, ctx, state, info, custom_bones );
				restore_render( );
			}
			else {
				oDrawModelExecute( ecx, ctx, state, info, custom_bones );
				restore_render( );
			}

			// LBY chams
			if ( g_menu.main.visuals.visualize_lby.get( ) && g_csgo.m_globals->m_curtime > last_hidden_time && g_csgo.m_globals->m_curtime - last_hidden_time < 0.5f )
			{
				col = g_menu.main.visuals.chams_lby_local_color.get( );
				float delta = 4.f * ( g_csgo.m_globals->m_curtime - last_hidden_time );
				float alpha = delta <= 1.f ? delta * std::min( ( int )col.a( ), 255 )
					: fabsf( 2.f - delta ) * std::min( ( int )col.a( ), 255 ); // i am a psychopath

				col.a( ) = alpha;

				draw_with_material( materialtype_t::flat, col, false, local_hidden );
			}

			return true;
		}

		// Enemy player
		if ( ent->enemy( g_cl.m_local ) )
		{
			m_draw_side = drawside_t::enemy;

			BoneArray out[ 128 ];
			float alpha_mod = 1.f;

			if ( g_menu.main.players.chams_enemy.get( 2 ) )
			{
				if ( GetFirstValidMatrix( ent, ent->index( ), out, alpha_mod ) ) {
					Color col = g_menu.main.players.chams_enemy_history_col.get( );
					col.a( ) *= alpha_mod;
					draw_with_material( materialtype_t::flat, col, true, out );
				}

				if ( GenerateLerpedMatrix( ent, ent->index( ), out, alpha_mod ) ) {
					Color col = g_menu.main.players.chams_enemy_history_col.get( );
					col.a( ) *= alpha_mod;
					draw_with_material( materialtype_t::flat, col, true, out );
				}
			}

			if ( g_menu.main.players.chams_enemy.get( 3 ) && GenerateRetrackMatrix( ent, ent->index( ), out, alpha_mod ) ) {
				Color col = g_menu.main.players.chams_enemy_history_col.get( );
				col.a( ) *= alpha_mod;
				draw_with_material( materialtype_t::flat, col, true, out );
			}

			if ( g_menu.main.players.chams_enemy.get( 0 ) )
			{
				int mat_index = -1;
				if ( g_menu.main.players.chams_enemy.get( 5 ) ) mat_index = 0;
				else if ( g_menu.main.players.chams_enemy.get( 6 ) ) mat_index = 1;
				else if ( g_menu.main.players.chams_enemy.get( 7 ) ) mat_index = 2;

				bool hittable = g_menu.main.players.chams_enemy.get( 4 ) && g_aimbot.m_players[ ent->index( ) - 1 ].m_target;
				if ( g_menu.main.players.chams_enemy.get( 1 ) ) {
					Color color = g_menu.main.players.chams_enemy_invis.get( );
					if ( hittable )
						color = g_menu.main.players.chams_enemy_threat.get( );

					draw_with_material( mat_index, color, true );
				}

				Color color = g_menu.main.players.chams_enemy_vis.get( );
				if ( hittable )
					color = g_menu.main.players.chams_enemy_threat.get( );

				draw_with_material( mat_index, color, false );
				return true;
			}
		}

		// Friendly player
		if ( g_menu.main.players.chams_friendly.get( 0 ) )
		{
			m_draw_side = drawside_t::friendly;

			int mat_index = -1;
			if ( g_menu.main.players.chams_friendly.get( 2 ) ) mat_index = 0;
			else if ( g_menu.main.players.chams_friendly.get( 3 ) ) mat_index = 1;
			else if ( g_menu.main.players.chams_friendly.get( 4 ) ) mat_index = 2;

			if ( g_menu.main.players.chams_friendly.get( 1 ) )
				draw_with_material( mat_index, g_menu.main.players.chams_friendly_invis.get( ), true );

			draw_with_material( mat_index, g_menu.main.players.chams_friendly_vis.get( ), false );
			return true;
		}

		break;
	}

	default:
		return false;
	}

	return false;
}