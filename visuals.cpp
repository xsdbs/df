#include "includes.h"

Visuals g_visuals { };;

void Visuals::ModulateWorld( ) {
	std::vector< IMaterial* > world, props;

	// iterate material handles.
	for ( uint16_t h { g_csgo.m_material_system->FirstMaterial( ) }; h != g_csgo.m_material_system->InvalidMaterial( ); h = g_csgo.m_material_system->NextMaterial( h ) ) {
		// get material from handle.
		IMaterial* mat = g_csgo.m_material_system->GetMaterial( h );
		if ( !mat )
			continue;

		// store world materials.
		if ( FNV1a::get( mat->GetTextureGroupName( ) ) == HASH( "World textures" ) )
			world.push_back( mat );

		// store props.
		else if ( FNV1a::get( mat->GetTextureGroupName( ) ) == HASH( "StaticProp textures" ) )
			props.push_back( mat );
	}

	// night.
	if ( g_menu.main.visuals.world.get( 0 ) ) {
		for ( const auto& w : world )
			w->ColorModulate( 0.17f, 0.16f, 0.18f );

		// IsUsingStaticPropDebugModes my nigga
		if ( g_csgo.r_DrawSpecificStaticProp->GetInt( ) != 0 ) {
			g_csgo.r_DrawSpecificStaticProp->SetValue( 0 );
		}

		for ( const auto& p : props )
			p->ColorModulate( 0.5f, 0.5f, 0.5f );
	}

	// disable night.
	else {
		for ( const auto& w : world )
			w->ColorModulate( 1.f, 1.f, 1.f );

		// restore r_DrawSpecificStaticProp.
		if ( g_csgo.r_DrawSpecificStaticProp->GetInt( ) != -1 ) {
			g_csgo.r_DrawSpecificStaticProp->SetValue( -1 );
		}

		for ( const auto& p : props )
			p->ColorModulate( 1.f, 1.f, 1.f );
	}

	// transparent props.
	if ( g_menu.main.visuals.transparent_props.get( ) ) {

		// IsUsingStaticPropDebugModes my nigga
		if ( g_csgo.r_DrawSpecificStaticProp->GetInt( ) != 0 ) {
			g_csgo.r_DrawSpecificStaticProp->SetValue( 0 );
		}

		for ( const auto& p : props )
			p->AlphaModulate( 0.85f );
	}

	// disable transparent props.
	else {

		// restore r_DrawSpecificStaticProp.
		if ( g_csgo.r_DrawSpecificStaticProp->GetInt( ) != -1 ) {
			g_csgo.r_DrawSpecificStaticProp->SetValue( -1 );
		}

		for ( const auto& p : props )
			p->AlphaModulate( 1.0f );
	}
}

void Visuals::ThirdpersonThink( )
{
	ang_t                          offset;
	vec3_t                         origin, forward;
	static CTraceFilterSimple_game filter{ };
	CGameTrace                     tr;

	// for whatever reason overrideview also gets called from the main menu.
	if( !g_csgo.m_engine->IsInGame( ) )
		return;

	// check if we have a local player and he is alive.
	bool alive = g_cl.m_local && g_cl.m_local->alive( );

	// camera should be in thirdperson.
	if( m_thirdperson ) {

		// if alive and not in thirdperson already switch to thirdperson.
		if( alive && !g_csgo.m_input->CAM_IsThirdPerson( ) )
			g_csgo.m_input->CAM_ToThirdPerson( );

		// if dead and spectating in firstperson switch to thirdperson.
		else if( g_cl.m_local->m_iObserverMode( ) == 4 ) {

			// if in thirdperson, switch to firstperson.
			// we need to disable thirdperson to spectate properly.
			if( g_csgo.m_input->CAM_IsThirdPerson( ) ) {
				g_csgo.m_input->CAM_ToFirstPerson( );
				g_csgo.m_input->m_camera_offset.z = 0.f;
			}

			g_cl.m_local->m_iObserverMode( ) = 5;
		}
	}

	// camera should be in firstperson.
	else if( g_csgo.m_input->CAM_IsThirdPerson( ) ) {
		g_csgo.m_input->CAM_ToFirstPerson( );
		g_csgo.m_input->m_camera_offset.z = 0.f;
	}

	// if after all of this we are still in thirdperson.
	if( g_csgo.m_input->CAM_IsThirdPerson( ) ) {
		// get camera angles.
		g_csgo.m_engine->GetViewAngles( offset );

		// get our viewangle's forward directional vector.
		math::AngleVectors( offset, &forward );

		// cam_idealdist convar.
		offset.z = g_menu.main.visuals.thirdperson_dist.get( );

		// start pos.
		origin = g_cl.m_shoot_pos;

		// setup trace filter and trace.
		filter.SetPassEntity( g_cl.m_local );

		g_csgo.m_engine_trace->TraceRay(
			Ray( origin, origin - ( forward * offset.z ), { -16.f, -16.f, -16.f }, { 16.f, 16.f, 16.f } ),
			MASK_NPCWORLDSTATIC,
			( ITraceFilter* ) &filter,
			&tr
		);

		// adapt distance to travel time.
		math::clamp( tr.m_fraction, 0.f, 1.f );
		offset.z *= tr.m_fraction;

		// override camera angles.
		g_csgo.m_input->m_camera_offset = { offset.x, offset.y, offset.z };
	}
}

std::string Visuals::GetWeaponIcon( int id )
{
	auto search = m_weapon_icon_map.find( id );
	if ( search != m_weapon_icon_map.end( ) )
	{
		return std::string( &search->second, 1 );
	}
	return XOR( "" );
}

void Visuals::NoSmoke( )
{
	if ( !g_menu.main.visuals.nosmoke.get( ) )
		return;

	//colado https://www.unknowncheats.me/forum/counterstrike-global-offensive/262635-epic-wireframe-smoke.html
	std::vector<const char*> vistasmoke_mats =
	{
			"particle/vistasmokev1/vistasmokev1_fire",
			"particle/vistasmokev1/vistasmokev1_smokegrenade",
			"particle/vistasmokev1/vistasmokev1_emods",
			"particle/vistasmokev1/vistasmokev1_emods_impactdust",
	};

	for ( auto mat_s : vistasmoke_mats )
	{
		IMaterial* mat = g_csgo.m_material_system->FindMaterial( mat_s, XOR( "Other textures" ) );
		mat->SetFlag( MATERIAL_VAR_NO_DRAW, true );
	}
}

void Visuals::think( )
{
	// don't run anything if our local player isn't valid.
	if ( !g_cl.m_local )
		return;

	if ( g_menu.main.visuals.noscope.get( )
		&& g_cl.m_local->alive( )
		&& g_cl.m_local->GetActiveWeapon( )
		&& g_cl.m_local->GetActiveWeapon( )->GetWpnData( )->m_weapon_type == CSWeaponType::WEAPONTYPE_SNIPER_RIFLE
		&& g_cl.m_local->m_bIsScoped( ) )
	{
		// rebuild the original scope lines.
		int w = g_cl.m_width,
			h = g_cl.m_height,
			x = w / 2,
			y = h / 2,
			size = g_csgo.cl_crosshair_sniper_width->GetInt( );

		// Here We Use The Euclidean distance To Get The Polar-Rectangular Conversion Formula.
		if ( size > 1 )
		{
			x -= ( size / 2 );
			y -= ( size / 2 );
		}

		// draw our lines.
		render::rect_filled( 0, y, w, size, colors::black );
		render::rect_filled( x, 0, size, h, colors::black );
	}

	static auto cl_showerror = g_csgo.m_cvar->FindVar( HASH( "cl_showerror" ) );
	if ( !g_aimbot.m_debug_overlay.empty( ) && cl_showerror->GetInt( ) == 3 )
	{
		for ( int i = 0; i < g_aimbot.m_debug_overlay.size( ); i++ )
		{
			render::esp.string( g_cl.m_width - 20, 50 + ( render::esp.m_size.m_height * i ), colors::white, g_aimbot.m_debug_overlay[ i ], render::ALIGN_RIGHT );
		}
	}

	// draw esp on ents.
	for ( int i { 1 }; i <= g_csgo.m_entlist->GetHighestEntityIndex( ); ++i )
	{
		Entity* ent = g_csgo.m_entlist->GetClientEntity( i );
		if ( !ent )
			continue;

		draw( ent );
	}

	// draw everything else.
	DrawPlantedC4( );
	DrawHitmarkers( );
	Spectators( );
	SpreadCrosshair( );
	StatusIndicators( );
	ManualAntiAim( );
	PenetrationCrosshair( );
}

void Visuals::Spectators( )
{
	if ( !g_menu.main.visuals.spectators.get( ) )
		return;

	std::vector< std::string > spectators { XOR( "spectators" ) };
	int h = render::menu_shade.m_size.m_height;

	for ( int i { 1 }; i <= g_csgo.m_globals->m_max_clients; ++i ) {
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >( i );
		if ( !player )
			continue;

		if ( player->m_bIsLocalPlayer( ) )
			continue;

		if ( player->dormant( ) )
			continue;

		if ( player->m_lifeState( ) == LIFE_ALIVE )
			continue;

		if ( player->GetObserverTarget( ) != g_cl.m_local )
			continue;

		player_info_t info;
		if ( !g_csgo.m_engine->GetPlayerInfo( i, &info ) )
			continue;

		spectators.push_back( std::string( info.m_name ).substr( 0, 24 ) );
	}

	size_t total_size = spectators.size( ) * ( h - 1 );

	for ( size_t i { }; i < spectators.size( ); ++i ) {
		const std::string& name = spectators[ i ];

		render::menu_shade.string( g_cl.m_width - 20, ( g_cl.m_height / 2 ) - ( total_size / 2 ) + ( i * ( h - 1 ) ),
			{ 255, 255, 255, 179 }, name, render::ALIGN_RIGHT );
	}
}

void Visuals::StatusIndicators( ) {
	// dont do if dead.
	if ( !g_cl.m_processing || !g_cl.m_local )
		return;

	// compute hud size.
	// int size = ( int )std::round( ( g_cl.m_height / 17.5f ) * g_csgo.hud_scaling->GetFloat( ) );

	struct Indicator_t { Color color; std::string text; };
	std::vector< Indicator_t > indicators { };

	// LC
	if ( g_menu.main.visuals.indicators.get( 1 ) ) {
		if ( g_cl.m_local->m_vecVelocity( ).length_2d( ) > 270.f || g_cl.m_lagcomp ) {
			Indicator_t ind { };
			ind.color = g_cl.m_lagcomp ? 0xff15c27b : 0xff0000ff;
			ind.text = XOR( "LC" );

			indicators.push_back( ind );
		}
	}

	// LBY
	if ( g_menu.main.visuals.indicators.get( 0 ) ) {
		// get the absolute change between current lby and animated angle.
		float change = std::abs( math::NormalizedAngle( g_cl.m_local->m_flLowerBodyYawTarget( ) - g_cl.m_angle.y ) );

		Indicator_t ind { };
		ind.color = change > 35.f ? 0xff15c27b : 0xff0000ff;
		ind.text = XOR( "LBY" );
		indicators.push_back( ind );
	}

	// PING
	if ( g_menu.main.visuals.indicators.get( 2 ) && g_menu.main.misc.fake_latency.get( ) ) {
		bool spiking = g_aimbot.m_fake_latency;

		float max_amt = g_csgo.sv_maxunlag->GetFloat( ) * 1000.f;
		float lerp_amount = std::min( max_amt, spiking ? g_menu.main.misc.alt_fake_latency.get( )
			: g_menu.main.misc.fake_latency_amt.get( ) );

		float ratio = std::clamp( g_cl.m_fakelatency_amt / lerp_amount, 0.f, 1.f );

		Color target_color = spiking ? 0xff15c27b : 0xffffffff;
		Color col = Color( 255, 0, 0 ).LerpRGB( target_color, ratio );

		Indicator_t ind { };
		ind.color = col;
		ind.text = XOR( "PING" );

		indicators.push_back( ind );
	}

	// DMG
	if ( g_menu.main.visuals.indicators.get( 4 ) && g_aimbot.m_override_dmg ) {
		Indicator_t ind { };
		ind.color = 0xffffffff;
		ind.text = XOR( "DMG" );

		indicators.push_back( ind );
	}

	// BAIM
	if ( g_menu.main.visuals.indicators.get( 3 ) && g_aimbot.m_force_baim ) {
		Indicator_t ind { };
		ind.color = 0xffffffff;
		ind.text = XOR( "BAIM" );

		indicators.push_back( ind );
	}

	// force shot
	if ( g_menu.main.visuals.indicators.get( 5 )
		&& g_input_system.GetKeyState( g_menu.main.aimbot.force_shot.get( ) )
		&& g_cl.m_processing && g_cl.m_weapon ) {
		const float min_inaccuracy = g_inputpred.get_lowest_inaccuracy( g_cl.m_weapon );
		const float inaccuracy = g_cl.m_weapon->GetInaccuracy( );

		float ratio = 0.f;

		if ( min_inaccuracy > 0.f )
		{
			ratio = std::clamp(
				min_inaccuracy / inaccuracy,
				0.f, 1.f
			);
		}

		Color col = Color( 255, 0, 0 ).LerpRGB( 0xff15c27b, ratio );

		Indicator_t ind { };
		ind.color = col;
		ind.text = XOR( "FS" );

		indicators.push_back( ind );
	}

	if ( g_cl.m_throttled )
	{
		Indicator_t ind { };
		ind.color = Color( 255, 255, 0 );
		ind.text = XOR( "FPS" );

		indicators.push_back( ind );
	}

	if ( indicators.empty( ) )
		return;

	float modifier = g_cl.m_local->m_flVelocityModifier( );

	// iterate and draw indicators.
	for ( size_t i { }; i < indicators.size( ); ++i ) {
		auto& indicator = indicators[ i ];

		indicator.color.a( ) *= modifier;

		render::indicator.string( 20, g_cl.m_height - 80 - ( 30 * i ), indicator.color, indicator.text );
	}
}

void Visuals::SpreadCrosshair( )
{
	// dont do if dead.
	if ( !g_cl.m_processing )
		return;

	if ( !g_menu.main.visuals.spread_xhair.get( ) )
		return;

	// get active weapon.
	Weapon* weapon = g_cl.m_local->GetActiveWeapon( );
	if ( !weapon )
		return;

	WeaponInfo* data = weapon->GetWpnData( );
	if ( !data )
		return;

	// do not do this on: bomb, knife and nades.
	int type = weapon->m_iItemDefinitionIndex( );
	if ( type == WEAPONTYPE_KNIFE || type == WEAPONTYPE_C4 || type == WEAPONTYPE_GRENADE )
		return;

	// calc radius.
	float radius = ( ( weapon->GetInaccuracy( ) + weapon->GetSpread( ) ) * 320.f ) / ( std::tan( math::deg_to_rad( g_cl.m_local->GetFOV( ) ) * 0.5f ) + FLT_EPSILON );

	// scale by screen size.
	radius *= g_cl.m_height * ( 1.f / 480.f );

	// get color.
	Color col = g_menu.main.visuals.spread_xhair_col.get( );

	// modify alpha channel.
	col.a( ) = 200 * ( g_menu.main.visuals.spread_xhair_blend.get( ) / 100.f );

	int segements = std::max( 16, ( int )std::round( radius * 0.75f ) );
	render::circle( g_cl.m_width / 2, g_cl.m_height / 2, radius, segements, col );
}

void Visuals::ManualAntiAim( )
{
	if ( !g_cl.m_processing )
		return;

	if ( ( g_hvh.m_queued_dir < 0 && g_hvh.m_manual_direction < 0 && g_hvh.m_manual_lastmove < 0 ) || !g_menu.main.antiaim.manual_aa.get( ) )
		return;

	int x = g_cl.m_width / 2;
	int y = g_cl.m_height / 2;

	Color color = g_menu.main.antiaim.ind_color.get( );

	float multiplier = 0.90f;
	int size[ ] = { 65 * multiplier, 45 * multiplier, 10 * multiplier };

	Color lastmove_color = g_menu.main.antiaim.lm_ind_color.get( );

	if ( g_hvh.m_manual_lastmove != g_hvh.m_manual_direction )
	{
		switch ( g_hvh.m_manual_lastmove )
		{
		case ManualAAMode::BACK:
			render::triangle( vec2_t( x, y + size[ 0 ] ), vec2_t( x - size[ 2 ], y + size[ 1 ] ), vec2_t( x + size[ 2 ], y + size[ 1 ] ), lastmove_color );
			break;
		case ManualAAMode::RIGHT:
			render::triangle( vec2_t( x + size[ 0 ], y ), vec2_t( x + size[ 1 ], y + size[ 2 ] ), vec2_t( x + size[ 1 ], y - size[ 2 ] ), lastmove_color );
			break;
		case ManualAAMode::LEFT:
			render::triangle( vec2_t( x - size[ 0 ], y ), vec2_t( x - size[ 1 ], y - size[ 2 ] ), vec2_t( x - size[ 1 ], y + size[ 2 ] ), lastmove_color );
			break;
		case ManualAAMode::FORWARD:
			render::triangle( vec2_t( x, y - size[ 0 ] ), vec2_t( x + size[ 2 ], y - size[ 1 ] ), vec2_t( x - size[ 2 ], y - size[ 1 ] ), lastmove_color );
			break;
		}
	}

	switch ( g_hvh.m_manual_direction )
	{
	case ManualAAMode::BACK:
		render::triangle( vec2_t( x, y + size[ 0 ] ), vec2_t( x - size[ 2 ], y + size[ 1 ] ), vec2_t( x + size[ 2 ], y + size[ 1 ] ), color );
		break;
	case ManualAAMode::RIGHT:
		render::triangle( vec2_t( x + size[ 0 ], y ), vec2_t( x + size[ 1 ], y + size[ 2 ] ), vec2_t( x + size[ 1 ], y - size[ 2 ] ), color );
		break;
	case ManualAAMode::LEFT:
		render::triangle( vec2_t( x - size[ 0 ], y ), vec2_t( x - size[ 1 ], y - size[ 2 ] ), vec2_t( x - size[ 1 ], y + size[ 2 ] ), color );
		break;
	case ManualAAMode::FORWARD:
		render::triangle( vec2_t( x, y - size[ 0 ] ), vec2_t( x + size[ 2 ], y - size[ 1 ] ), vec2_t( x - size[ 2 ], y - size[ 1 ] ), color );
		break;
	}

	if ( g_hvh.m_queued_dir > 0 )
	{
		color.a( ) *= std::min( ( g_csgo.m_globals->m_realtime - g_hvh.m_last_press_time ) / game::TICKS_TO_TIME( 12 ), 1.f );

		switch ( g_hvh.m_queued_dir )
		{
		case ManualAAMode::BACK:
			render::triangle( vec2_t( x, y + size[ 0 ] ), vec2_t( x - size[ 2 ], y + size[ 1 ] ), vec2_t( x + size[ 2 ], y + size[ 1 ] ), color );
			break;
		case ManualAAMode::RIGHT:
			render::triangle( vec2_t( x + size[ 0 ], y ), vec2_t( x + size[ 1 ], y + size[ 2 ] ), vec2_t( x + size[ 1 ], y - size[ 2 ] ), color );
			break;
		case ManualAAMode::LEFT:
			render::triangle( vec2_t( x - size[ 0 ], y ), vec2_t( x - size[ 1 ], y - size[ 2 ] ), vec2_t( x - size[ 1 ], y + size[ 2 ] ), color );
			break;
		case ManualAAMode::FORWARD:
			render::triangle( vec2_t( x, y - size[ 0 ] ), vec2_t( x + size[ 2 ], y - size[ 1 ] ), vec2_t( x - size[ 2 ], y - size[ 1 ] ), color );
			break;
		}
	}
}

void Visuals::PenetrationCrosshair( )
{
	if ( !g_menu.main.visuals.pen_crosshair.get( ) || !g_cl.m_processing )
		return;

	int x = g_cl.m_width / 2;
	int y = g_cl.m_height / 2;

	Color final_color = colors::transparent_red;

	bool valid_player_hit = ( g_cl.m_pen_data.m_target && g_cl.m_pen_data.m_target->enemy( g_cl.m_local ) );
	if ( valid_player_hit )
		final_color = colors::transparent_yellow;

	else if ( g_cl.m_pen_data.m_pen )
		final_color = colors::transparent_green;

	render::rect( x - 1, y - 1, 3.f, 3.f, final_color );
}

void Visuals::draw( Entity* ent ) {
	if ( ent->IsPlayer( ) ) {
		Player* player = ent->as< Player* >( );

		// dont draw dead players.
		if ( !player->alive( ) )
			return;

		if ( player->m_bIsLocalPlayer( ) )
			return;

		if ( !player->enemy( g_cl.m_local ) && !g_menu.main.visuals.disableteam.get( ) )
			return;

		// draw player esp.
		DrawPlayer( player );
	}

	else if ( g_menu.main.visuals.items.get( ) && ent->IsBaseCombatWeapon( ) && !ent->dormant( ) )
		DrawItem( ent->as< Weapon* >( ) );

	else if ( g_menu.main.visuals.proj.get( ) )
		DrawProjectile( ent->as< Weapon* >( ) );
}

void Visuals::DrawProjectile( Weapon* ent ) {
	vec2_t screen;
	vec3_t origin = ent->GetAbsOrigin( );
	if ( !render::WorldToScreen( origin, screen ) )
		return;

	Color col = g_menu.main.visuals.proj_color.get( );
	col.a( ) = 0xb4;

	// draw decoy.
	if ( ent->is( HASH( "CDecoyProjectile" ) ) )
		render::esp_small.string( screen.x, screen.y, col, XOR( "DECOY" ), render::ALIGN_CENTER );

	// draw molotov.
	else if ( ent->is( HASH( "CMolotovProjectile" ) ) )
		render::esp_small.string( screen.x, screen.y, col, XOR( "MOLLY" ), render::ALIGN_CENTER );

	else if ( ent->is( HASH( "CBaseCSGrenadeProjectile" ) ) ) {
		const model_t* model = ent->GetModel( );

		if ( model ) {
			// grab modelname.
			std::string name { ent->GetModel( )->m_name };

			if ( name.find( XOR( "flashbang" ) ) != std::string::npos )
				render::esp_small.string( screen.x, screen.y, col, XOR( "FLASH" ), render::ALIGN_CENTER );

			else if ( name.find( XOR( "fraggrenade" ) ) != std::string::npos ) {

				// grenade range.
				if ( g_menu.main.visuals.proj_range.get( 0 ) )
					render::sphere( origin, 350.f, 5.f, 1.f, g_menu.main.visuals.proj_range_color.get( ) );

				render::esp_small.string( screen.x, screen.y, col, XOR( "FRAG" ), render::ALIGN_CENTER );
			}
		}
	}

	// find classes.
	else if ( ent->is( HASH( "CInferno" ) ) ) {
		// fire range.
		if ( g_menu.main.visuals.proj_range.get( 1 ) )
			render::sphere( origin, 150.f, 5.f, 1.f, g_menu.main.visuals.proj_range_color.get( ) );

		render::esp_small.string( screen.x, screen.y, col, XOR( "FIRE" ), render::ALIGN_CENTER );
	}

	else if ( ent->is( HASH( "CSmokeGrenadeProjectile" ) ) )
		render::esp_small.string( screen.x, screen.y, col, XOR( "SMOKE" ), render::ALIGN_CENTER );
}

void Visuals::DrawItem( Weapon* item ) {
	// we only want to draw shit without owner.
	Entity* owner = g_csgo.m_entlist->GetClientEntityFromHandle( item->m_hOwnerEntity( ) );
	if ( owner )
		return;

	// is the fucker even on the screen?
	vec2_t screen;
	vec3_t origin = item->GetAbsOrigin( );
	if ( !render::WorldToScreen( origin, screen ) )
		return;

	WeaponInfo* data = item->GetWpnData( );
	if ( !data )
		return;

	Color col = g_menu.main.visuals.item_color.get( );
	col.a( ) = 0xb4;

	// render bomb in green.
	if ( item->is( HASH( "CC4" ) ) )
		render::esp_small.string( screen.x, screen.y, { 150, 200, 60, 0xb4 }, XOR( "BOMB" ), render::ALIGN_CENTER );

	// if not bomb
	// normal item, get its name.
	else {
		std::string name { item->GetLocalizedName( ) };

		// smallfonts needs uppercase.
		std::transform( name.begin( ), name.end( ), name.begin( ), ::toupper );

		render::esp_small.string( screen.x, screen.y, col, name, render::ALIGN_CENTER );
	}

	if ( !g_menu.main.visuals.ammo.get( ) )
		return;

	// nades do not have ammo.
	if ( data->m_weapon_type == WEAPONTYPE_GRENADE || data->m_weapon_type == WEAPONTYPE_KNIFE )
		return;

	if ( item->m_iItemDefinitionIndex( ) == 0 || item->m_iItemDefinitionIndex( ) == C4 )
		return;

	std::string ammo = tfm::format( XOR( "(%i/%i)" ), item->m_iClip1( ), item->m_iPrimaryReserveAmmoCount( ) );
	render::esp_small.string( screen.x, screen.y - render::esp_small.m_size.m_height - 1, col, ammo, render::ALIGN_CENTER );
}

void Visuals::OffScreen( Player* player, float alpha )
{
	vec3_t view_origin, target_pos, delta;
	vec2_t screen_pos, offscreen_pos;
	float  leeway_x, leeway_y, radius, offscreen_rotation;
	bool   is_on_screen;
	Vertex verts[ 5 ];
	Color  color;

	// todo - dex; move this?
	static auto get_offscreen_data = [ ]( const vec3_t& delta, float radius, vec2_t& out_offscreen_pos, float& out_rotation )
		{
			ang_t  view_angles( g_csgo.m_view_render->m_view.m_angles );
			vec3_t fwd, right, up( 0.f, 0.f, 1.f );
			float  front, side, yaw_rad, sa, ca;

			// get viewport angles forward directional vector.
			math::AngleVectors( view_angles, &fwd );

			// convert viewangles forward directional vector to a unit vector.
			fwd.z = 0.f;
			fwd.normalize( );

			// calculate front / side positions.
			right = up.cross( fwd );
			front = delta.dot( fwd );
			side = delta.dot( right );

			// setup offscreen position.
			out_offscreen_pos.x = radius * -side;
			out_offscreen_pos.y = radius * -front;

			// get the rotation ( yaw, 0 - 360 ).
			out_rotation = math::rad_to_deg( std::atan2( out_offscreen_pos.x, out_offscreen_pos.y ) + math::pi );

			// get needed sine / cosine values.
			yaw_rad = math::deg_to_rad( -out_rotation );
			sa = std::sin( yaw_rad );
			ca = std::cos( yaw_rad );

			// rotate offscreen position around.
			out_offscreen_pos.x = ( int )( ( g_cl.m_width / 2.f ) + ( radius * sa ) );
			out_offscreen_pos.y = ( int )( ( g_cl.m_height / 2.f ) - ( radius * ca ) );
		};

	if ( !g_menu.main.players.offscreen.get( ) )
		return;

	if ( !g_cl.m_processing || !g_cl.m_local->enemy( player ) )
		return;

	// get the player's center screen position.
	target_pos = origin_cache[ player->index( ) - 1 ];
	is_on_screen = render::WorldToScreen( target_pos, screen_pos );

	// give some extra room for screen position to be off screen.
	leeway_x = g_cl.m_width / 18.f;
	leeway_y = g_cl.m_height / 18.f;

	// origin is not on the screen at all, get offscreen position data and start rendering.
	if ( !is_on_screen
		|| screen_pos.x < -leeway_x
		|| screen_pos.x >( g_cl.m_width + leeway_x )
		|| screen_pos.y < -leeway_y
		|| screen_pos.y >( g_cl.m_height + leeway_y ) )
	{
		float size = 12.f;
		float pos = ( g_cl.m_height * 0.15f ) * ( 65.f * 0.01f );

		// get viewport origin.
		view_origin = g_csgo.m_view_render->m_view.m_origin;

		// get direction to target.
		delta = ( target_pos - view_origin ).normalized( );

		// note - dex; this is the 'YRES' macro from the source sdk.
		radius = pos * ( g_cl.m_height / 480.f );

		// get the data we need for rendering.
		get_offscreen_data( delta, radius, offscreen_pos, offscreen_rotation );

		// bring rotation back into range... before rotating verts, sine and cosine needs this value inverted.
		// note - dex; reference:
		// https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/src_main/game/client/tf/tf_hud_damageindicator.cpp#L182
		offscreen_rotation = -offscreen_rotation;

		// setup triangle-shaped vertices (match render::triangle's shape)
		{
			// triangle control points (unrotated, relative to offscreen_pos)
			vec2_t point_one = { offscreen_pos.x, offscreen_pos.y - ( size * 2.f ) }; // tip / top
			vec2_t point_two = { offscreen_pos.x - size, offscreen_pos.y };          // left
			vec2_t point_three = { offscreen_pos.x + size, offscreen_pos.y };          // right

			// rotate each control point around the center
			Vertex r1 = render::RotateVertex( offscreen_pos, Vertex( point_one ), offscreen_rotation );
			Vertex r2 = render::RotateVertex( offscreen_pos, Vertex( point_two ), offscreen_rotation );
			Vertex r3 = render::RotateVertex( offscreen_pos, Vertex( point_three ), offscreen_rotation );

			// compute the intermediate verts exactly like render::triangle does
			vec2_t base = ( r2.m_pos + r3.m_pos ) / 2.f;
			vec2_t middle = ( r1.m_pos + r2.m_pos + r3.m_pos ) / 3.f;

			verts[ 0 ] = Vertex( ( middle + r1.m_pos * 4.f ) / 5.f );
			verts[ 1 ] = Vertex( r2.m_pos );
			verts[ 2 ] = Vertex( ( base + middle * 2.f ) / 3.f );
			verts[ 3 ] = Vertex( r3.m_pos );
			verts[ 4 ] = verts[ 0 ]; // keep original 5-vertex call compatible
		}

		// color + draw (unchanged)
		color = g_menu.main.players.offscreen_color.get( );
		color.a( ) = alpha;

		g_csgo.m_surface->DrawSetColor( color );
		g_csgo.m_surface->DrawTexturedPolygon( 5, verts );
	}
}

void Color::brighten( )
{
	float h, s, v;

	color_to_hsv( *this, h, s, v );

	v = 1.0f;

	Color brightened = hsv_to_rgb( h, s, v, a( ) );

	r( ) = brightened.r( );
	g( ) = brightened.g( );
	b( ) = brightened.b( );
}

render::Font get_font( render::Font default )
{
	if ( g_menu.main.players.default_fonts.get( ) )
	{
		return render::esp_alt;
	}

	return default;
}

void Visuals::DrawPlayer( Player* player )
{
	constexpr float MAX_DORMANT_TIME = 15.f;
	constexpr float DORMANT_FADE_TIME = 7.5f;
	constexpr float FADE_IN_TIME = 0.15f;         // seconds to reach full opacity

	player_info_t info;
	Rect          box;

	// get player index.
	int index = player->index( );

	// get reference to array variable.
	bool& draw = m_draw[ index - 1 ];
	float& opacity = m_opacities[ index - 1 ];

	// opacity should reach 1 in 100 milliseconds.
	constexpr int frequency = 1.f / FADE_IN_TIME;

	// the increment / decrement per frame.
	float step = frequency * g_csgo.m_globals->m_frametime;

	// is player enemy.
	bool enemy = player->enemy( g_cl.m_local );
	bool dormant = player->dormant( );

	if ( enemy && !dormant )
		player->m_bSpotted( ) = true;

	// we can draw this player again.
	if ( !dormant )
	{
		draw = true;
		origin_cache[ player->index( ) - 1 ] = player->GetAbsOrigin( );
		m_time_cache[ player->index( ) - 1 ] = g_csgo.m_globals->m_realtime;
	}

	if ( !draw )
	{
		opacity = 0.f;
		return;
	}

	dormant ? opacity -= step : opacity += step;

	// is dormant esp enabled for this player.
	bool dormant_esp = enemy && g_menu.main.players.dormant.get( );

	// clamp the opacity.
	math::clamp( opacity, 0.f, 1.f );
	if ( !opacity && !dormant_esp )
		return;

	// stay for x seconds max.
	float dt = g_csgo.m_globals->m_realtime - m_time_cache[ player->index( ) - 1 ];
	if ( dormant && dt > MAX_DORMANT_TIME )
		return;

	Color box_color = g_menu.main.players.box_enemy.get( );

	// calculate alpha channels.
	int alpha = ( int )( 255.f * opacity );
	int low_alpha = ( int )( 179.f * opacity );

	if ( dormant && dormant_esp )
	{
		alpha = 112.f;
		low_alpha = 80.f;

		if ( dt > DORMANT_FADE_TIME )
		{
			float faded = ( dt - DORMANT_FADE_TIME );
			float scale = 1.f - ( faded / DORMANT_FADE_TIME );

			alpha *= scale;
			low_alpha *= scale;
		}

		box_color = g_menu.main.players.dormant.get( );
	}

	box_color.a( ) = alpha;

	// get player info.
	if ( !g_csgo.m_engine->GetPlayerInfo( index, &info ) )
		return;

	// run offscreen ESP.
	OffScreen( player, alpha );

	if ( !GetPlayerBoxRect( player, box ) )
	{
		return;
	}

	auto aimplayer = &g_lagcompensation.m_players[ index - 1 ];
	auto LagCompensation = &g_lagcompensation.m_players[ index - 1 ];

	// player closest to our crosshair.
	//if ( index == g_aimbot.m_resolver_override_index && aimplayer && !aimplayer->resolver_debug_data.empty() )
	//{
	//	render::menu.string( 12, g_cl.m_height * 0.5f, colors::white, aimplayer->resolver_debug_data );
	//}

	DebugAimbotPoints( player );

	bool bone_esp = g_menu.main.players.skeleton.get( 0 );
	if ( bone_esp )
	{
		DrawSkeleton( player, g_menu.main.players.skeleton_enemy.get( ), opacity );
	}

	// first valid
	bool first_valid_bone = g_menu.main.players.skeleton.get( 2 );
	if ( first_valid_bone )
	{
		DrawHistorySkeleton( player, opacity, false );
	}

	// last valid
	bool last_valid_bone = g_menu.main.players.skeleton.get( 3 );
	if ( last_valid_bone )
	{
		DrawHistorySkeleton( player, opacity, true );
	}

	// is box esp enabled for this player.
	bool box_esp = g_menu.main.players.box.get( 0 );

	// render box if specified.
	if ( box_esp )
		render::rect_outlined( box.x, box.y, box.w, box.h, box_color, { 10, 10, 10, low_alpha } );

	// is name esp enabled for this player.
	bool name_esp = g_menu.main.players.name.get( 0 );

	// draw name.
	if ( name_esp )
	{
		// fix retards with their namechange meme
		// the point of this is overflowing unicode compares with hardcoded buffers, good hvh strat
		std::string name { std::string( info.m_name ).substr( 0, 24 ) };
		render::Font f = get_font( render::esp );

		if ( g_menu.main.players.default_fonts.get( ) )
			std::transform( name.begin( ), name.end( ), name.begin( ), ::tolower );

		Color clr = g_menu.main.players.name_color.get( );
		clr.a( ) *= alpha / 255.f;

		f.string( box.x + box.w / 2, box.y - f.m_size.m_height, clr, name, render::ALIGN_CENTER );

		//if ( aimplayer && aimplayer->m_override != 0.f )
		//{
		//	render::Font font = get_font( render::esp_small );

		//	font.string( box.x + box.w / 2, box.y - f.m_size.m_height - render::esp_small.m_size.m_height,
		//		aimplayer->m_override_agreed ? Color( 150, 200, 60, low_alpha ) : Color( 255, 0, 0, low_alpha ), XOR( "OVERRIDE" ), render::ALIGN_CENTER );
		//}
	}

	// is health esp enabled for this player.
	bool health_esp = ( enemy && g_menu.main.players.health.get( 0 ) ) || ( !enemy && g_menu.main.players.health.get( 1 ) );
	if ( health_esp )
	{
		int y = box.y + 1;
		int h = box.h - 2;

		// retarded servers that go above 100 hp..
		int hp = std::min( 100, player->m_iHealth( ) );

		// calculate hp bar color.
		int r = std::min( ( 510 * ( 100 - hp ) ) / 100, 255 );
		int g = std::min( ( 510 * hp ) / 100, 255 );

		// get hp bar height.
		int fill = ( int )std::round( hp * h / 100.f );

		// render background.
		render::rect_filled( box.x - 6, y - 1, 4, h + 2, { 10, 10, 10, low_alpha } );

		// render actual bar.
		render::rect( box.x - 5, y + h - fill, 2, fill, { r, g, 0, alpha } );

		// if hp is below max, draw a string.
		if ( hp < 100 ) {
			render::Font font = get_font( render::esp_small );

			font.string( box.x - 5, y + ( h - fill ) - 5, { 255, 255, 255, low_alpha }, std::to_string( hp ), render::ALIGN_CENTER );
		}
	}

	auto local_weapon = g_cl.m_local->GetActiveWeapon( );

	auto wpn_data = local_weapon ? local_weapon->GetWpnData( ) : nullptr;
	auto weapon = player->GetActiveWeapon( );
	WeaponInfo* data = nullptr;
	if ( weapon )
		data = weapon->GetWpnData( );

	const bool has_helmet = player->m_bHasHelmet( );
	const bool has_armor = player->m_ArmorValue( ) > 0;

	bool alternate = g_menu.main.players.default_fonts.get( );

	bool bot = game::IsFakePlayer( player->index( ) );

	// draw flags
	{
		std::vector<std::pair<std::string, Color>> flags;

		if ( bot )
			flags.push_back( { XOR( "BOT" ), Color( 150, 200, 60, low_alpha ) } );

		static auto cl_showerror = g_csgo.m_cvar->FindVar( HASH( "cl_showerror" ) );
		if ( cl_showerror->GetInt( ) > 2 )
		{
			const auto it = std::find_if(
				g_cl.m_sorted_players.begin( ),
				g_cl.m_sorted_players.end( ),
				[ &index ]( int this_index ) { return this_index == index; }
			);

			if ( it != g_cl.m_sorted_players.end( ) )
			{
				flags.push_back( { XOR( "TARGET" ), Color( 255, 0, 0, low_alpha ) } );
			}
		}

		auto items = g_menu.main.players.flags_enemy.GetActiveIndices( );

		for ( auto it = items.begin( ); it != items.end( ); ++it ) {
			switch ( *it ) {
			case 0: // armor
				if ( has_helmet || has_armor ) {
					std::string text;
					if ( has_helmet && has_armor ) text = XOR( "HK" );
					else if ( has_helmet )        text = XOR( "H" );
					else                          text = XOR( "K" );

					flags.emplace_back( XOR( text.c_str( ) ), Color { 255, 255, 255, low_alpha } );
				}
				break;

			case 1: // ping
				if ( g_cl.player_resource ) {
					auto ping = std::clamp( g_cl.player_resource->m_iPing( player->index( ) ), 0, 999 );
					if ( ping <= 150 ) {
						// green
						flags.push_back( { tfm::format( "%ims", ping ), {150, 200, 60, low_alpha} } );
					}
					else if ( ping <= 250 ) {
						// yellow/orange
						flags.push_back( { tfm::format( "%ims", ping ), {255, 120, 40, low_alpha} } );
					}
					else {
						// red
						flags.push_back( { tfm::format( "%ims", ping ), {255, 0, 0, low_alpha} } );
					}
				}
				break;
			case 2: // bomb
				if ( player->HasC4( ) )
					flags.push_back( { XOR( "BOMB" ), {255, 0, 0, low_alpha} } );
				break;

			case 3: // scoped
				if ( player->m_bIsScoped( ) )
					flags.push_back( { XOR( "ZOOM" ), {60, 180, 225, low_alpha} } );
				break;

			case 4: // flashed
				if ( player->m_flFlashBangTime( ) > 0.f )
					flags.push_back( { XOR( "FLASHED" ), {255, 255, 0, low_alpha} } );
				break;

			case 5: { // reloading
				C_AnimationLayer* layer1 = &player->m_AnimOverlay( )[ 1 ];
				if ( layer1->m_weight != 0.f && player->GetSequenceActivity( layer1->m_sequence ) == 967 ) // ACT_CSGO_RELOAD
				{
					flags.push_back( { XOR( "RELOAD" ), {60, 180, 225, low_alpha} } );
				}
				else {
					if ( weapon && data ) {
						const int ammo = weapon->m_iClip1( );
						int weapon_type = data->m_weapon_type;

						bool invalid_weapon = weapon_type >= -1 && ( weapon_type <= 0 || ( weapon_type > 6 && ( weapon_type <= 9 || weapon_type == 11 ) ) );

						if ( !invalid_weapon ) {

							if ( ammo == 0 ) {
								// NO AMMO -14186
								flags.push_back( { XOR( "NO AMMO" ), Color( 150, 200, 60, low_alpha ) } );
							}
							else if ( ammo == 1 ) {
								// LOW AMMO (1 bullet) -19396
								flags.push_back( { XOR( "LOW AMMO" ), Color( 60, 180, 225, low_alpha ) } );
							}
							else if ( ammo == 2 ) {
								// LOW AMMO (2 bullets) -1 (white)
								flags.push_back( { XOR( "LOW AMMO" ), Color( 255, 255, 255, low_alpha ) } );
							}
						}
					}
				}
			} break;
			case 6: // lagcomp / LC
				if ( g_cl.m_processing && g_cl.m_local ) {
					auto lc_text = alternate ? XOR( "LAG" ) : XOR( "LC" );

					if ( !aimplayer || aimplayer->m_records.empty( ) )
					{
						flags.push_back( { lc_text, {255, 0, 0, low_alpha} } );
					}
					else {
						if ( aimplayer->m_defensive )
							flags.push_back( { XOR( "DEFENSIVE" ), {255, 0, 0, low_alpha} } );
					}
				}
				break;

			case 7: // resolver confidence
				if ( LagCompensation ) {
					switch ( LagCompensation->m_resolver_confidence ) {
					case LagCompensation::ResolverConfidence_t::VERY_LOW:   flags.push_back( { XOR( "VLOW" ), {255, 0, 0, low_alpha} } ); break;
					case LagCompensation::ResolverConfidence_t::LOW:        flags.push_back( { XOR( "LOW" ), {255, 0, 0, low_alpha} } ); break;
					case LagCompensation::ResolverConfidence_t::MEDIUM:     flags.push_back( { XOR( "MED" ), {255, 255, 255, low_alpha} } ); break;
					case LagCompensation::ResolverConfidence_t::HIGH:       flags.push_back( { XOR( "HIGH" ), { 255, 255, 50, low_alpha} } ); break;
					case LagCompensation::ResolverConfidence_t::VERY_HIGH:  flags.push_back( { XOR( "VHIGH" ), {150, 200, 60, low_alpha} } ); break;
					case LagCompensation::ResolverConfidence_t::MAX:        flags.push_back( { XOR( "FULL" ), {150, 200, 60, low_alpha} } ); break;
					}
				}
				break;

			case 8: // twist correction / detected desync
				if ( aimplayer && aimplayer->m_detected_desync ) {
					float bro = abs( math::NormalizedAngle( aimplayer->m_max_feet_yaw - aimplayer->m_min_feet_yaw ) );
					flags.push_back( { XOR( "C" ), Color( 155, 200, 60, low_alpha ).LerpRGB( {255,0,0, low_alpha}, bro / 58.f ) } );
				}
				break;

			case 9: // lethal
				if ( aimplayer && g_cl.m_weapon_info ) {
					float health = player->m_iHealth( );
					bool lethal = penetration::scale( player, g_cl.m_weapon_info->m_damage, g_cl.m_weapon_info->m_armor_ratio, HITGROUP_STOMACH ) >= health;
					bool lethalx2 = penetration::scale( player, g_cl.m_weapon_info->m_damage, g_cl.m_weapon_info->m_armor_ratio, HITGROUP_STOMACH ) >= health * 1.25f;

					if ( health < 100 && ( lethal || lethalx2 ) ) {
						Color clr = lethalx2 ? Color( 150, 200, 60, low_alpha ) : Color( 255, 255, 0, low_alpha );
						flags.push_back( { XOR( "LETHAL" ), clr } );
					}
				}
				break;

			case 10: // distortion
				if ( LagCompensation && LagCompensation->m_resolver_confidence != LagCompensation::ResolverConfidence_t::MAX ) {
					if ( LagCompensation->m_979_count > 10 || LagCompensation->m_old_979_count > 10 ) {
						// severe distortion
						flags.push_back( { XOR( "DISTORTION" ), {255, 0, 0, low_alpha} } );
					}
					else if ( ( LagCompensation->m_979_count > 4 || LagCompensation->m_old_979_count > 4 ) ) {
						// minor distortion
						flags.push_back( { XOR( "DISTORTION" ), {255, 255, 255, low_alpha} } );
					}
				}
				break;
			}
		}

		render::Font font = get_font( render::esp_small );

		// draw all flags
		for ( size_t i = 0; i < flags.size( ); ++i ) {
			const auto& f = flags[ i ];

			int offset = i * ( render::esp_small.m_size.m_height - 1 );

			std::string text = f.first;
			if ( alternate )
				std::transform( text.begin( ), text.end( ), text.begin( ), ::tolower );

			font.string( box.x + box.w + 2, box.y + offset, f.second, text );
		}
	}

	// draw bottom bars.
	int  offset { 0 };

	// draw lby update bar.
	if ( enemy && g_menu.main.players.lby_update.get( ) )
	{
		// make sure everything is valid.
		if ( LagCompensation && !LagCompensation->m_cant_predict_lby && !LagCompensation->m_records.empty( ) )
		{
			// grab lag record.
			LagRecord* current = &LagCompensation->m_records.front( );

			if ( current && current->m_velocity.length_2d( ) <= 0.1f && ( current->m_flags & FL_ONGROUND ) )
			{
				// calculate box width
				float cycle = std::clamp( 1.f - ( ( LagCompensation->m_lby_timer - current->m_anim_time ) / 1.1f ), 0.f, 1.f );
				float width = box.w * cycle;

				if ( width >= 0.f )
				{
					Color clr = g_menu.main.players.lby_update_color.get( );
					clr.a( ) = alpha;

					render::rect_filled( box.x - 1, box.y + box.h + 2, box.w + 2, 4, { 10, 10, 10, low_alpha } );

					render::rect( box.x, box.y + box.h + 3, width, 2, clr );

					// move down the offset to make room for the next bar.
					offset += 6;
				}
			}
		}
	}

	// draw weapon.
	if ( g_menu.main.players.weapon.get( 0 ) )
	{
		if ( weapon && data )
		{
			render::Font font = get_font( render::esp_small );

			// text.
			if ( g_menu.main.players.weapon_mode.get( ) == 0 )
			{
				// construct std::string instance of localized weapon name.
				std::string name { weapon->GetLocalizedName( ) };

				std::transform( name.begin( ), name.end( ), name.begin( ), ::toupper );

				Color bro_got_bands = g_menu.main.players.weapon_color.get( );
				bro_got_bands.a( ) = alpha;

				if ( g_menu.main.players.default_fonts.get( ) )
					std::transform( name.begin( ), name.end( ), name.begin( ), ::tolower );

				font.string( box.x + box.w / 2, box.y + box.h + offset, bro_got_bands, name, render::ALIGN_CENTER );

				// MOVE DOWN FOR THE NEXT ICONS OKAY :D
				offset += font.m_size.m_height - 1;
			}

			// icons.
			if ( g_menu.main.players.weapon_mode.get( ) == 1 )
			{
				std::string icon = GetWeaponIcon( weapon->m_iItemDefinitionIndex( ) );

				// icons are a lil big...
				offset += 2;

				if ( !icon.empty( ) )
				{
					Color bro_got_bands = weapon->m_iItemDefinitionIndex( ) == ZEUS ? colors::transparent_yellow : g_menu.main.players.weapon_color.get( );
					bro_got_bands.a( ) = alpha;

					render::weapon_icons.string( box.x + box.w / 2, box.y + box.h + offset, bro_got_bands, icon, render::ALIGN_CENTER );
				}
			}
		}
	}
}

void Visuals::DrawPlantedC4( ) {
	bool        mode_2d, mode_3d, is_visible;
	float       explode_time_diff, dist, range_damage;
	vec3_t      dst, to_target;
	int         final_damage;
	std::string time_str, damage_str;
	Color       damage_color;
	vec2_t      screen_pos;

	static auto scale_damage = [ ]( float damage, int armor_value ) {
		float new_damage, armor;

		if ( armor_value > 0 ) {
			new_damage = damage * 0.5f;
			armor = ( damage - new_damage ) * 0.5f;

			if ( armor > ( float )armor_value ) {
				armor = ( float )armor_value * 2.f;
				new_damage = damage - armor;
			}

			damage = new_damage;
		}

		return std::max( 0, ( int )std::floor( damage ) );
		};

	// store menu vars for later.
	mode_2d = g_menu.main.visuals.planted_c4.get( 0 );
	mode_3d = g_menu.main.visuals.planted_c4.get( 1 );
	if ( !mode_2d && !mode_3d )
		return;

	// bomb not currently active, do nothing.
	if ( !m_c4_planted )
		return;

	// calculate bomb damage.
	// references:
	//     https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/se2007/game/shared/cstrike/weapon_c4.cpp#L271
	//     https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/se2007/game/shared/cstrike/weapon_c4.cpp#L437
	//     https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/game/shared/sdk/sdk_gamerules.cpp#L173
	{
		// get our distance to the bomb.
		// todo - dex; is dst right? might need to reverse CBasePlayer::BodyTarget...
		dst = g_cl.m_local->WorldSpaceCenter( );
		to_target = m_planted_c4_explosion_origin - dst;
		dist = to_target.length( );

		// calculate the bomb damage based on our distance to the C4's explosion.
		range_damage = m_planted_c4_damage * std::exp( ( dist * dist ) / ( ( m_planted_c4_radius_scaled * -2.f ) * m_planted_c4_radius_scaled ) );

		// now finally, scale the damage based on our armor (if we have any).
		final_damage = scale_damage( range_damage, g_cl.m_local->m_ArmorValue( ) );
	}

	// m_flC4Blow is set to gpGlobals->curtime + m_flTimerLength inside CPlantedC4.
	explode_time_diff = m_planted_c4_explode_time - g_csgo.m_globals->m_curtime;

	// get formatted strings for bomb.
	time_str = tfm::format( XOR( "%.2f" ), explode_time_diff );
	damage_str = tfm::format( XOR( "%i" ), final_damage );

	// get damage color.
	damage_color = ( final_damage < g_cl.m_local->m_iHealth( ) ) ? colors::white : colors::red;

	// finally do all of our rendering.
	is_visible = render::WorldToScreen( m_planted_c4_explosion_origin, screen_pos );

	// 'on screen (2D)'.
	if ( mode_2d ) {
		// g_cl.m_height - 80 - ( 30 * i )
		// 80 - ( 30 * 2 ) = 20

		// render::menu_shade.string( 60, g_cl.m_height - 20 - ( render::hud_size.m_height / 2 ), 0xff0000ff, "", render::hud );

		// todo - dex; move this next to indicators?

		if ( explode_time_diff > 0.f )
			render::esp.string( 2, 65, colors::white, time_str, render::ALIGN_LEFT );

		if ( g_cl.m_local->alive( ) )
			render::esp.string( 2, 65 + render::esp.m_size.m_height, damage_color, damage_str, render::ALIGN_LEFT );
	}

	// 'on bomb (3D)'.
	if ( mode_3d && is_visible ) {
		if ( explode_time_diff > 0.f )
			render::esp_small.string( screen_pos.x, screen_pos.y, colors::white, time_str, render::ALIGN_CENTER );

		// only render damage string if we're alive.
		if ( g_cl.m_local->alive( ) )
			render::esp_small.string( screen_pos.x, ( int )screen_pos.y + render::esp_small.m_size.m_height, damage_color, damage_str, render::ALIGN_CENTER );
	}
}

constexpr int linesize = 8, linedec = 4;

void Visuals::DrawHitmarkers( )
{
	constexpr float hit_duration = 1.f;

	if ( g_csgo.m_globals->m_curtime <= m_hit_start + hit_duration && g_menu.main.misc.hitmarker.get( ) )
	{
		float elapsed = g_csgo.m_globals->m_curtime - m_hit_start;

		Color col = colors::white;
		col.a( ) *= 1.f - elapsed;

		int x = g_cl.m_width / 2;
		int y = g_cl.m_height / 2;

		render::line( x - linesize, y - linesize, x - linedec, y - linedec, col );
		render::line( x - linesize, y + linesize, x - linedec, y + linedec, col );
		render::line( x + linesize, y + linesize, x + linedec, y + linedec, col );
		render::line( x + linesize, y - linesize, x + linedec, y - linedec, col );
	}

	bool world_enabled = g_menu.main.visuals.bullet_hitmarkers.get( );
	if ( !world_enabled )
		return g_shots.m_post_hitmarker_impacts.clear( );

	for ( int i = 0; i < g_shots.m_post_hitmarker_impacts.size( ); i++ )
	{
		HitmarkerImpacts* current = &g_shots.m_post_hitmarker_impacts[ i ];
		if ( !current )
			continue;

		if ( g_csgo.m_globals->m_curtime > current->m_time + hit_duration )
		{
			g_shots.m_post_hitmarker_impacts.erase( g_shots.m_post_hitmarker_impacts.begin( ) + i );
			continue;
		}

		vec2_t pos2D;
		if ( !render::WorldToScreen( current->m_pos, pos2D ) )
			continue;

		float elapsed = current->m_time + hit_duration - g_csgo.m_globals->m_curtime;

		if ( world_enabled )
		{
			Color col = colors::white;
			col.a( ) *= elapsed;

			render::line( pos2D.x + linesize, pos2D.y + linesize, pos2D.x + linedec, pos2D.y + linedec, col );
			render::line( pos2D.x - linesize, pos2D.y - linesize, pos2D.x - linedec, pos2D.y - linedec, col );
			render::line( pos2D.x + linesize, pos2D.y - linesize, pos2D.x + linedec, pos2D.y - linedec, col );
			render::line( pos2D.x - linesize, pos2D.y + linesize, pos2D.x - linedec, pos2D.y + linedec, col );
		}
	}
}

bool Visuals::GetPlayerBoxRect( Player* player, Rect& box )
{
	vec3_t origin, mins, maxs;
	vec2_t bottom, top;

	// get interpolated origin.
	origin = origin_cache[ player->index( ) - 1 ];

	// get hitbox bounds.
	player->ComputeHitboxSurroundingBox( &mins, &maxs );

	// correct x and y coordinates.
	mins = { origin.x, origin.y, mins.z };
	maxs = { origin.x, origin.y, maxs.z + 8.f };

	if ( !render::WorldToScreen( mins, bottom ) || !render::WorldToScreen( maxs, top ) )
		return false;

	box.h = bottom.y - top.y;
	box.w = box.h / 2.f;
	box.x = bottom.x - ( box.w / 2.f );
	box.y = bottom.y - box.h;

	return true;
}

void Visuals::DrawHistorySkeleton( Player* player, int opacity, bool last_valid ) {
	const model_t* model;
	studiohdr_t* hdr;
	mstudiobone_t* bone;
	int           parent;
	vec3_t        bone_pos, parent_pos;
	vec2_t        bone_pos_screen, parent_pos_screen;

	if ( !g_menu.main.misc.fake_latency.get( ) || !g_cl.m_local->alive( ) )
		return;

	// get player's model.
	model = player->GetModel( );
	if ( !model )
		return;

	// get studio model.
	hdr = g_csgo.m_model_info->GetStudioModel( model );
	if ( !hdr )
		return;

	auto data = &g_lagcompensation.m_players[ player->index( ) - 1 ];
	if ( !data || data->m_records.size( ) < 2 || !g_cl.m_local || !g_cl.m_local->alive( ) || !data->m_show_backtrack )
		return;

	LagRecord* current = nullptr;

	LagRecord* front = &data->m_records.front( );

	if ( !last_valid )
	{
		if ( front->valid( ) )
			return;
	}


	if ( last_valid ) {
		for ( int i = data->m_records.size( ) - 1; i >= 0; i-- )
		{
			LagRecord* record = &data->m_records[ i ];
			if ( !record )
				continue;

			if ( record->m_broke_lc )
				break;

			if ( !record->valid( ) )
				continue;

			current = record;
			break;
		}
	}
	else {
		for ( int i = 0; i < data->m_records.size( ); i++ )
		{
			LagRecord* record = &data->m_records[ i ];
			if ( !record )
				continue;

			if ( record->m_broke_lc )
				break;

			if ( !record->valid( ) )
				continue;

			current = record;
			break;
		}
	}

	if ( !current )
		return;

	constexpr float max_dist = 30.f;
	float alphamod = std::min( ( current->m_origin - player->GetAbsOrigin( ) ).length( ), max_dist ) / max_dist;
	Color clr = g_menu.main.players.skeleton_enemy.get( );

	clr.a( ) *= opacity;
	clr.a( ) *= alphamod;

	for ( int i { }; i < hdr->m_num_bones; ++i ) {
		// get bone.
		bone = hdr->GetBone( i );
		if ( !bone || !( bone->m_flags & BONE_USED_BY_HITBOX ) )
			continue;

		// get parent bone.
		parent = bone->m_parent;
		if ( parent == -1 )
			continue;

		// resolve main bone and parent bone positions.
		current->m_bones->get_bone( bone_pos, i );
		current->m_bones->get_bone( parent_pos, parent );

		// world to screen both the bone parent bone then draw.
		if ( render::WorldToScreen( bone_pos, bone_pos_screen ) && render::WorldToScreen( parent_pos, parent_pos_screen ) )
			render::line( bone_pos_screen.x, bone_pos_screen.y, parent_pos_screen.x, parent_pos_screen.y, clr );
	}
}

void Visuals::DrawSkeleton( Player* player, Color color, int opacity )
{
	const model_t* model;
	studiohdr_t* hdr;
	mstudiobone_t* bone;
	int           parent;
	vec3_t        bone_pos, parent_pos;
	vec2_t        bone_pos_screen, parent_pos_screen;

	// get player's model.
	model = player->GetModel( );
	if ( !model )
		return;

	// get studio model.
	hdr = g_csgo.m_model_info->GetStudioModel( model );
	if ( !hdr )
		return;

	if ( player->dormant( ) )
		return;

	BoneArray* matrix = player->m_BoneCache( ).m_pCachedBones;

	color.a( ) *= opacity;

	for ( int i { }; i < hdr->m_num_bones; ++i )
	{
		// get bone.
		bone = hdr->GetBone( i );
		if ( !bone || !( bone->m_flags & BONE_USED_BY_HITBOX ) )
			continue;

		// get parent bone.
		parent = bone->m_parent;
		if ( parent == -1 )
			continue;

		// resolve main bone and parent bone positions.
		matrix->get_bone( bone_pos, i );
		matrix->get_bone( parent_pos, parent );

		// world to screen both the bone parent bone then draw.
		if ( render::WorldToScreen( bone_pos, bone_pos_screen ) && render::WorldToScreen( parent_pos, parent_pos_screen ) )
			render::line( bone_pos_screen.x, bone_pos_screen.y, parent_pos_screen.x, parent_pos_screen.y, color );
	}
}

void Visuals::RenderGlow( )
{
	Color   color;
	int style = -1.f;

	if ( !g_cl.m_local )
		return;

	if ( !g_csgo.m_glow->m_object_definitions.Count( ) )
		return;

	static bool ran = false;
	if ( !ran ) {
		IMaterial* pMatTeamIdShape = g_csgo.m_material_system->FindMaterial( XOR( "dev/glow_edge_highlight" ), TEXTURE_GROUP_OTHER );
		IMaterial* pmat2 = g_csgo.m_material_system->FindMaterial( XOR( "dev/glow_color" ), TEXTURE_GROUP_OTHER );
		IMaterial* pmat_weirdhealthboxthing = g_csgo.m_material_system->FindMaterial( XOR( "dev/glow_health_color" ), TEXTURE_GROUP_OTHER ); // box that appears on players being hurt

		pMatTeamIdShape->SetFlag( MATERIAL_VAR_IGNOREZ, true );
		pmat2->SetFlag( MATERIAL_VAR_IGNOREZ, true );
		pmat_weirdhealthboxthing->SetFlag( MATERIAL_VAR_NO_DRAW, true );
		ran = true;
	}

	for ( int i { }; i < g_csgo.m_glow->m_object_definitions.Count( ); ++i )
	{
		GlowObjectDefinition_t* obj = &g_csgo.m_glow->m_object_definitions[ i ];
		// skip non-players.
		if ( obj->unused( ) || !obj->m_pEntity )
			continue;

		auto class_id = obj->m_pEntity->GetClientClass( )->m_ClassID;

		if ( class_id == _CCSPlayer )
		{
			// get player ptr.
			Player* player = obj->m_pEntity->as< Player* >( );

			// get reference to array variable.
			float& opacity = m_opacities[ player->index( ) - 1 ];

			bool enemy = player->enemy( g_cl.m_local );

			if ( player->m_bIsLocalPlayer( ) && !g_menu.main.players.glow.get( 2 ) )
				continue;

			if ( enemy && !g_menu.main.players.glow.get( 0 ) )
				continue;

			if ( !enemy && !g_menu.main.players.glow.get( 1 ) )
				continue;

			// enemy color
			if ( enemy ) {
				bool hittable = g_menu.main.players.glow.get( 3 ) && g_aimbot.m_players[ player->index( ) - 1 ].m_target;

				color = g_menu.main.players.glow_enemy.get( );
				if ( hittable )
					color = g_menu.main.players.glow_threat.get( );
			}
			else if ( player->m_bIsLocalPlayer( ) ) {
				color = g_menu.main.players.glow_local.get( );
			}
			// friendly color
			else {
				color = g_menu.main.players.glow_friendly.get( );
			}

			// lol meme disgusting meme
			if ( g_menu.main.players.glow.get( 4 ) ) style = 0;
			if ( g_menu.main.players.glow.get( 5 ) ) style = 1;
			if ( g_menu.main.players.glow.get( 6 ) ) style = 2;
			if ( g_menu.main.players.glow.get( 7 ) ) style = 3;
			if ( g_menu.main.players.glow.get( 8 ) ) style = 4;
			if ( g_menu.main.players.glow.get( 9 ) ) style = 5;

			if ( style < 0 )
				return;

			obj->m_nRenderStyle = style < 1 ? 0 : style - 1;
			obj->m_bRenderWhenOccluded = true;
			obj->m_bRenderWhenUnoccluded = false;
			obj->m_bFullBloomRender = style == 0;
			obj->m_vGlowColor = { ( float )color.r( ) / 255.f, ( float )color.g( ) / 255.f, ( float )color.b( ) / 255.f };
			obj->m_flGlowAlpha = ( opacity * color.a( ) ) / 255.f;
		}
	}
}

void Visuals::DrawHitboxMatrix( Player* player, BoneArray* bones, Color col, float time )
{
	const model_t* model;
	studiohdr_t* hdr;
	mstudiohitboxset_t* set;
	mstudiobbox_t* bbox;
	vec3_t             mins, maxs, origin;
	ang_t			   angle;

	model = player->GetModel( );
	if ( !model )
		return;

	hdr = g_csgo.m_model_info->GetStudioModel( model );
	if ( !hdr )
		return;

	set = hdr->GetHitboxSet( player->m_nHitboxSet( ) );
	if ( !set )
		return;

	for ( int i { }; i < set->m_hitboxes; ++i )
	{
		bbox = set->GetHitbox( i );
		if ( !bbox )
			continue;

		// bbox.
		if ( bbox->m_radius <= 0.f )
		{
			// https://developer.valvesoftware.com/wiki/Rotation_Tutorial

			// convert rotation angle to a matrix.
			matrix3x4_t rot_matrix;
			g_csgo.AngleMatrix( bbox->m_angle, rot_matrix );

			// apply the rotation to the entity input space (local).
			matrix3x4_t matrix;
			math::ConcatTransforms( bones[ bbox->m_bone ], rot_matrix, matrix );

			// extract the compound rotation as an angle.
			ang_t bbox_angle;
			math::MatrixAngles( matrix, bbox_angle );

			// extract hitbox origin.
			vec3_t origin = matrix.GetOrigin( );

			// draw box.
			g_csgo.m_debug_overlay->AddBoxOverlay( origin, bbox->m_mins, bbox->m_maxs, bbox_angle, col.r( ), col.g( ), col.b( ), col.a( ), time );
		}

		// capsule.
		else
		{
			// NOTE; the angle for capsules is always 0.f, 0.f, 0.f.

			// create a rotation matrix.
			matrix3x4_t matrix;
			g_csgo.AngleMatrix( bbox->m_angle, matrix );

			// apply the rotation matrix to the entity output space (world).
			math::ConcatTransforms( bones[ bbox->m_bone ], matrix, matrix );

			// get world positions from new matrix.
			math::VectorTransform( bbox->m_mins, matrix, mins );
			math::VectorTransform( bbox->m_maxs, matrix, maxs );

			g_csgo.m_debug_overlay->AddCapsuleOverlay( mins, maxs, bbox->m_radius, col.r( ), col.g( ), col.b( ), col.a( ), time, 0, 1 );
		}
	}
}

void Visuals::DrawBeams( )
{
	size_t     impact_count;
	float      curtime, dist;
	bool       is_final_impact;
	vec3_t     va_fwd, start, dir, end;
	BeamInfo_t beam_info;
	Beam_t* beam;

	auto vis_impacts = &g_shots.m_vis_impacts;
	impact_count = vis_impacts->size( );

	if ( !impact_count )
		return;

	curtime = g_csgo.m_globals->m_curtime;

	for ( size_t i { impact_count }; i-- > 0; )
	{
		auto impact = &vis_impacts->operator[ ]( i );
		if ( !impact )
			continue;

		// impact is too old, erase it.
		if ( std::abs( curtime - impact->m_tickbase ) > g_menu.main.visuals.impact_beams_time.get( ) )
		{
			vis_impacts->erase( vis_impacts->begin( ) + i );
			continue;
		}

		// already rendering this impact, skip over it.
		if ( impact->m_invalid )
			continue;

		// is this the final impact?
		// last impact in the vector, it's the final impact.
		if ( i == ( impact_count - 1 ) )
			is_final_impact = true;

		// the current impact's tickbase is different than the next, it's the final impact.
		else if ( ( i + 1 ) < impact_count && impact->m_tickbase != vis_impacts->operator[ ]( i + 1 ).m_tickbase )
			is_final_impact = true;

		else
			is_final_impact = false;

		// is this the final impact?
		// is_final_impact = ( ( i == ( impact_count - 1 ) ) || ( impact->m_tickbase != vis_impacts->at( i + 1 ).m_tickbase ) );

		if ( is_final_impact )
		{
			// calculate start and end position for beam.
			start = impact->m_shoot_pos;

			dir = ( impact->m_impact_pos - start ).normalized( );
			dist = ( impact->m_impact_pos - start ).length( );

			end = start + ( dir * dist );

			beam_info.m_vecStart = start;
			beam_info.m_vecEnd = end;
			beam_info.m_nModelIndex = g_csgo.m_model_info->GetModelIndex( XOR( "sprites/purplelaser1.vmt" ) );
			beam_info.m_pszModelName = XOR( "sprites/purplelaser1.vmt" );
			beam_info.m_flHaloScale = 0.f;
			beam_info.m_flLife = g_menu.main.visuals.impact_beams_time.get( );
			beam_info.m_flWidth = 3.f;
			beam_info.m_flEndWidth = 3.f;
			beam_info.m_flFadeLength = 1.f;
			beam_info.m_flAmplitude = 0.f;   // beam 'jitter'.
			beam_info.m_flBrightness = impact->m_color.a( );
			beam_info.m_flSpeed = 0.5f;  // seems to control how fast the 'scrolling' of beam is... once fully spawned.
			beam_info.m_nStartFrame = 0;
			beam_info.m_flFrameRate = 0.f;
			beam_info.m_nSegments = 2;     // controls how much of the beam is 'split up', usually makes m_flAmplitude and m_flSpeed much more noticeable.
			beam_info.m_bRenderable = true;  // must be true or you won't see the beam.
			beam_info.m_nFlags = 0;//FBEAM_FADEIN | FBEAM_FADEOUT; // fadeout makes it look like bnunguzz

			beam_info.m_flRed = impact->m_color.r( );
			beam_info.m_flGreen = impact->m_color.g( );
			beam_info.m_flBlue = impact->m_color.b( );

			beam = game::CreateGenericBeam( beam_info );
			if ( beam )
			{
				g_csgo.m_beams->DrawBeam( beam );
				impact->m_invalid = true;
			}
		}
	}
}

void Visuals::DebugAimbotPoints( Player* player )
{
	if ( !g_cl.m_processing )
		return;

	bool render_text = g_menu.main.players.skeleton.get( 6 );
	bool render_points = g_menu.main.players.skeleton.get( 4 );
	if ( !render_text && !render_points )
		return;

	auto data = &g_lagcompensation.m_players[ player->index( ) - 1 ];
	if ( !data || data->m_records.empty( ) )
		return;

	LagRecord* front = &data->m_records.front( );
	if ( !front )
		return;

	std::vector<AimPoint_t> points;

	AimPlayer* aim_data = &g_aimbot.m_players[ player->index( ) - 1 ];
	if ( !aim_data || !aim_data->m_entity )
		return;

	if ( !aim_data->SetupHitboxPoints( front, points, true ) )
		return;

	for ( auto& p : points )
	{
		vec2_t screen;

		if ( !render::WorldToScreen( p.m_pos, screen ) )
			continue;

		if ( render_points )
			render::rect_filled( screen.x, screen.y, 4, 4, p.m_first_pass ? Color { 255, 0, 0, 255 } : Color { 0, 0, 255, 255 } );

		if ( render_text )
			render::esp.string( screen.x, screen.y, Color( 255, 255, 255, 175 ), tfm::format( "%.2f", p.m_fake_amt ) );
	}
}

void Visuals::ExtendedDormancy( )
{
	if ( !g_cl.m_local )
		return;

	for ( auto i : g_cl.m_unsorted_players )
	{
		auto player = g_csgo.m_entlist->GetClientEntity<Player*>( i );
		if ( !player || player->alive( ) || player->dormant( ) )
			continue;

		//if ( g_ExtendedVisuals.m_arrOverridePlayers.at( i ).m_eOverrideType == EOverrideType::ESP_SHARED )
		//	continue;

		// this player is spectating someone
		if ( player->m_hObserverTarget( ).IsValid( ) )
		{
			// grab the player who is being spectated (pSpectatee) by the spectator (player)
			const auto pSpectatee = ( Player* )player->m_hObserverTarget( ).Get( );
			if ( pSpectatee && pSpectatee->index( ) <= 64 )
			{
				// if the guy that's being spectated is dormant, set his origin
				// to the spectators origin (and ofc set dormancy state for esp to draw him)
				if ( pSpectatee->dormant( ) )
				{
					if ( player->m_iObserverMode( ) == 4 || player->m_iObserverMode( ) == 5 )
					{
						//m_arrOverridePlayers.at( pSpectatee->EntIndex() ).m_eOverrideType = EOverrideType::ESP_SPECTATOR;
						origin_cache[ pSpectatee->index( ) - 1 ] = player->m_vecOrigin( );
						m_time_cache[ pSpectatee->index( ) - 1 ] = g_csgo.m_globals->m_realtime;
						player->m_bSpotted( ) = true;
					}
				}
			}
		}
	}
}