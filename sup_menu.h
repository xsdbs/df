#pragma once

class AimbotTab : public Tab {
public:
	// col1.
	Checkbox	  enable;
	Checkbox      silent_aim;
	Checkbox	  auto_fire;
	Checkbox      separate_pointscale;
	MultiDropdown hitbox;
	MultiDropdown multipoint;
	Dropdown	  multipoint_strength;

	Slider        aim_height;
	Slider		  head_scale;
	Slider		  body_scale;
	Slider		  limbs_scale;
	Checkbox	  dynamic_scale;
	Slider		  minimal_damage;

	MultiDropdown resolve_antiaim;
	MultiDropdown miscellaneous;
	MultiDropdown optimizations;

	// col 2
	Slider		  hitchance;
	Dropdown      hitchance_strength;
	Slider        prefer_body_over_damage;
	Slider		  prefer_hitchance;
	Slider		  prefer_fake_body_overlap;
	Slider        point_scale_air_limiter;
	Slider		  prefer_fake_head_overlap;
	Slider		  minimal_assesed_damage;
	MultiDropdown automatic_stop;
	Checkbox      between_shots;
	Checkbox      increase_fire_rate;

	Dropdown      delay_shot_body;
	Dropdown      delay_shot_head;

	Dropdown      prefer_backtrack;
	Dropdown	  tab_selection;
	Keybind		  force_body_aim;
	Keybind       force_shot;
	Keybind		  resolver_override;
	Keybind		  minimal_damage_override;
	Slider		  smugs_black_leg;

public:
	void init( ) {
		// title.
		SetTitle( XOR( "aimbot" ) );

		enable.setup( XOR( "enable" ), XOR( "enable" ) );
		RegisterElement( &enable );

		silent_aim.setup( XOR( "silent aim" ), XOR( "silent_aim" ) );
		RegisterElement( &silent_aim );

		auto_fire.setup( XOR( "auto-fire" ), XOR( "auto_fire" ) );
		RegisterElement( &auto_fire );

		hitbox.setup(
			XOR( "hitbox" ),
			XOR( "hitbox" ),
			{ XOR( "head" ), XOR( "chest" ), XOR( "body" ), XOR( "arms" ), XOR( "thighs" ), XOR( "calfs" ), XOR( "feet" ) }
		);
		RegisterElement( &hitbox );

		multipoint.setup(
			XOR( "multi-point" ),
			XOR( "multipoint" ),
			{ XOR( "head" ), XOR( "chest" ), XOR( "body" ), XOR( "arms" ), XOR( "thighs" ), XOR( "calfs" ), XOR( "feet" ) }
		);
		RegisterElement( &multipoint );

		multipoint_strength.setup( XOR( "" ), XOR( "multipoint_strength" ), { XOR( "low" ), XOR( "medium" ), XOR( "high" ) }, false );
		RegisterElement( &multipoint_strength );

		body_scale.setup( "", XOR( "body_scale" ), 1.f, 100.f, false, 0, 90.f, 1.f, XOR( L"%" ) );
		body_scale.AddShowCallback( callbacks::IsMultipointOn );
		RegisterElement( &body_scale );

		aim_height.setup( "maximum aim-height", XOR( "aim_height" ), 20.f, 100.f, true, 0, 50.f, 1.f, XOR( L"%" ) );
		aim_height.AddShowCallback( callbacks::IsMultipointOn );
		RegisterElement( &aim_height );

		dynamic_scale.setup( XOR( "dynamic point-scale" ), XOR( "dynamic_scale" ) );
		RegisterElement( &dynamic_scale );

		separate_pointscale.setup( XOR( "separate point-scale" ), XOR( "separate_pointscale" ) );
		RegisterElement( &separate_pointscale );

		head_scale.setup( XOR( "head hitbox scale" ), XOR( "body_hitbox_scale" ), 1.f, 100.f, true, 0, 50.f, 1.f, XOR( L"%" ) );
		head_scale.AddShowCallback( callbacks::IsMultipointBodyOn );
		head_scale.AddShowCallback( callbacks::IsSeparateScaleOn );
		RegisterElement( &head_scale );

		limbs_scale.setup( XOR( "limb hitbox scale" ), XOR( "limbs_hitbox_scale" ), 1.f, 100.f, true, 0, 50.f, 1.f, XOR( L"%" ) );
		limbs_scale.AddShowCallback( callbacks::CanApplyLimbsScale );
		RegisterElement( &limbs_scale );

		minimal_damage.setup( XOR( "minimal damage" ), XOR( "minimal_damage" ), 1.f, 100.f, true, 0, 30.f, 1.f, XOR( L"hp" ) );
		RegisterElement( &minimal_damage );

		resolve_antiaim.setup( XOR( "resolve anti-aim" ), XOR( "resolve_antiaim" ), { XOR( "fake-angles" ) , XOR( "desync correction" ) } );
		RegisterElement( &resolve_antiaim );

		miscellaneous.setup( XOR( "aimbot flags" ), XOR( "misc" ), { XOR( "non-strict force body aim" ), XOR( "headshot only" ) } );
		RegisterElement( &miscellaneous );

		optimizations.setup( XOR( "optimizations" ), XOR( "optimizations" ), { XOR( "reduce targets per tick" ),
			XOR( "limit scan intensity" ),
			XOR( "reduce backtrack targets" ) } );
		RegisterElement( &optimizations );

		hitchance.setup( XOR( "minimal hitchance" ), XOR( "hitchance" ), 1.f, 100.f, true, 0, 65.f, 1.f, XOR( L"%" ) );
		RegisterElement( &hitchance, 1 );

		hitchance_strength.setup( XOR( "" ), XOR( "hitchance_strength" ), { XOR( "low" ) , XOR( "medium" ), XOR( "high" ), XOR( "very high" ) }, false );
		RegisterElement( &hitchance_strength, 1 );

		prefer_hitchance.setup( XOR( "prefer hitchance bias" ), XOR( "prefer_hitchance" ), -100.f, 100.f, true, 0, 50.f, 1.f, XOR( L"%" ) );
		RegisterElement( &prefer_hitchance, 1 );

		prefer_body_over_damage.setup( XOR( "prefer body bias" ), XOR( "prefer_body_over_damage" ), 0.f, 100.f, true, 0, 0.f, 1.f, XOR( L"%" ) );
		RegisterElement( &prefer_body_over_damage, 1 );

		minimal_assesed_damage.setup( XOR( "minimal assessed damage" ), XOR( "minimal_assesed_damage" ), 1.f, 100.f, true, 0, 5.f, 1.f, XOR( L"hp" ) );
		RegisterElement( &minimal_assesed_damage, 1 );

		automatic_stop.setup( XOR( "automatic stop" ), XOR( "automatic_stop" ), { XOR( "early" ), XOR( "stop" ), XOR( "jump-scout" ), XOR( "fall-back" ), XOR( "delay for accuracy" ), XOR( "auto scope" ) } );
		RegisterElement( &automatic_stop, 1 );

		between_shots.setup( XOR( "between shots" ), XOR( "automatic_stop_between_shots" ) );
		between_shots.AddShowCallback( callbacks::EarlyInactive );
		RegisterElement( &between_shots, 1 );

		tab_selection.setup( XOR( "accuracy" ), XOR( "tab_selection" ), { XOR( "lag compensation" ), XOR( "override aimbot" ), XOR( "override accuracy" ) } );
		RegisterElement( &tab_selection, 1 );

		delay_shot_body.setup( XOR( "delay for body on peek" ), XOR( "delay_shot_body" ), {
			XOR( "off" ), XOR( "low" ), XOR( "medium" ), XOR( "high" ), XOR( "very high" )
			} );
		delay_shot_body.AddShowCallback( callbacks::IsAccuracyTabOn );
		RegisterElement( &delay_shot_body, 1 );

		delay_shot_head.setup( XOR( "delay for head on peek" ), XOR( "delay_shot_head" ), {
			XOR( "off" ), XOR( "low" ), XOR( "medium" ), XOR( "high" ), XOR( "very high" )
			} );
		delay_shot_head.AddShowCallback( callbacks::IsAccuracyTabOn );
		RegisterElement( &delay_shot_head, 1 );

		increase_fire_rate.setup( XOR( "increase fire-rate" ), XOR( "increase_fire_rate" ) );
		increase_fire_rate.AddShowCallback( callbacks::IsAccuracyTabOn );
		RegisterElement( &increase_fire_rate, 1 );

		point_scale_air_limiter.setup( XOR( "limit air point-scale" ), XOR( "point_scale_air_limiter" ), 0.f, 100.f, true, 0, 75.f, 1.f, XOR( L"%" ) );
		point_scale_air_limiter.AddShowCallback( callbacks::IsLagcompTabOn );
		RegisterElement( &point_scale_air_limiter, 1 );

		prefer_fake_body_overlap.setup( XOR( "prefer overlap over fake body" ), XOR( "prefer_fake_body_overlap" ), 0.f, 100.f, true, 0, 30.f, 1.f, XOR( L"%" ) );
		prefer_fake_body_overlap.AddShowCallback( callbacks::IsLagcompTabOn );
		RegisterElement( &prefer_fake_body_overlap, 1 );

		prefer_fake_head_overlap.setup( XOR( "prefer overlap over fake head" ), XOR( "prefer_fake_head_overlap" ), 0.f, 100.f, true, 0, 30.f, 1.f, XOR( L"%" ) );
		prefer_fake_head_overlap.AddShowCallback( callbacks::IsLagcompTabOn );
		RegisterElement( &prefer_fake_head_overlap, 1 );

		force_body_aim.setup( XOR( "force body aim" ), XOR( "force_body_aim" ) );
		force_body_aim.AddShowCallback( callbacks::IsBindsTabOn );
		force_body_aim.SetToggleCallback( callbacks::ToggleForceBodyAim );
		RegisterElement( &force_body_aim, 1 );

		resolver_override.setup( XOR( "resolver override" ), XOR( "resolver_override" ) );
		resolver_override.AddShowCallback( callbacks::IsBindsTabOn );
		RegisterElement( &resolver_override, 1 );

		force_shot.setup( XOR( "force shot" ), XOR( "force_shot" ) );
		force_shot.AddShowCallback( callbacks::IsBindsTabOn );
		RegisterElement( &force_shot, 1 );

		minimal_damage_override.setup( XOR( "minimal damage override" ), XOR( "minimal_damage_override" ) );
		minimal_damage_override.AddShowCallback( callbacks::IsBindsTabOn );
		minimal_damage_override.SetToggleCallback( callbacks::ToggleDmgOverride );
		RegisterElement( &minimal_damage_override, 1 );

		smugs_black_leg.setup( XOR( " " ), XOR( "smugs_black_leg" ), 1.f, 100.f, false, 0, 1.f, 1.f, XOR( L"hp" ) );
		smugs_black_leg.AddShowCallback( callbacks::IsBindsTabOn );
		smugs_black_leg.AddShowCallback( callbacks::IsOverrideDamageOn );
		RegisterElement( &smugs_black_leg, 1 );
	}
};

class AntiAimTab : public Tab {
public:
	// general anti-aim
	Checkbox      instant_yaw_update;
	Checkbox      enable;
	Dropdown      pitch;
	Dropdown      base_yaw;
	Dropdown      angle_preference;

	// lby settings
	Dropdown      lby_orientation;
	Dropdown      lby_side;
	Dropdown      lby_breaker_type;
	Slider        lby_limit;
	MultiDropdown fake_body_flags;

	// desync / twist
	MultiDropdown twist_move;
	Slider        minimum_desync;
	Slider        lock_threshold;

	// fakelag
	Checkbox      lag_enable;
	MultiDropdown lag_active;
	Dropdown      lag_type;
	Slider        lag_limit;

	// manual anti-aim
	Checkbox      manual_aa;
	Colorpicker   lm_ind_color;
	Colorpicker   ind_color;
	Keybind       manual_front;
	Keybind       manual_back;
	Keybind       manual_left;
	Keybind       manual_right;
	Checkbox      manual_lastmoving;

	// misc
	MultiDropdown misc;
	Dropdown      leg_movement;
	Keybind		  activate_desync;

public:
	void init( ) {
		SetTitle( XOR( "anti-aim" ) );

		// general anti-aim
		enable.setup( XOR( "enable" ), XOR( "enable" ) );
		RegisterElement( &enable );

		pitch.setup( XOR( "pitch" ), XOR( "pitch" ), { XOR( "down" ), XOR( "up" ), XOR( "zero" ) } );
		RegisterElement( &pitch );

		base_yaw.setup( XOR( "base yaw" ), XOR( "base_yaw" ), { XOR( "freestand" ), XOR( "backwards" ) } );
		RegisterElement( &base_yaw );

		angle_preference.setup( XOR( "angle preference" ), XOR( "angle_preference" ), { XOR( "closest" ), XOR( "furthest" ) } );
		angle_preference.AddShowCallback( callbacks::AnglePreference );
		RegisterElement( &angle_preference );

		// lby
		lby_orientation.setup( XOR( "lby side" ), XOR( "lby_orientation" ), { XOR( "peek" ), XOR( "hide" ) } );
		RegisterElement( &lby_orientation );

		lby_side.setup( XOR( "lby angle" ), XOR( "lby_side" ), { XOR( "safest" ), XOR( "furthest" ) } );
		RegisterElement( &lby_side );

		lby_breaker_type.setup( XOR( "breaker type" ), XOR( "lby_breaker_type" ), { XOR( "force 979" ), XOR( "suppress 979" ) }, true );
		RegisterElement( &lby_breaker_type );

		fake_body_flags.setup( XOR( "fake body flags" ), XOR( "fake_body_flags" ), { XOR( "adaptive lby" ), XOR( "extend last moving" ) } );
		RegisterElement( &fake_body_flags );

		// desync / twist
		twist_move.setup( XOR( "move twist" ), XOR( "twist_move" ), { XOR( "enable" ), XOR( "delay move" ), XOR( "disable fakelag" ), XOR( "tilt yaw towards freestand" ) } );
		RegisterElement( &twist_move );

		minimum_desync.setup( "", XOR( "minimum_desync" ), 1.f, 50.f, false, 0, 15.f, 1.f, XOR( L"°" ) );
		RegisterElement( &minimum_desync );

		lock_threshold.setup( "", XOR( "lock_threshold" ), 1.f, 50.f, false, 0, 30.f, 1.f, XOR( L"°" ) );
		RegisterElement( &lock_threshold );

		// fakelag
		lag_enable.setup( XOR( "fake-lag" ), XOR( "lag_enable" ) );
		RegisterElement( &lag_enable, 1 );

		lag_active.setup( "", XOR( "lag_active" ), { XOR( "stand" ), XOR( "move" ), XOR( "air" ), XOR( "crouch" ), XOR( "hold" ) }, false );
		RegisterElement( &lag_active, 1 );

		lag_type.setup( "", XOR( "lag_type" ), { XOR( "default" ), XOR( "adaptive" ) }, false );
		RegisterElement( &lag_type, 1 );

		lag_limit.setup( XOR( " " ), XOR( "lag_limit" ), 2, 14, false, 0, 2, 1.f );
		RegisterElement( &lag_limit, 1 );

		lby_limit.setup( XOR( "lby lag limit" ), XOR( "lby_limit" ), 2, 14, true, 0, 2, 1.f );
		RegisterElement( &lby_limit, 1 );

		// manual anti-aim
		manual_aa.setup( XOR( "manual aa" ), XOR( "manual_aa" ) );
		RegisterElement( &manual_aa, 1 );

		ind_color.setup( XOR( "color" ), XOR( "ind_color" ), colors::burgundy );
		RegisterElement( &ind_color, 1 );

		manual_front.setup( XOR( "manual front" ), XOR( "manual_front" ) );
		RegisterElement( &manual_front, 1 );

		manual_back.setup( XOR( "manual back" ), XOR( "manual_back" ) );
		RegisterElement( &manual_back, 1 );

		manual_left.setup( XOR( "manual left" ), XOR( "manual_left" ) );
		RegisterElement( &manual_left, 1 );

		manual_right.setup( XOR( "manual right" ), XOR( "manual_right" ) );
		RegisterElement( &manual_right, 1 );

		manual_lastmoving.setup( XOR( "double press to set last-moving" ), XOR( "manual_lastmoving" ) );
		RegisterElement( &manual_lastmoving );

		lm_ind_color.setup( XOR( "color" ), XOR( "lm_ind_color" ), colors::black );
		lm_ind_color.AddShowCallback( callbacks::IsFakeLastMoveOn );
		RegisterElement( &lm_ind_color );

		// misc
		misc.setup( XOR( "anti-aim flags" ), XOR( "misc" ), {
			XOR( "at targets" ),
			XOR( "prefer backwards on duck" ),
			XOR( "adaptive in air" ),
			XOR( "enable on round end" ),
			XOR( "enable on freezetime" ),
			XOR( "enable in use" )
			} );
		RegisterElement( &misc, 1 );

		leg_movement.setup( XOR( "leg movement" ), XOR( "leg_movement" ), { XOR( "off" ), XOR( "slide" ), XOR( "never slide" ) } );
		RegisterElement( &leg_movement, 1 );

		instant_yaw_update.setup( XOR( "instant yaw update" ), XOR( "instant_yaw_update" ) );
		RegisterElement( &instant_yaw_update, 1 );

		activate_desync.setup( XOR( "enable desync" ), XOR( "activate_desync" ) );
		activate_desync.SetToggleCallback( callbacks::ToggleDesync );
		RegisterElement( &activate_desync, 1 );
	}
};

class PlayersTab : public Tab {
public:
	Checkbox      default_fonts;
	MultiDropdown box;
	Colorpicker   box_enemy;
	Colorpicker   box_friendly;
	Checkbox      dormant;
	Checkbox      offscreen;
	MultiDropdown name;
	Colorpicker   name_color;
	Colorpicker   offscreen_color;
	MultiDropdown health;
	MultiDropdown flags_enemy;
	MultiDropdown flags_friendly;
	MultiDropdown weapon;
	Dropdown      weapon_mode;
	Colorpicker   weapon_color;
	Checkbox      ammo;
	Colorpicker   ammo_color;
	Checkbox      lby_update;
	Colorpicker   lby_update_color;

	// col2.
	MultiDropdown skeleton;
	Colorpicker   aimbot_matrix;
	Colorpicker   skeleton_enemy;
	Colorpicker   skeleton_friendly;

	MultiDropdown glow;
	Colorpicker   glow_threat;
	Colorpicker   glow_enemy;
	Colorpicker   glow_friendly;
	Colorpicker   glow_local;
	MultiDropdown chams_enemy;
	Slider		  chams_enemy_phong;
	Colorpicker   chams_enemy_vis;
	Colorpicker   chams_enemy_threat;
	Colorpicker   chams_enemy_invis;
	Colorpicker   chams_metallic_enemy_color;
	Colorpicker   chams_enemy_history_col;
	MultiDropdown chams_friendly;
	Slider		  chams_friendly_phong;
	Colorpicker   chams_friendly_vis;
	Colorpicker   chams_friendly_invis;
	Colorpicker   chams_metallic_friendly_color;
public:
	void init( ) {
		SetTitle( XOR( "players" ) );

		default_fonts.setup( XOR( "default fonts" ), XOR( "default_fonts" ) );
		RegisterElement( &default_fonts );

		box.setup( XOR( "boxes" ), XOR( "box" ), { XOR( "enemy" ), XOR( "friendly" ) } );
		RegisterElement( &box );

		box_enemy.setup( XOR( "box enemy color" ), XOR( "box_enemy" ), { 150, 200, 60 } );
		RegisterElement( &box_enemy );

		box_friendly.setup( XOR( "box friendly color" ), XOR( "box_friendly" ), { 255, 200, 0 } );
		RegisterElement( &box_friendly );

		dormant.setup( XOR( "dormant enemies" ), XOR( "dormant" ) );
		RegisterElement( &dormant );

		offscreen.setup( XOR( "enemy offscreen esp" ), XOR( "offscreen" ) );
		RegisterElement( &offscreen );

		name.setup( XOR( "name" ), XOR( "name" ), { XOR( "enemy" ), XOR( "friendly" ) } );
		RegisterElement( &name );

		name_color.setup( XOR( "name color" ), XOR( "name_color" ), { 255, 255, 255, 175 } );
		RegisterElement( &name_color );

		offscreen_color.setup( XOR( "offscreen esp color" ), XOR( "offscreen_color" ), colors::white );
		RegisterElement( &offscreen_color );

		health.setup( XOR( "health" ), XOR( "health" ), { XOR( "enemy" ), XOR( "friendly" ) } );
		RegisterElement( &health );

		flags_enemy.setup( XOR( "flags enemy" ), XOR( "flags_enemy" ), { XOR( "armor" ),XOR( "ping" ),XOR( "bomb" ),XOR( "scoped" ),XOR( "flashed" ),XOR( "reloading" ), XOR( "lagcomp" ),XOR( "resolver confidence" ),XOR( "twist correction" ),XOR( "lethal" ),XOR( "distortion" ) } );
		RegisterElement( &flags_enemy );

		flags_friendly.setup( XOR( "flags friendly" ), XOR( "flags_friendly" ), { XOR( "armor" ), XOR( "scoped" ), XOR( "flashed" ), XOR( "reloading" ), XOR( "bomb" ) } );
		RegisterElement( &flags_friendly );

		weapon.setup( XOR( "weapon" ), XOR( "weapon" ), { XOR( "enemy" ), XOR( "friendly" ) } );
		RegisterElement( &weapon );

		weapon_mode.setup( "", XOR( "weapon_mode" ), { XOR( "text" ), XOR( "icon" ) }, false );
		RegisterElement( &weapon_mode );

		weapon_color.setup( XOR( "color" ), XOR( "weapon_color" ), colors::white );
		RegisterElement( &weapon_color );

		lby_update.setup( XOR( "lby update" ), XOR( "lby_update" ) );
		RegisterElement( &lby_update );

		lby_update_color.setup( XOR( "color" ), XOR( "lby_update_color" ), colors::purple );
		RegisterElement( &lby_update_color );

		// col2.
		skeleton.setup( XOR( "skeleton" ), XOR( "skeleton" ), { XOR( "enemy" ), XOR( "friendly" ),
			XOR( "first valid" ), XOR( "last valid" ), XOR( "visualize points" ), XOR( "aimbot" ), XOR( "visualize fake points" ) } );
		RegisterElement( &skeleton, 1 );

		aimbot_matrix.setup( XOR( "aimbot matrix color" ), XOR( "aimbot_matrix" ), { 255, 255, 255 } );
		aimbot_matrix.AddShowCallback( callbacks::IsVisualizeAimbotMatrixOn );
		RegisterElement( &aimbot_matrix, 1 );

		skeleton_enemy.setup( XOR( "enemy color" ), XOR( "skeleton_enemy" ), { 255, 255, 255 } );
		RegisterElement( &skeleton_enemy, 1 );

		skeleton_friendly.setup( XOR( "friendly color" ), XOR( "skeleton_friendly" ), { 255, 255, 255 } );
		RegisterElement( &skeleton_friendly, 1 );

		glow.setup( XOR( "glow" ), XOR( "glow" ), { XOR( "enemy" ), XOR( "friendly" ), XOR( "local" ), XOR( "threat" ), XOR( "blur" ), XOR( "blur outline" ), XOR( "model" ), XOR( "edge" ), XOR( "pulse" ) } );
		RegisterElement( &glow, 1 );

		glow_threat.setup( XOR( "threat color" ), XOR( "glow_threat" ), { 255, 0, 0 } );
		RegisterElement( &glow_threat, 1 );

		glow_enemy.setup( XOR( "enemy color" ), XOR( "glow_enemy" ), { 150, 200, 60 } );
		RegisterElement( &glow_enemy, 1 );

		glow_friendly.setup( XOR( "friendly color" ), XOR( "glow_friendly" ), { 150, 200, 60 } );
		RegisterElement( &glow_friendly, 1 );

		glow_local.setup( XOR( "local color" ), XOR( "glow_local" ), colors::transparent_red );
		RegisterElement( &glow_local, 1 );

		chams_enemy.setup( XOR( "enemy chams" ), XOR( "chams_enemy" ), { XOR( "visible" ), XOR( "invisible" ), XOR( "history" ), XOR( "retrack" ), XOR( "threat" ), XOR( "textured" ), XOR( "flat" ), XOR( "metallic" ) } );
		RegisterElement( &chams_enemy, 1 );

		chams_enemy_phong.setup( XOR( "" ), XOR( "chams_enemy_phong" ), 1.f, 255.f, false, 0, 100.f, 1.f, XOR( L"%" ) );
		chams_enemy_phong.AddShowCallback( callbacks::MetallicChamsEnemy );
		RegisterElement( &chams_enemy_phong, 1 );

		chams_enemy_vis.setup( XOR( "color visible" ), XOR( "chams_enemy_vis" ), { 150, 200, 60 } );
		RegisterElement( &chams_enemy_vis, 1 );

		chams_enemy_invis.setup( XOR( "color invisible" ), XOR( "chams_enemy_invis" ), { 60, 180, 225 } );
		RegisterElement( &chams_enemy_invis, 1 );

		chams_enemy_threat.setup( XOR( "color threat" ), XOR( "chams_enemy_threat" ), { 255, 255, 0 } );
		RegisterElement( &chams_enemy_threat, 1 );

		chams_metallic_enemy_color.setup( XOR( "metallic color" ), XOR( "chams_metallic_enemy_color" ), { 241, 255, 46 } );
		chams_metallic_enemy_color.AddShowCallback( callbacks::MetallicChamsEnemy );
		RegisterElement( &chams_metallic_enemy_color, 1 );

		chams_enemy_history_col.setup( XOR( "history color" ), XOR( "chams_history_col" ), { 255, 255, 200 } );
		RegisterElement( &chams_enemy_history_col, 1 );

		chams_friendly.setup( XOR( "friendly chams" ), XOR( "chams_friendly" ), { XOR( "visible" ), XOR( "invisible" ), XOR( "textured" ), XOR( "flat" ), XOR( "metallic" ) } );
		RegisterElement( &chams_friendly, 1 );

		chams_friendly_phong.setup( XOR( "" ), XOR( "chams_friendly_phong" ), 1.f, 255.f, false, 0, 100.f, 1.f, XOR( L"%" ) );
		chams_friendly_phong.AddShowCallback( callbacks::MetallicChamsFriendly );
		RegisterElement( &chams_friendly_phong, 1 );

		chams_friendly_vis.setup( XOR( "color visible" ), XOR( "chams_friendly_vis" ), { 255, 200, 0 } );
		RegisterElement( &chams_friendly_vis, 1 );

		chams_friendly_invis.setup( XOR( "color invisible" ), XOR( "chams_friendly_invis" ), { 255, 50, 0 } );
		RegisterElement( &chams_friendly_invis, 1 );

		chams_metallic_friendly_color.setup( XOR( "metallic color" ), XOR( "chams_metallic_friendly_color" ), { 241, 255, 46 } );
		chams_metallic_friendly_color.AddShowCallback( callbacks::MetallicChamsFriendly );
		RegisterElement( &chams_metallic_friendly_color, 1 );
	}
};

class VisualsTab : public Tab {
public:
	Checkbox      items;
	Checkbox      ammo;
	Colorpicker   item_color;
	Checkbox      proj;
	Colorpicker   proj_color;
	MultiDropdown proj_range;
	Colorpicker   proj_range_color;
	MultiDropdown planted_c4;
	Checkbox      disableteam;
	MultiDropdown world;
	Checkbox      transparent_props;
	Checkbox      enemy_radar;
	Checkbox      visualize_lby;
	Colorpicker   chams_lby_local_color;

	Slider        chams_local_scope_blend;
	MultiDropdown chams_local;
	Slider chams_local_phong;
	Colorpicker   chams_local_color;
	Colorpicker   chams_metallic_local_color;

	// col2.
	Checkbox      novisrecoil;
	Checkbox      nosmoke;
	Checkbox      nofog;
	Checkbox      noflash;
	Checkbox      noscope;
	Checkbox      fov;
	Slider        fov_amt;
	Slider        fov_scoped_amt;
	Checkbox      viewmodel_fov;
	Slider        viewmodel_fov_amt;
	Checkbox      spectators;
	Checkbox      force_xhair;
	Checkbox      spread_xhair;
	Colorpicker   spread_xhair_col;
	Slider        spread_xhair_blend;
	Checkbox      pen_crosshair;
	MultiDropdown indicators;
	Checkbox      tracers;
	Checkbox      bullet_impacts;
	Checkbox      bullet_hitmarkers;
	Checkbox      impact_beams;
	Colorpicker   impact_beams_color;
	Colorpicker   impact_beams_hurt_color;
	Slider        impact_beams_time;
	Keybind       thirdperson;
	Slider        thirdperson_dist;

public:
	void init( ) {
		SetTitle( XOR( "visuals" ) );

		items.setup( XOR( "dropped weapons" ), XOR( "items" ) );
		RegisterElement( &items );

		ammo.setup( XOR( "dropped weapons ammo" ), XOR( "ammo" ) );
		RegisterElement( &ammo );

		item_color.setup( XOR( "color" ), XOR( "item_color" ), colors::white );
		RegisterElement( &item_color );

		proj.setup( XOR( "projectiles" ), XOR( "proj" ) );
		RegisterElement( &proj );

		proj_color.setup( XOR( "color" ), XOR( "proj_color" ), colors::white );
		RegisterElement( &proj_color );

		proj_range.setup( XOR( "projectile range" ), XOR( "proj_range" ), { XOR( "frag" ), XOR( "molly" ) } );
		RegisterElement( &proj_range );

		proj_range_color.setup( XOR( "color" ), XOR( "proj_range_color" ), colors::burgundy );
		RegisterElement( &proj_range_color );

		planted_c4.setup( XOR( "planted c4" ), XOR( "planted_c4" ), { XOR( "on screen (2D)" ), XOR( "on bomb (3D)" ) } );
		RegisterElement( &planted_c4 );

		disableteam.setup( XOR( "do not render teammates" ), XOR( "disableteam" ) );
		RegisterElement( &disableteam );

		enemy_radar.setup( XOR( "force enemies on radar" ), XOR( "enemy_radar" ) );
		RegisterElement( &enemy_radar );

		spectators.setup( XOR( "show spectator list" ), XOR( "spectators" ) );
		RegisterElement( &spectators );

		world.setup( XOR( "world" ), XOR( "world" ), { XOR( "night" ), XOR( "fullbright" ) } );
		world.SetCallback( Visuals::ModulateWorld );
		RegisterElement( &world );

		transparent_props.setup( XOR( "transparent props" ), XOR( "transparent_props" ) );
		transparent_props.SetCallback( Visuals::ModulateWorld );
		RegisterElement( &transparent_props );

		chams_local.setup( XOR( "local chams" ), XOR( "chams_local" ), { XOR( "visible" ), XOR( "textured" ), XOR( "flat" ), XOR( "metallic" ) } );
		RegisterElement( &chams_local );

		chams_local_scope_blend.setup( "", XOR( "chams_local_scope_blend" ), 0.f, 100.f, false, 0, 50.f, 1.f, XOR( L"%" ) );
		RegisterElement( &chams_local_scope_blend );

		chams_local_phong.setup( XOR( "" ), XOR( "chams_local_phong" ), 1.f, 255.f, false, 0, 100.f, 1.f, XOR( L"%" ) );
		chams_local_phong.AddShowCallback( callbacks::MetallicChamsLocal );
		RegisterElement( &chams_local_phong );

		chams_local_color.setup( XOR( "color visible" ), XOR( "chams_local_color" ), { 255, 255, 255 } );
		RegisterElement( &chams_local_color );

		chams_metallic_local_color.setup( XOR( "metallic color" ), XOR( "chams_metallic_local_color" ), { 241, 255, 46 } );
		chams_metallic_local_color.AddShowCallback( callbacks::MetallicChamsLocal );
		RegisterElement( &chams_metallic_local_color );

		visualize_lby.setup( XOR( "visualize lby" ), XOR( "visualize_lby" ) );
		RegisterElement( &visualize_lby );

		chams_lby_local_color.setup( XOR( "lby color" ), XOR( "chams_lby_local_color" ), colors::burgundy );
		RegisterElement( &chams_lby_local_color );

		// col2.
		novisrecoil.setup( XOR( "remove visual recoil" ), XOR( "novisrecoil" ) );
		RegisterElement( &novisrecoil, 1 );

		nosmoke.setup( XOR( "remove smoke grenades" ), XOR( "nosmoke" ) );
		RegisterElement( &nosmoke, 1 );

		nofog.setup( XOR( "remove fog" ), XOR( "nofog" ) );
		RegisterElement( &nofog, 1 );

		noflash.setup( XOR( "remove flashbangs" ), XOR( "noflash" ) );
		RegisterElement( &noflash, 1 );

		noscope.setup( XOR( "remove scope" ), XOR( "noscope" ) );
		RegisterElement( &noscope, 1 );

		fov.setup( XOR( "override fov" ), XOR( "fov" ) );
		RegisterElement( &fov, 1 );

		fov_amt.setup( "", XOR( "fov_amt" ), 60.f, 140.f, false, 0, 90.f, 1.f, XOR( L"°" ) );
		RegisterElement( &fov_amt, 1 );

		fov_scoped_amt.setup( "override scoped fov", XOR( "fov_scoped_amt" ), 0.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		RegisterElement( &fov_scoped_amt, 1 );

		viewmodel_fov.setup( XOR( "override viewmodel fov" ), XOR( "viewmodel_fov" ) );
		RegisterElement( &viewmodel_fov, 1 );

		viewmodel_fov_amt.setup( "", XOR( "viewmodel_fov_amt" ), 60.f, 140.f, false, 0, 74.f, 1.f, XOR( L"°" ) );
		RegisterElement( &viewmodel_fov_amt, 1 );

		force_xhair.setup( XOR( "force crosshair" ), XOR( "force_xhair" ) );
		RegisterElement( &force_xhair, 1 );

		spread_xhair.setup( XOR( "visualize spread" ), XOR( "spread_xhair" ) );
		RegisterElement( &spread_xhair, 1 );

		spread_xhair_col.setup( XOR( "visualize spread color" ), XOR( "spread_xhair_col" ), colors::burgundy );
		RegisterElement( &spread_xhair_col, 1 );

		spread_xhair_blend.setup( "", XOR( "spread_xhair_blend" ), 10.f, 100.f, false, 0, 100.f, 1.f, XOR( L"%" ) );
		RegisterElement( &spread_xhair_blend, 1 );

		pen_crosshair.setup( XOR( "penetration crosshair" ), XOR( "pen_xhair" ) );
		RegisterElement( &pen_crosshair, 1 );

		indicators.setup( XOR( "indicators" ), XOR( "indicators" ), { XOR( "lby" ), XOR( "lag compensation" ), XOR( "fake latency" ), XOR( "override baim" ), XOR( "override dmg" ), XOR( "force shot" ) } );
		RegisterElement( &indicators, 1 );

		tracers.setup( XOR( "grenade simulation" ), XOR( "tracers" ) );
		RegisterElement( &tracers, 1 );

		bullet_hitmarkers.setup( XOR( "bullet hitmarkers" ), XOR( "bullet_hitmarkers" ) );
		RegisterElement( &bullet_hitmarkers, 1 );

		bullet_impacts.setup( XOR( "bullet impacts" ), XOR( "bullet_impacts" ) );
		RegisterElement( &bullet_impacts, 1 );

		impact_beams.setup( XOR( "bullet impact beams" ), XOR( "impact_beams" ) );
		RegisterElement( &impact_beams, 1 );

		impact_beams_color.setup( XOR( "impact beams color" ), XOR( "impact_beams_color" ), colors::light_blue );
		RegisterElement( &impact_beams_color, 1 );

		impact_beams_hurt_color.setup( XOR( "impact beams hurt color" ), XOR( "impact_beams_hurt_color" ), colors::red );
		RegisterElement( &impact_beams_hurt_color, 1 );

		impact_beams_time.setup( XOR( "impact beams time" ), XOR( "impact_beams_time" ), 1.f, 10.f, true, 0, 1.f, 1.f, XOR( L"s" ) );
		RegisterElement( &impact_beams_time, 1 );

		thirdperson.setup( XOR( "thirdperson" ), XOR( "thirdperson" ) );
		thirdperson.SetToggleCallback( callbacks::ToggleThirdPerson );
		RegisterElement( &thirdperson, 1 );

		thirdperson_dist.setup( "", XOR( "thirdperson_dist" ), 0.f, 150.f, false, 0, 90.f, 1.f, XOR( L"%" ) );
		RegisterElement( &thirdperson_dist, 1 );

	}
};

class MovementTab : public Tab {
public:
	Checkbox bhop;
	Checkbox airduck;
	Checkbox quickswitch_on_autopeek;
	Checkbox autostrafe;
	Slider   autostrafe_smoothing;


	Keybind  fakewalk;
	Slider max_shot_delay;
	Keybind  autopeek;
	Keybind  edge_jump;

public:
	void init( ) {
		SetTitle( XOR( "movement" ) );

		bhop.setup( XOR( "automatic jump" ), XOR( "bhop" ) );
		RegisterElement( &bhop );

		airduck.setup( XOR( "duck in air" ), XOR( "airduck" ) );
		RegisterElement( &airduck );

		autostrafe.setup( XOR( "auto-strafe" ), XOR( "autostrafe" ) );
		RegisterElement( &autostrafe );

		autostrafe_smoothing.setup( "", XOR( "autostrafe_smoothing" ), 1.f, 100.f, false, 0, 35.f, 1.f, XOR( L"%" ) );
		RegisterElement( &autostrafe_smoothing );

		fakewalk.setup( XOR( "fake-walk" ), XOR( "fakewalk" ) );
		RegisterElement( &fakewalk, 1 );

		max_shot_delay.setup( "", XOR( "max_shot_delay" ), 0.f, 8.f, false, 0, 0.f, 1.f, XOR( L"t" ) );
		RegisterElement( &max_shot_delay, 1 );

		autopeek.setup( XOR( "automatic peek" ), XOR( "autopeek" ) );
		RegisterElement( &autopeek, 1 );

		quickswitch_on_autopeek.setup( XOR( "switch on shot" ), XOR( "quickswitch_on_autopeek" ) );
		RegisterElement( &quickswitch_on_autopeek, 1 );

		edge_jump.setup( XOR( "edge-jump" ), XOR( "edge_jump" ) );
		RegisterElement( &edge_jump, 1 );
	}
};

class MiscTab : public Tab {
public:
	// col1.
	Checkbox autobuy;
	Dropdown buy1;
	Dropdown buy2;
	MultiDropdown buy3;
	MultiDropdown notifications;
	Keybind   last_tick_defuse;
	Checkbox fake_latency;
	Slider	fake_latency_amt;

	Keybind       alt_fake_latency;
	Slider		  alt_fake_latency_amt;

	// col 2
	Checkbox increase_responsiveness;
	Checkbox hitmarker;
	Checkbox hitsound;
	MultiDropdown model_smoothing;
	Checkbox preserve_killfeed;
	Checkbox ragdoll_force;
	Checkbox override_aspect_ratio;
	Slider	 aspect_ratio;
public:
	void init( ) {
		SetTitle( XOR( "misc" ) );

		autobuy.setup( XOR( "autobuy" ), XOR( "autobuy" ) );
		RegisterElement( &autobuy );

		// col 1
		buy1.setup( XOR( "auto buy items" ), XOR( "auto_buy1" ),
			{
				XOR( "awp" ),
				XOR( "auto-sniper" ),
				XOR( "scout" ),
			} );
		RegisterElement( &buy1 );

		buy2.setup( "", XOR( "auto_buy2" ),
			{
				XOR( "none" ),
				XOR( "deage / r8" ),
			}, false );
		RegisterElement( &buy2 );

		buy3.setup( "", XOR( "auto_buy3" ),
			{
				XOR( "nades" ),
				XOR( "armor" ),
				XOR( "taser" ),
				XOR( "ignore on pistol round" ),
			}, false );
		RegisterElement( &buy3 );

		last_tick_defuse.setup( XOR( "last tick defuse" ), XOR( "last_tick_defuse" ) );
		RegisterElement( &last_tick_defuse );

		notifications.setup( XOR( "notifications" ), XOR( "notifications" ), { XOR( "damage" ), XOR( "harmed" ), XOR( "purchases" ), XOR( "bomb" ), XOR( "defuse" ) } );
		RegisterElement( &notifications );

		fake_latency.setup( XOR( "fake latency" ), XOR( "fake_latency" ) );
		RegisterElement( &fake_latency );

		fake_latency_amt.setup( "", XOR( "fake_latency_amt" ), 50.f, 800.f, false, 0, 200.f, 10.f, XOR( L"ms" ) );
		RegisterElement( &fake_latency_amt );

		alt_fake_latency.setup( XOR( "secondary fake latency" ), XOR( "alt_fake_latency" ) );
		alt_fake_latency.SetToggleCallback( callbacks::ToggleFakeLatency );
		RegisterElement( &alt_fake_latency );

		alt_fake_latency_amt.setup( "", XOR( "alt_fake_latency_amt" ), 50.f, 800.f, false, 0, 400.f, 10.f, XOR( L"ms" ) );
		RegisterElement( &alt_fake_latency_amt );

		// col 2
		model_smoothing.setup( XOR( "model smoothing" ), XOR( "model_smoothing" ), { XOR( "force foot plant" ), XOR( "remove model sway" ), XOR( "smooth animations" ) } );
		RegisterElement( &model_smoothing, 1 );

		preserve_killfeed.setup( XOR( "preserve killfeed" ), XOR( "preserve_killfeed" ) );
		RegisterElement( &preserve_killfeed, 1 );

		increase_responsiveness.setup( XOR( "increase hit-effects" ), XOR( "increase_responsiveness" ) );
		RegisterElement( &increase_responsiveness, 1 );

		hitmarker.setup( XOR( "hitmarker" ), XOR( "hitmarker" ) );
		RegisterElement( &hitmarker, 1 );

		hitsound.setup( XOR( "hitsound" ), XOR( "hitsound" ) );
		RegisterElement( &hitsound, 1 );

		ragdoll_force.setup( XOR( "ragdoll force" ), XOR( "ragdoll_force" ) );
		RegisterElement( &ragdoll_force, 1 );

		override_aspect_ratio.setup( XOR( "override aspect ratio" ), XOR( "override_aspect_ratio" ) );
		RegisterElement( &override_aspect_ratio, 1 );

		aspect_ratio.setup( XOR( " " ), XOR( "aspect_ratio" ), 0.f, 200.f, false, 0, 0.f, 1.f, XOR( L"%" ) );
		aspect_ratio.AddShowCallback( callbacks::IsAspectRatioOn );
		RegisterElement( &aspect_ratio, 1 );
	}
};

class SkinsTab : public Tab {
public:
	Checkbox enable;

	Edit     id_deagle;
	Checkbox stattrak_deagle;
	Slider   quality_deagle;
	Slider	 seed_deagle;

	Edit     id_elite;
	Checkbox stattrak_elite;
	Slider   quality_elite;
	Slider	 seed_elite;

	Edit     id_fiveseven;
	Checkbox stattrak_fiveseven;
	Slider   quality_fiveseven;
	Slider	 seed_fiveseven;

	Edit     id_glock;
	Checkbox stattrak_glock;
	Slider   quality_glock;
	Slider	 seed_glock;

	Edit     id_ak47;
	Checkbox stattrak_ak47;
	Slider   quality_ak47;
	Slider	 seed_ak47;

	Edit     id_aug;
	Checkbox stattrak_aug;
	Slider   quality_aug;
	Slider	 seed_aug;

	Edit     id_awp;
	Checkbox stattrak_awp;
	Slider   quality_awp;
	Slider	 seed_awp;

	Edit     id_famas;
	Checkbox stattrak_famas;
	Slider   quality_famas;
	Slider	 seed_famas;

	Edit     id_g3sg1;
	Checkbox stattrak_g3sg1;
	Slider   quality_g3sg1;
	Slider	 seed_g3sg1;

	Edit     id_galil;
	Checkbox stattrak_galil;
	Slider   quality_galil;
	Slider	 seed_galil;

	Edit     id_m249;
	Checkbox stattrak_m249;
	Slider   quality_m249;
	Slider	 seed_m249;

	Edit     id_m4a4;
	Checkbox stattrak_m4a4;
	Slider   quality_m4a4;
	Slider	 seed_m4a4;

	Edit     id_mac10;
	Checkbox stattrak_mac10;
	Slider   quality_mac10;
	Slider	 seed_mac10;

	Edit     id_p90;
	Checkbox stattrak_p90;
	Slider   quality_p90;
	Slider	 seed_p90;

	Edit     id_ump45;
	Checkbox stattrak_ump45;
	Slider   quality_ump45;
	Slider	 seed_ump45;

	Edit     id_xm1014;
	Checkbox stattrak_xm1014;
	Slider   quality_xm1014;
	Slider	 seed_xm1014;

	Edit     id_bizon;
	Checkbox stattrak_bizon;
	Slider   quality_bizon;
	Slider	 seed_bizon;

	Edit     id_mag7;
	Checkbox stattrak_mag7;
	Slider   quality_mag7;
	Slider	 seed_mag7;

	Edit     id_negev;
	Checkbox stattrak_negev;
	Slider   quality_negev;
	Slider	 seed_negev;

	Edit     id_sawedoff;
	Checkbox stattrak_sawedoff;
	Slider   quality_sawedoff;
	Slider	 seed_sawedoff;

	Edit     id_tec9;
	Checkbox stattrak_tec9;
	Slider   quality_tec9;
	Slider	 seed_tec9;

	Edit     id_p2000;
	Checkbox stattrak_p2000;
	Slider   quality_p2000;
	Slider	 seed_p2000;

	Edit     id_mp7;
	Checkbox stattrak_mp7;
	Slider   quality_mp7;
	Slider	 seed_mp7;

	Edit     id_mp9;
	Checkbox stattrak_mp9;
	Slider   quality_mp9;
	Slider	 seed_mp9;

	Edit     id_nova;
	Checkbox stattrak_nova;
	Slider   quality_nova;
	Slider	 seed_nova;

	Edit     id_p250;
	Checkbox stattrak_p250;
	Slider   quality_p250;
	Slider	 seed_p250;

	Edit     id_scar20;
	Checkbox stattrak_scar20;
	Slider   quality_scar20;
	Slider	 seed_scar20;

	Edit     id_sg553;
	Checkbox stattrak_sg553;
	Slider   quality_sg553;
	Slider	 seed_sg553;

	Edit     id_ssg08;
	Checkbox stattrak_ssg08;
	Slider   quality_ssg08;
	Slider	 seed_ssg08;

	Edit     id_m4a1s;
	Checkbox stattrak_m4a1s;
	Slider   quality_m4a1s;
	Slider	 seed_m4a1s;

	Edit     id_usps;
	Checkbox stattrak_usps;
	Slider   quality_usps;
	Slider	 seed_usps;

	Edit     id_cz75a;
	Checkbox stattrak_cz75a;
	Slider   quality_cz75a;
	Slider	 seed_cz75a;

	Edit     id_revolver;
	Checkbox stattrak_revolver;
	Slider   quality_revolver;
	Slider	 seed_revolver;

	Edit     id_bayonet;
	Checkbox stattrak_bayonet;
	Slider   quality_bayonet;
	Slider	 seed_bayonet;

	Edit     id_flip;
	Checkbox stattrak_flip;
	Slider   quality_flip;
	Slider	 seed_flip;

	Edit     id_gut;
	Checkbox stattrak_gut;
	Slider   quality_gut;
	Slider	 seed_gut;

	Edit     id_karambit;
	Checkbox stattrak_karambit;
	Slider   quality_karambit;
	Slider	 seed_karambit;

	Edit     id_m9;
	Checkbox stattrak_m9;
	Slider   quality_m9;
	Slider	 seed_m9;

	Edit     id_huntsman;
	Checkbox stattrak_huntsman;
	Slider   quality_huntsman;
	Slider	 seed_huntsman;

	Edit     id_falchion;
	Checkbox stattrak_falchion;
	Slider   quality_falchion;
	Slider	 seed_falchion;

	Edit     id_bowie;
	Checkbox stattrak_bowie;
	Slider   quality_bowie;
	Slider	 seed_bowie;

	Edit     id_butterfly;
	Checkbox stattrak_butterfly;
	Slider   quality_butterfly;
	Slider	 seed_butterfly;

	Edit     id_daggers;
	Checkbox stattrak_daggers;
	Slider   quality_daggers;
	Slider	 seed_daggers;

	// col 2.
	Dropdown knife;
	Dropdown glove;
	Edit	 glove_id;

public:
	void init( ) {
		SetTitle( XOR( "skins" ) );

		enable.setup( XOR( "enable" ), XOR( "skins_enable" ) );
		enable.SetCallback( callbacks::ForceFullUpdate );
		RegisterElement( &enable );

		// weapons...
		id_deagle.setup( XOR( "paintkit id" ), XOR( "id_deagle" ), 3 );
		id_deagle.SetCallback( callbacks::SkinUpdate );
		id_deagle.AddShowCallback( callbacks::DEAGLE );
		RegisterElement( &id_deagle );

		stattrak_deagle.setup( XOR( "stattrak" ), XOR( "stattrak_deagle" ) );
		stattrak_deagle.SetCallback( callbacks::SkinUpdate );
		stattrak_deagle.AddShowCallback( callbacks::DEAGLE );
		RegisterElement( &stattrak_deagle );

		quality_deagle.setup( XOR( "quality" ), XOR( "quality_deagle" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_deagle.SetCallback( callbacks::SkinUpdate );
		quality_deagle.AddShowCallback( callbacks::DEAGLE );
		RegisterElement( &quality_deagle );

		seed_deagle.setup( XOR( "seed" ), XOR( "seed_deagle" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_deagle.SetCallback( callbacks::SkinUpdate );
		seed_deagle.AddShowCallback( callbacks::DEAGLE );
		RegisterElement( &seed_deagle );

		id_elite.setup( XOR( "paintkit id" ), XOR( "id_elite" ), 3 );
		id_elite.SetCallback( callbacks::SkinUpdate );
		id_elite.AddShowCallback( callbacks::ELITE );
		RegisterElement( &id_elite );

		stattrak_elite.setup( XOR( "stattrak" ), XOR( "stattrak_elite" ) );
		stattrak_elite.SetCallback( callbacks::SkinUpdate );
		stattrak_elite.AddShowCallback( callbacks::ELITE );
		RegisterElement( &stattrak_elite );

		quality_elite.setup( XOR( "quality" ), XOR( "quality_elite" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_elite.SetCallback( callbacks::SkinUpdate );
		quality_elite.AddShowCallback( callbacks::ELITE );
		RegisterElement( &quality_elite );

		seed_elite.setup( XOR( "seed" ), XOR( "seed_elite" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_elite.SetCallback( callbacks::SkinUpdate );
		seed_elite.AddShowCallback( callbacks::ELITE );
		RegisterElement( &seed_elite );

		id_fiveseven.setup( XOR( "paintkit id" ), XOR( "id_fiveseven" ), 3 );
		id_fiveseven.SetCallback( callbacks::SkinUpdate );
		id_fiveseven.AddShowCallback( callbacks::FIVESEVEN );
		RegisterElement( &id_fiveseven );

		stattrak_fiveseven.setup( XOR( "stattrak" ), XOR( "stattrak_fiveseven" ) );
		stattrak_fiveseven.SetCallback( callbacks::SkinUpdate );
		stattrak_fiveseven.AddShowCallback( callbacks::FIVESEVEN );
		RegisterElement( &stattrak_fiveseven );

		quality_fiveseven.setup( XOR( "quality" ), XOR( "quality_fiveseven" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_fiveseven.SetCallback( callbacks::SkinUpdate );
		quality_fiveseven.AddShowCallback( callbacks::FIVESEVEN );
		RegisterElement( &quality_fiveseven );

		seed_fiveseven.setup( XOR( "seed" ), XOR( "seed_fiveseven" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_fiveseven.SetCallback( callbacks::SkinUpdate );
		seed_fiveseven.AddShowCallback( callbacks::FIVESEVEN );
		RegisterElement( &seed_fiveseven );

		id_glock.setup( XOR( "paintkit id" ), XOR( "id_glock" ), 3 );
		id_glock.SetCallback( callbacks::SkinUpdate );
		id_glock.AddShowCallback( callbacks::GLOCK );
		RegisterElement( &id_glock );

		stattrak_glock.setup( XOR( "stattrak" ), XOR( "stattrak_glock" ) );
		stattrak_glock.SetCallback( callbacks::SkinUpdate );
		stattrak_glock.AddShowCallback( callbacks::GLOCK );
		RegisterElement( &stattrak_glock );

		quality_glock.setup( XOR( "quality" ), XOR( "quality_glock" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_glock.SetCallback( callbacks::SkinUpdate );
		quality_glock.AddShowCallback( callbacks::GLOCK );
		RegisterElement( &quality_glock );

		seed_glock.setup( XOR( "seed" ), XOR( "seed_glock" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_glock.SetCallback( callbacks::SkinUpdate );
		seed_glock.AddShowCallback( callbacks::GLOCK );
		RegisterElement( &seed_glock );

		id_ak47.setup( XOR( "paintkit id" ), XOR( "id_ak47" ), 3 );
		id_ak47.SetCallback( callbacks::SkinUpdate );
		id_ak47.AddShowCallback( callbacks::AK47 );
		RegisterElement( &id_ak47 );

		stattrak_ak47.setup( XOR( "stattrak" ), XOR( "stattrak_ak47" ) );
		stattrak_ak47.SetCallback( callbacks::SkinUpdate );
		stattrak_ak47.AddShowCallback( callbacks::AK47 );
		RegisterElement( &stattrak_ak47 );

		quality_ak47.setup( XOR( "quality" ), XOR( "quality_ak47" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_ak47.SetCallback( callbacks::SkinUpdate );
		quality_ak47.AddShowCallback( callbacks::AK47 );
		RegisterElement( &quality_ak47 );

		seed_ak47.setup( XOR( "seed" ), XOR( "seed_ak47" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_ak47.SetCallback( callbacks::SkinUpdate );
		seed_ak47.AddShowCallback( callbacks::AK47 );
		RegisterElement( &seed_ak47 );

		id_aug.setup( XOR( "paintkit id" ), XOR( "id_aug" ), 3 );
		id_aug.SetCallback( callbacks::SkinUpdate );
		id_aug.AddShowCallback( callbacks::AUG );
		RegisterElement( &id_aug );

		stattrak_aug.setup( XOR( "stattrak" ), XOR( "stattrak_aug" ) );
		stattrak_aug.SetCallback( callbacks::SkinUpdate );
		stattrak_aug.AddShowCallback( callbacks::AUG );
		RegisterElement( &stattrak_aug );

		quality_aug.setup( XOR( "quality" ), XOR( "quality_aug" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_aug.SetCallback( callbacks::SkinUpdate );
		quality_aug.AddShowCallback( callbacks::AUG );
		RegisterElement( &quality_aug );

		seed_aug.setup( XOR( "seed" ), XOR( "seed_aug" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_aug.SetCallback( callbacks::SkinUpdate );
		seed_aug.AddShowCallback( callbacks::AUG );
		RegisterElement( &seed_aug );

		id_awp.setup( XOR( "paintkit id" ), XOR( "id_awp" ), 3 );
		id_awp.SetCallback( callbacks::SkinUpdate );
		id_awp.AddShowCallback( callbacks::AWP );
		RegisterElement( &id_awp );

		stattrak_awp.setup( XOR( "stattrak" ), XOR( "stattrak_awp" ) );
		stattrak_awp.SetCallback( callbacks::SkinUpdate );
		stattrak_awp.AddShowCallback( callbacks::AWP );
		RegisterElement( &stattrak_awp );

		quality_awp.setup( XOR( "quality" ), XOR( "quality_awp" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_awp.SetCallback( callbacks::SkinUpdate );
		quality_awp.AddShowCallback( callbacks::AWP );
		RegisterElement( &quality_awp );

		seed_awp.setup( XOR( "seed" ), XOR( "seed_awp" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_awp.SetCallback( callbacks::SkinUpdate );
		seed_awp.AddShowCallback( callbacks::AWP );
		RegisterElement( &seed_awp );

		id_famas.setup( XOR( "paintkit id" ), XOR( "id_famas" ), 3 );
		id_famas.SetCallback( callbacks::SkinUpdate );
		id_famas.AddShowCallback( callbacks::FAMAS );
		RegisterElement( &id_famas );

		stattrak_famas.setup( XOR( "stattrak" ), XOR( "stattrak_famas" ) );
		stattrak_famas.SetCallback( callbacks::SkinUpdate );
		stattrak_famas.AddShowCallback( callbacks::FAMAS );
		RegisterElement( &stattrak_famas );

		quality_famas.setup( XOR( "quality" ), XOR( "quality_famas" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_famas.SetCallback( callbacks::SkinUpdate );
		quality_famas.AddShowCallback( callbacks::FAMAS );
		RegisterElement( &quality_famas );

		seed_famas.setup( XOR( "seed" ), XOR( "seed_famas" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_famas.SetCallback( callbacks::SkinUpdate );
		seed_famas.AddShowCallback( callbacks::FAMAS );
		RegisterElement( &seed_famas );

		id_g3sg1.setup( XOR( "paintkit id" ), XOR( "id_g3sg1" ), 3 );
		id_g3sg1.SetCallback( callbacks::SkinUpdate );
		id_g3sg1.AddShowCallback( callbacks::G3SG1 );
		RegisterElement( &id_g3sg1 );

		stattrak_g3sg1.setup( XOR( "stattrak" ), XOR( "stattrak_g3sg1" ) );
		stattrak_g3sg1.SetCallback( callbacks::SkinUpdate );
		stattrak_g3sg1.AddShowCallback( callbacks::G3SG1 );
		RegisterElement( &stattrak_g3sg1 );

		quality_g3sg1.setup( XOR( "quality" ), XOR( "quality_g3sg1" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_g3sg1.SetCallback( callbacks::SkinUpdate );
		quality_g3sg1.AddShowCallback( callbacks::G3SG1 );
		RegisterElement( &quality_g3sg1 );

		seed_g3sg1.setup( XOR( "seed" ), XOR( "seed_g3sg1" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_g3sg1.SetCallback( callbacks::SkinUpdate );
		seed_g3sg1.AddShowCallback( callbacks::G3SG1 );
		RegisterElement( &seed_g3sg1 );

		id_galil.setup( XOR( "paintkit id" ), XOR( "id_galil" ), 3 );
		id_galil.SetCallback( callbacks::SkinUpdate );
		id_galil.AddShowCallback( callbacks::GALIL );
		RegisterElement( &id_galil );

		stattrak_galil.setup( XOR( "stattrak" ), XOR( "stattrak_galil" ) );
		stattrak_galil.SetCallback( callbacks::SkinUpdate );
		stattrak_galil.AddShowCallback( callbacks::GALIL );
		RegisterElement( &stattrak_galil );

		quality_galil.setup( XOR( "quality" ), XOR( "quality_galil" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_galil.SetCallback( callbacks::SkinUpdate );
		quality_galil.AddShowCallback( callbacks::GALIL );
		RegisterElement( &quality_galil );

		seed_galil.setup( XOR( "seed" ), XOR( "seed_galil" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_galil.SetCallback( callbacks::SkinUpdate );
		seed_galil.AddShowCallback( callbacks::GALIL );
		RegisterElement( &seed_galil );

		id_m249.setup( XOR( "paintkit id" ), XOR( "id_m249" ), 3 );
		id_m249.SetCallback( callbacks::SkinUpdate );
		id_m249.AddShowCallback( callbacks::M249 );
		RegisterElement( &id_m249 );

		stattrak_m249.setup( XOR( "stattrak" ), XOR( "stattrak_m249" ) );
		stattrak_m249.SetCallback( callbacks::SkinUpdate );
		stattrak_m249.AddShowCallback( callbacks::M249 );
		RegisterElement( &stattrak_m249 );

		quality_m249.setup( XOR( "quality" ), XOR( "quality_m249" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_m249.SetCallback( callbacks::SkinUpdate );
		quality_m249.AddShowCallback( callbacks::M249 );
		RegisterElement( &quality_m249 );

		seed_m249.setup( XOR( "seed" ), XOR( "seed_m249" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_m249.SetCallback( callbacks::SkinUpdate );
		seed_m249.AddShowCallback( callbacks::M249 );
		RegisterElement( &seed_m249 );

		id_m4a4.setup( XOR( "paintkit id" ), XOR( "id_m4a4" ), 3 );
		id_m4a4.SetCallback( callbacks::SkinUpdate );
		id_m4a4.AddShowCallback( callbacks::M4A4 );
		RegisterElement( &id_m4a4 );

		stattrak_m4a4.setup( XOR( "stattrak" ), XOR( "stattrak_m4a4" ) );
		stattrak_m4a4.SetCallback( callbacks::SkinUpdate );
		stattrak_m4a4.AddShowCallback( callbacks::M4A4 );
		RegisterElement( &stattrak_m4a4 );

		quality_m4a4.setup( XOR( "quality" ), XOR( "quality_m4a4" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_m4a4.SetCallback( callbacks::SkinUpdate );
		quality_m4a4.AddShowCallback( callbacks::M4A4 );
		RegisterElement( &quality_m4a4 );

		seed_m4a4.setup( XOR( "seed" ), XOR( "seed_m4a4" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_m4a4.SetCallback( callbacks::SkinUpdate );
		seed_m4a4.AddShowCallback( callbacks::M4A4 );
		RegisterElement( &seed_m4a4 );

		id_mac10.setup( XOR( "paintkit id" ), XOR( "id_mac10" ), 3 );
		id_mac10.SetCallback( callbacks::SkinUpdate );
		id_mac10.AddShowCallback( callbacks::MAC10 );
		RegisterElement( &id_mac10 );

		stattrak_mac10.setup( XOR( "stattrak" ), XOR( "stattrak_mac10" ) );
		stattrak_mac10.SetCallback( callbacks::SkinUpdate );
		stattrak_mac10.AddShowCallback( callbacks::MAC10 );
		RegisterElement( &stattrak_mac10 );

		quality_mac10.setup( XOR( "quality" ), XOR( "quality_mac10" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_mac10.SetCallback( callbacks::SkinUpdate );
		quality_mac10.AddShowCallback( callbacks::MAC10 );
		RegisterElement( &quality_mac10 );

		seed_mac10.setup( XOR( "seed" ), XOR( "seed_mac10" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_mac10.SetCallback( callbacks::SkinUpdate );
		seed_mac10.AddShowCallback( callbacks::MAC10 );
		RegisterElement( &seed_mac10 );

		id_p90.setup( XOR( "paintkit id" ), XOR( "id_p90" ), 3 );
		id_p90.SetCallback( callbacks::SkinUpdate );
		id_p90.AddShowCallback( callbacks::P90 );
		RegisterElement( &id_p90 );

		stattrak_p90.setup( XOR( "stattrak" ), XOR( "stattrak_p90" ) );
		stattrak_p90.SetCallback( callbacks::SkinUpdate );
		stattrak_p90.AddShowCallback( callbacks::P90 );
		RegisterElement( &stattrak_p90 );

		quality_p90.setup( XOR( "quality" ), XOR( "quality_p90" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_p90.SetCallback( callbacks::SkinUpdate );
		quality_p90.AddShowCallback( callbacks::P90 );
		RegisterElement( &quality_p90 );

		seed_p90.setup( XOR( "seed" ), XOR( "seed_p90" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_p90.SetCallback( callbacks::SkinUpdate );
		seed_p90.AddShowCallback( callbacks::P90 );
		RegisterElement( &seed_p90 );

		id_ump45.setup( XOR( "paintkit id" ), XOR( "id_ump45" ), 3 );
		id_ump45.SetCallback( callbacks::SkinUpdate );
		id_ump45.AddShowCallback( callbacks::UMP45 );
		RegisterElement( &id_ump45 );

		stattrak_ump45.setup( XOR( "stattrak" ), XOR( "stattrak_ump45" ) );
		stattrak_ump45.SetCallback( callbacks::SkinUpdate );
		stattrak_ump45.AddShowCallback( callbacks::UMP45 );
		RegisterElement( &stattrak_ump45 );

		quality_ump45.setup( XOR( "quality" ), XOR( "quality_ump45" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_ump45.SetCallback( callbacks::SkinUpdate );
		quality_ump45.AddShowCallback( callbacks::UMP45 );
		RegisterElement( &quality_ump45 );

		seed_ump45.setup( XOR( "seed" ), XOR( "seed_ump45" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_ump45.SetCallback( callbacks::SkinUpdate );
		seed_ump45.AddShowCallback( callbacks::UMP45 );
		RegisterElement( &seed_ump45 );

		id_xm1014.setup( XOR( "paintkit id" ), XOR( "id_xm1014" ), 3 );
		id_xm1014.SetCallback( callbacks::SkinUpdate );
		id_xm1014.AddShowCallback( callbacks::XM1014 );
		RegisterElement( &id_xm1014 );

		stattrak_xm1014.setup( XOR( "stattrak" ), XOR( "stattrak_xm1014" ) );
		stattrak_xm1014.SetCallback( callbacks::SkinUpdate );
		stattrak_xm1014.AddShowCallback( callbacks::XM1014 );
		RegisterElement( &stattrak_xm1014 );

		quality_xm1014.setup( XOR( "quality" ), XOR( "quality_xm1014" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_xm1014.SetCallback( callbacks::SkinUpdate );
		quality_xm1014.AddShowCallback( callbacks::XM1014 );
		RegisterElement( &quality_xm1014 );

		seed_xm1014.setup( XOR( "seed" ), XOR( "seed_xm1014" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_xm1014.SetCallback( callbacks::SkinUpdate );
		seed_xm1014.AddShowCallback( callbacks::XM1014 );
		RegisterElement( &seed_xm1014 );

		id_bizon.setup( XOR( "paintkit id" ), XOR( "id_bizon" ), 3 );
		id_bizon.SetCallback( callbacks::SkinUpdate );
		id_bizon.AddShowCallback( callbacks::BIZON );
		RegisterElement( &id_bizon );

		stattrak_bizon.setup( XOR( "stattrak" ), XOR( "stattrak_bizon" ) );
		stattrak_bizon.SetCallback( callbacks::SkinUpdate );
		stattrak_bizon.AddShowCallback( callbacks::BIZON );
		RegisterElement( &stattrak_bizon );

		quality_bizon.setup( XOR( "quality" ), XOR( "quality_bizon" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_bizon.SetCallback( callbacks::SkinUpdate );
		quality_bizon.AddShowCallback( callbacks::BIZON );
		RegisterElement( &quality_bizon );

		seed_bizon.setup( XOR( "seed" ), XOR( "seed_bizon" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_bizon.SetCallback( callbacks::SkinUpdate );
		seed_bizon.AddShowCallback( callbacks::BIZON );
		RegisterElement( &seed_bizon );

		id_mag7.setup( XOR( "paintkit id" ), XOR( "id_mag7" ), 3 );
		id_mag7.SetCallback( callbacks::SkinUpdate );
		id_mag7.AddShowCallback( callbacks::MAG7 );
		RegisterElement( &id_mag7 );

		stattrak_mag7.setup( XOR( "stattrak" ), XOR( "stattrak_mag7" ) );
		stattrak_mag7.SetCallback( callbacks::SkinUpdate );
		stattrak_mag7.AddShowCallback( callbacks::MAG7 );
		RegisterElement( &stattrak_mag7 );

		quality_mag7.setup( XOR( "quality" ), XOR( "quality_mag7" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_mag7.SetCallback( callbacks::SkinUpdate );
		quality_mag7.AddShowCallback( callbacks::MAG7 );
		RegisterElement( &quality_mag7 );

		seed_mag7.setup( XOR( "seed" ), XOR( "seed_mag7" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_mag7.SetCallback( callbacks::SkinUpdate );
		seed_mag7.AddShowCallback( callbacks::MAG7 );
		RegisterElement( &seed_mag7 );

		id_negev.setup( XOR( "paintkit id" ), XOR( "id_negev" ), 3 );
		id_negev.SetCallback( callbacks::SkinUpdate );
		id_negev.AddShowCallback( callbacks::NEGEV );
		RegisterElement( &id_negev );

		stattrak_negev.setup( XOR( "stattrak" ), XOR( "stattrak_negev" ) );
		stattrak_negev.SetCallback( callbacks::SkinUpdate );
		stattrak_negev.AddShowCallback( callbacks::NEGEV );
		RegisterElement( &stattrak_negev );

		quality_negev.setup( XOR( "quality" ), XOR( "quality_negev" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_negev.SetCallback( callbacks::SkinUpdate );
		quality_negev.AddShowCallback( callbacks::NEGEV );
		RegisterElement( &quality_negev );

		seed_negev.setup( XOR( "seed" ), XOR( "seed_negev" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_negev.SetCallback( callbacks::SkinUpdate );
		seed_negev.AddShowCallback( callbacks::NEGEV );
		RegisterElement( &seed_negev );

		id_sawedoff.setup( XOR( "paintkit id" ), XOR( "id_sawedoff" ), 3 );
		id_sawedoff.SetCallback( callbacks::SkinUpdate );
		id_sawedoff.AddShowCallback( callbacks::SAWEDOFF );
		RegisterElement( &id_sawedoff );

		stattrak_sawedoff.setup( XOR( "stattrak" ), XOR( "stattrak_sawedoff" ) );
		stattrak_sawedoff.SetCallback( callbacks::SkinUpdate );
		stattrak_sawedoff.AddShowCallback( callbacks::SAWEDOFF );
		RegisterElement( &stattrak_sawedoff );

		quality_sawedoff.setup( XOR( "quality" ), XOR( "quality_sawedoff" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_sawedoff.SetCallback( callbacks::SkinUpdate );
		quality_sawedoff.AddShowCallback( callbacks::SAWEDOFF );
		RegisterElement( &quality_sawedoff );

		seed_sawedoff.setup( XOR( "seed" ), XOR( "seed_sawedoff" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_sawedoff.SetCallback( callbacks::SkinUpdate );
		seed_sawedoff.AddShowCallback( callbacks::SAWEDOFF );
		RegisterElement( &seed_sawedoff );

		id_tec9.setup( XOR( "paintkit id" ), XOR( "id_tec9" ), 3 );
		id_tec9.SetCallback( callbacks::SkinUpdate );
		id_tec9.AddShowCallback( callbacks::TEC9 );
		RegisterElement( &id_tec9 );

		stattrak_tec9.setup( XOR( "stattrak" ), XOR( "stattrak_tec9" ) );
		stattrak_tec9.SetCallback( callbacks::SkinUpdate );
		stattrak_tec9.AddShowCallback( callbacks::TEC9 );
		RegisterElement( &stattrak_tec9 );

		quality_tec9.setup( XOR( "quality" ), XOR( "quality_tec9" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_tec9.SetCallback( callbacks::SkinUpdate );
		quality_tec9.AddShowCallback( callbacks::TEC9 );
		RegisterElement( &quality_tec9 );

		seed_tec9.setup( XOR( "seed" ), XOR( "seed_tec9" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_tec9.SetCallback( callbacks::SkinUpdate );
		seed_tec9.AddShowCallback( callbacks::TEC9 );
		RegisterElement( &seed_tec9 );

		id_p2000.setup( XOR( "paintkit id" ), XOR( "id_p2000" ), 3 );
		id_p2000.SetCallback( callbacks::SkinUpdate );
		id_p2000.AddShowCallback( callbacks::P2000 );
		RegisterElement( &id_p2000 );

		stattrak_p2000.setup( XOR( "stattrak" ), XOR( "stattrak_p2000" ) );
		stattrak_p2000.SetCallback( callbacks::SkinUpdate );
		stattrak_p2000.AddShowCallback( callbacks::P2000 );
		RegisterElement( &stattrak_p2000 );

		quality_p2000.setup( XOR( "quality" ), XOR( "quality_p2000" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_p2000.SetCallback( callbacks::SkinUpdate );
		quality_p2000.AddShowCallback( callbacks::P2000 );
		RegisterElement( &quality_p2000 );

		seed_p2000.setup( XOR( "seed" ), XOR( "seed_p2000" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_p2000.SetCallback( callbacks::SkinUpdate );
		seed_p2000.AddShowCallback( callbacks::P2000 );
		RegisterElement( &seed_p2000 );

		id_mp7.setup( XOR( "paintkit id" ), XOR( "id_mp7" ), 3 );
		id_mp7.SetCallback( callbacks::SkinUpdate );
		id_mp7.AddShowCallback( callbacks::MP7 );
		RegisterElement( &id_mp7 );

		stattrak_mp7.setup( XOR( "stattrak" ), XOR( "stattrak_mp7" ) );
		stattrak_mp7.SetCallback( callbacks::SkinUpdate );
		stattrak_mp7.AddShowCallback( callbacks::MP7 );
		RegisterElement( &stattrak_mp7 );

		quality_mp7.setup( XOR( "quality" ), XOR( "quality_mp7" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_mp7.SetCallback( callbacks::SkinUpdate );
		quality_mp7.AddShowCallback( callbacks::MP7 );
		RegisterElement( &quality_mp7 );

		seed_mp7.setup( XOR( "seed" ), XOR( "seed_mp7" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_mp7.SetCallback( callbacks::SkinUpdate );
		seed_mp7.AddShowCallback( callbacks::MP7 );
		RegisterElement( &seed_mp7 );

		id_mp9.setup( XOR( "paintkit id" ), XOR( "id_mp9" ), 3 );
		id_mp9.SetCallback( callbacks::SkinUpdate );
		id_mp9.AddShowCallback( callbacks::MP9 );
		RegisterElement( &id_mp9 );

		stattrak_mp9.setup( XOR( "stattrak" ), XOR( "stattrak_mp9" ) );
		stattrak_mp9.SetCallback( callbacks::SkinUpdate );
		stattrak_mp9.AddShowCallback( callbacks::MP9 );
		RegisterElement( &stattrak_mp9 );

		quality_mp9.setup( XOR( "quality" ), XOR( "quality_mp9" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_mp9.SetCallback( callbacks::SkinUpdate );
		quality_mp9.AddShowCallback( callbacks::MP9 );
		RegisterElement( &quality_mp9 );

		seed_mp9.setup( XOR( "seed" ), XOR( "seed_mp9" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_mp9.SetCallback( callbacks::SkinUpdate );
		seed_mp9.AddShowCallback( callbacks::MP9 );
		RegisterElement( &seed_mp9 );

		id_nova.setup( XOR( "paintkit id" ), XOR( "id_nova" ), 3 );
		id_nova.SetCallback( callbacks::SkinUpdate );
		id_nova.AddShowCallback( callbacks::NOVA );
		RegisterElement( &id_nova );

		stattrak_nova.setup( XOR( "stattrak" ), XOR( "stattrak_nova" ) );
		stattrak_nova.SetCallback( callbacks::SkinUpdate );
		stattrak_nova.AddShowCallback( callbacks::NOVA );
		RegisterElement( &stattrak_nova );

		quality_nova.setup( XOR( "quality" ), XOR( "quality_nova" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_nova.SetCallback( callbacks::SkinUpdate );
		quality_nova.AddShowCallback( callbacks::NOVA );
		RegisterElement( &quality_nova );

		seed_nova.setup( XOR( "seed" ), XOR( "seed_nova" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_nova.SetCallback( callbacks::SkinUpdate );
		seed_nova.AddShowCallback( callbacks::NOVA );
		RegisterElement( &seed_nova );

		id_p250.setup( XOR( "paintkit id" ), XOR( "id_p250" ), 3 );
		id_p250.SetCallback( callbacks::SkinUpdate );
		id_p250.AddShowCallback( callbacks::P250 );
		RegisterElement( &id_p250 );

		stattrak_p250.setup( XOR( "stattrak" ), XOR( "stattrak_p250" ) );
		stattrak_p250.SetCallback( callbacks::SkinUpdate );
		stattrak_p250.AddShowCallback( callbacks::P250 );
		RegisterElement( &stattrak_p250 );

		quality_p250.setup( XOR( "quality" ), XOR( "quality_p250" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_p250.SetCallback( callbacks::SkinUpdate );
		quality_p250.AddShowCallback( callbacks::P250 );
		RegisterElement( &quality_p250 );

		seed_p250.setup( XOR( "seed" ), XOR( "seed_p250" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_p250.SetCallback( callbacks::SkinUpdate );
		seed_p250.AddShowCallback( callbacks::P250 );
		RegisterElement( &seed_p250 );

		id_scar20.setup( XOR( "paintkit id" ), XOR( "id_scar20" ), 3 );
		id_scar20.SetCallback( callbacks::SkinUpdate );
		id_scar20.AddShowCallback( callbacks::SCAR20 );
		RegisterElement( &id_scar20 );

		stattrak_scar20.setup( XOR( "stattrak" ), XOR( "stattrak_scar20" ) );
		stattrak_scar20.SetCallback( callbacks::SkinUpdate );
		stattrak_scar20.AddShowCallback( callbacks::SCAR20 );
		RegisterElement( &stattrak_scar20 );

		quality_scar20.setup( XOR( "quality" ), XOR( "quality_scar20" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_scar20.SetCallback( callbacks::SkinUpdate );
		quality_scar20.AddShowCallback( callbacks::SCAR20 );
		RegisterElement( &quality_scar20 );

		seed_scar20.setup( XOR( "seed" ), XOR( "seed_scar20" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_scar20.SetCallback( callbacks::SkinUpdate );
		seed_scar20.AddShowCallback( callbacks::SCAR20 );
		RegisterElement( &seed_scar20 );

		id_sg553.setup( XOR( "paintkit id" ), XOR( "id_sg553" ), 3 );
		id_sg553.SetCallback( callbacks::SkinUpdate );
		id_sg553.AddShowCallback( callbacks::SG553 );
		RegisterElement( &id_sg553 );

		stattrak_sg553.setup( XOR( "stattrak" ), XOR( "stattrak_sg553" ) );
		stattrak_sg553.SetCallback( callbacks::SkinUpdate );
		stattrak_sg553.AddShowCallback( callbacks::SG553 );
		RegisterElement( &stattrak_sg553 );

		quality_sg553.setup( XOR( "quality" ), XOR( "quality_sg553" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_sg553.SetCallback( callbacks::SkinUpdate );
		quality_sg553.AddShowCallback( callbacks::SG553 );
		RegisterElement( &quality_sg553 );

		seed_sg553.setup( XOR( "seed" ), XOR( "seed_sg553" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_sg553.SetCallback( callbacks::SkinUpdate );
		seed_sg553.AddShowCallback( callbacks::SG553 );
		RegisterElement( &seed_sg553 );

		id_ssg08.setup( XOR( "paintkit id" ), XOR( "id_ssg08" ), 3 );
		id_ssg08.SetCallback( callbacks::SkinUpdate );
		id_ssg08.AddShowCallback( callbacks::SSG08 );
		RegisterElement( &id_ssg08 );

		stattrak_ssg08.setup( XOR( "stattrak" ), XOR( "stattrak_ssg08" ) );
		stattrak_ssg08.SetCallback( callbacks::SkinUpdate );
		stattrak_ssg08.AddShowCallback( callbacks::SSG08 );
		RegisterElement( &stattrak_ssg08 );

		quality_ssg08.setup( XOR( "quality" ), XOR( "quality_ssg08" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_ssg08.SetCallback( callbacks::SkinUpdate );
		quality_ssg08.AddShowCallback( callbacks::SSG08 );
		RegisterElement( &quality_ssg08 );

		seed_ssg08.setup( XOR( "seed" ), XOR( "seed_ssg08" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_ssg08.SetCallback( callbacks::SkinUpdate );
		seed_ssg08.AddShowCallback( callbacks::SSG08 );
		RegisterElement( &seed_ssg08 );

		id_m4a1s.setup( XOR( "paintkit id" ), XOR( "id_m4a1s" ), 3 );
		id_m4a1s.SetCallback( callbacks::SkinUpdate );
		id_m4a1s.AddShowCallback( callbacks::M4A1S );
		RegisterElement( &id_m4a1s );

		stattrak_m4a1s.setup( XOR( "stattrak" ), XOR( "stattrak_m4a1s" ) );
		stattrak_m4a1s.SetCallback( callbacks::SkinUpdate );
		stattrak_m4a1s.AddShowCallback( callbacks::M4A1S );
		RegisterElement( &stattrak_m4a1s );

		quality_m4a1s.setup( XOR( "quality" ), XOR( "quality_m4a1s" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_m4a1s.SetCallback( callbacks::SkinUpdate );
		quality_m4a1s.AddShowCallback( callbacks::M4A1S );
		RegisterElement( &quality_m4a1s );

		seed_m4a1s.setup( XOR( "seed" ), XOR( "seed_m4a1s" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_m4a1s.SetCallback( callbacks::SkinUpdate );
		seed_m4a1s.AddShowCallback( callbacks::M4A1S );
		RegisterElement( &seed_m4a1s );

		id_usps.setup( XOR( "paintkit id" ), XOR( "id_usps" ), 3 );
		id_usps.SetCallback( callbacks::SkinUpdate );
		id_usps.AddShowCallback( callbacks::USPS );
		RegisterElement( &id_usps );

		stattrak_usps.setup( XOR( "stattrak" ), XOR( "stattrak_usps" ) );
		stattrak_usps.SetCallback( callbacks::SkinUpdate );
		stattrak_usps.AddShowCallback( callbacks::USPS );
		RegisterElement( &stattrak_usps );

		quality_usps.setup( XOR( "quality" ), XOR( "quality_usps" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_usps.SetCallback( callbacks::SkinUpdate );
		quality_usps.AddShowCallback( callbacks::USPS );
		RegisterElement( &quality_usps );

		seed_usps.setup( XOR( "seed" ), XOR( "seed_usps" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_usps.SetCallback( callbacks::SkinUpdate );
		seed_usps.AddShowCallback( callbacks::USPS );
		RegisterElement( &seed_usps );

		id_cz75a.setup( XOR( "paintkit id" ), XOR( "id_cz75a" ), 3 );
		id_cz75a.SetCallback( callbacks::SkinUpdate );
		id_cz75a.AddShowCallback( callbacks::CZ75A );
		RegisterElement( &id_cz75a );

		stattrak_cz75a.setup( XOR( "stattrak" ), XOR( "stattrak_cz75a" ) );
		stattrak_cz75a.SetCallback( callbacks::SkinUpdate );
		stattrak_cz75a.AddShowCallback( callbacks::CZ75A );
		RegisterElement( &stattrak_cz75a );

		quality_cz75a.setup( XOR( "quality" ), XOR( "quality_cz75a" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_cz75a.SetCallback( callbacks::SkinUpdate );
		quality_cz75a.AddShowCallback( callbacks::CZ75A );
		RegisterElement( &quality_cz75a );

		seed_cz75a.setup( XOR( "seed" ), XOR( "seed_cz75a" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_cz75a.SetCallback( callbacks::SkinUpdate );
		seed_cz75a.AddShowCallback( callbacks::CZ75A );
		RegisterElement( &seed_cz75a );

		id_revolver.setup( XOR( "paintkit id" ), XOR( "id_revolver" ), 3 );
		id_revolver.SetCallback( callbacks::SkinUpdate );
		id_revolver.AddShowCallback( callbacks::REVOLVER );
		RegisterElement( &id_revolver );

		stattrak_revolver.setup( XOR( "stattrak" ), XOR( "stattrak_revolver" ) );
		stattrak_revolver.SetCallback( callbacks::SkinUpdate );
		stattrak_revolver.AddShowCallback( callbacks::REVOLVER );
		RegisterElement( &stattrak_revolver );

		quality_revolver.setup( XOR( "quality" ), XOR( "quality_revolver" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_revolver.SetCallback( callbacks::SkinUpdate );
		quality_revolver.AddShowCallback( callbacks::REVOLVER );
		RegisterElement( &quality_revolver );

		seed_revolver.setup( XOR( "seed" ), XOR( "seed_revolver" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_revolver.SetCallback( callbacks::SkinUpdate );
		seed_revolver.AddShowCallback( callbacks::REVOLVER );
		RegisterElement( &seed_revolver );

		id_bayonet.setup( XOR( "paintkit id" ), XOR( "id_bayonet" ), 3 );
		id_bayonet.SetCallback( callbacks::SkinUpdate );
		id_bayonet.AddShowCallback( callbacks::KNIFE_BAYONET );
		RegisterElement( &id_bayonet );

		stattrak_bayonet.setup( XOR( "stattrak" ), XOR( "stattrak_bayonet" ) );
		stattrak_bayonet.SetCallback( callbacks::SkinUpdate );
		stattrak_bayonet.AddShowCallback( callbacks::KNIFE_BAYONET );
		RegisterElement( &stattrak_bayonet );

		quality_bayonet.setup( XOR( "quality" ), XOR( "quality_bayonet" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_bayonet.SetCallback( callbacks::SkinUpdate );
		quality_bayonet.AddShowCallback( callbacks::KNIFE_BAYONET );
		RegisterElement( &quality_bayonet );

		seed_bayonet.setup( XOR( "seed" ), XOR( "seed_bayonet" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_bayonet.SetCallback( callbacks::SkinUpdate );
		seed_bayonet.AddShowCallback( callbacks::KNIFE_BAYONET );
		RegisterElement( &seed_bayonet );

		id_flip.setup( XOR( "paintkit id" ), XOR( "id_flip" ), 3 );
		id_flip.SetCallback( callbacks::SkinUpdate );
		id_flip.AddShowCallback( callbacks::KNIFE_FLIP );
		RegisterElement( &id_flip );

		stattrak_flip.setup( XOR( "stattrak" ), XOR( "stattrak_flip" ) );
		stattrak_flip.SetCallback( callbacks::SkinUpdate );
		stattrak_flip.AddShowCallback( callbacks::KNIFE_FLIP );
		RegisterElement( &stattrak_flip );

		quality_flip.setup( XOR( "quality" ), XOR( "quality_flip" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_flip.SetCallback( callbacks::SkinUpdate );
		quality_flip.AddShowCallback( callbacks::KNIFE_FLIP );
		RegisterElement( &quality_flip );

		seed_flip.setup( XOR( "seed" ), XOR( "seed_flip" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_flip.SetCallback( callbacks::SkinUpdate );
		seed_flip.AddShowCallback( callbacks::KNIFE_FLIP );
		RegisterElement( &seed_flip );

		id_gut.setup( XOR( "paintkit id" ), XOR( "id_gut" ), 3 );
		id_gut.SetCallback( callbacks::SkinUpdate );
		id_gut.AddShowCallback( callbacks::KNIFE_GUT );
		RegisterElement( &id_gut );

		stattrak_gut.setup( XOR( "stattrak" ), XOR( "stattrak_gut" ) );
		stattrak_gut.SetCallback( callbacks::SkinUpdate );
		stattrak_gut.AddShowCallback( callbacks::KNIFE_GUT );
		RegisterElement( &stattrak_gut );

		quality_gut.setup( XOR( "quality" ), XOR( "quality_gut" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_gut.SetCallback( callbacks::SkinUpdate );
		quality_gut.AddShowCallback( callbacks::KNIFE_GUT );
		RegisterElement( &quality_gut );

		seed_gut.setup( XOR( "seed" ), XOR( "seed_gut" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_gut.SetCallback( callbacks::SkinUpdate );
		seed_gut.AddShowCallback( callbacks::KNIFE_GUT );
		RegisterElement( &seed_gut );

		id_karambit.setup( XOR( "paintkit id" ), XOR( "id_karambit" ), 3 );
		id_karambit.SetCallback( callbacks::SkinUpdate );
		id_karambit.AddShowCallback( callbacks::KNIFE_KARAMBIT );
		RegisterElement( &id_karambit );

		stattrak_karambit.setup( XOR( "stattrak" ), XOR( "stattrak_karambit" ) );
		stattrak_karambit.SetCallback( callbacks::SkinUpdate );
		stattrak_karambit.AddShowCallback( callbacks::KNIFE_KARAMBIT );
		RegisterElement( &stattrak_karambit );

		quality_karambit.setup( XOR( "quality" ), XOR( "quality_karambit" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_karambit.SetCallback( callbacks::SkinUpdate );
		quality_karambit.AddShowCallback( callbacks::KNIFE_KARAMBIT );
		RegisterElement( &quality_karambit );

		seed_karambit.setup( XOR( "seed" ), XOR( "seed_karambit" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_karambit.SetCallback( callbacks::SkinUpdate );
		seed_karambit.AddShowCallback( callbacks::KNIFE_KARAMBIT );
		RegisterElement( &seed_karambit );

		id_m9.setup( XOR( "paintkit id" ), XOR( "id_m9" ), 3 );
		id_m9.SetCallback( callbacks::SkinUpdate );
		id_m9.AddShowCallback( callbacks::KNIFE_M9_BAYONET );
		RegisterElement( &id_m9 );

		stattrak_m9.setup( XOR( "stattrak" ), XOR( "stattrak_m9" ) );
		stattrak_m9.SetCallback( callbacks::SkinUpdate );
		stattrak_m9.AddShowCallback( callbacks::KNIFE_M9_BAYONET );
		RegisterElement( &stattrak_m9 );

		quality_m9.setup( XOR( "quality" ), XOR( "quality_m9" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_m9.SetCallback( callbacks::SkinUpdate );
		quality_m9.AddShowCallback( callbacks::KNIFE_M9_BAYONET );
		RegisterElement( &quality_m9 );

		seed_m9.setup( XOR( "seed" ), XOR( "seed_m9" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_m9.SetCallback( callbacks::SkinUpdate );
		seed_m9.AddShowCallback( callbacks::KNIFE_M9_BAYONET );
		RegisterElement( &seed_m9 );

		id_huntsman.setup( XOR( "paintkit id" ), XOR( "id_huntsman" ), 3 );
		id_huntsman.SetCallback( callbacks::SkinUpdate );
		id_huntsman.AddShowCallback( callbacks::KNIFE_HUNTSMAN );
		RegisterElement( &id_huntsman );

		stattrak_huntsman.setup( XOR( "stattrak" ), XOR( "stattrak_huntsman" ) );
		stattrak_huntsman.SetCallback( callbacks::SkinUpdate );
		stattrak_huntsman.AddShowCallback( callbacks::KNIFE_HUNTSMAN );
		RegisterElement( &stattrak_huntsman );

		quality_huntsman.setup( XOR( "quality" ), XOR( "quality_huntsman" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_huntsman.SetCallback( callbacks::SkinUpdate );
		quality_huntsman.AddShowCallback( callbacks::KNIFE_HUNTSMAN );
		RegisterElement( &quality_huntsman );

		seed_huntsman.setup( XOR( "seed" ), XOR( "seed_huntsman" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_huntsman.SetCallback( callbacks::SkinUpdate );
		seed_huntsman.AddShowCallback( callbacks::KNIFE_HUNTSMAN );
		RegisterElement( &seed_huntsman );

		id_falchion.setup( XOR( "paintkit id" ), XOR( "id_falchion" ), 3 );
		id_falchion.SetCallback( callbacks::SkinUpdate );
		id_falchion.AddShowCallback( callbacks::KNIFE_FALCHION );
		RegisterElement( &id_falchion );

		stattrak_falchion.setup( XOR( "stattrak" ), XOR( "stattrak_falchion" ) );
		stattrak_falchion.SetCallback( callbacks::SkinUpdate );
		stattrak_falchion.AddShowCallback( callbacks::KNIFE_FALCHION );
		RegisterElement( &stattrak_falchion );

		quality_falchion.setup( XOR( "quality" ), XOR( "quality_falchion" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_falchion.SetCallback( callbacks::SkinUpdate );
		quality_falchion.AddShowCallback( callbacks::KNIFE_FALCHION );
		RegisterElement( &quality_falchion );

		seed_falchion.setup( XOR( "seed" ), XOR( "seed_falchion" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_falchion.SetCallback( callbacks::SkinUpdate );
		seed_falchion.AddShowCallback( callbacks::KNIFE_FALCHION );
		RegisterElement( &seed_falchion );

		id_bowie.setup( XOR( "paintkit id" ), XOR( "id_bowie" ), 3 );
		id_bowie.SetCallback( callbacks::SkinUpdate );
		id_bowie.AddShowCallback( callbacks::KNIFE_BOWIE );
		RegisterElement( &id_bowie );

		stattrak_bowie.setup( XOR( "stattrak" ), XOR( "stattrak_bowie" ) );
		stattrak_bowie.SetCallback( callbacks::SkinUpdate );
		stattrak_bowie.AddShowCallback( callbacks::KNIFE_BOWIE );
		RegisterElement( &stattrak_bowie );

		quality_bowie.setup( XOR( "quality" ), XOR( "quality_bowie" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_bowie.SetCallback( callbacks::SkinUpdate );
		quality_bowie.AddShowCallback( callbacks::KNIFE_BOWIE );
		RegisterElement( &quality_bowie );

		seed_bowie.setup( XOR( "seed" ), XOR( "seed_bowie" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_bowie.SetCallback( callbacks::SkinUpdate );
		seed_bowie.AddShowCallback( callbacks::KNIFE_BOWIE );
		RegisterElement( &seed_bowie );

		id_butterfly.setup( XOR( "paintkit id" ), XOR( "id_butterfly" ), 3 );
		id_butterfly.SetCallback( callbacks::SkinUpdate );
		id_butterfly.AddShowCallback( callbacks::KNIFE_BUTTERFLY );
		RegisterElement( &id_butterfly );

		stattrak_butterfly.setup( XOR( "stattrak" ), XOR( "stattrak_butterfly" ) );
		stattrak_butterfly.SetCallback( callbacks::SkinUpdate );
		stattrak_butterfly.AddShowCallback( callbacks::KNIFE_BUTTERFLY );
		RegisterElement( &stattrak_butterfly );

		quality_butterfly.setup( XOR( "quality" ), XOR( "quality_butterfly" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_butterfly.SetCallback( callbacks::SkinUpdate );
		quality_butterfly.AddShowCallback( callbacks::KNIFE_BUTTERFLY );
		RegisterElement( &quality_butterfly );

		seed_butterfly.setup( XOR( "seed" ), XOR( "seed_butterfly" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_butterfly.SetCallback( callbacks::SkinUpdate );
		seed_butterfly.AddShowCallback( callbacks::KNIFE_BUTTERFLY );
		RegisterElement( &seed_butterfly );

		id_daggers.setup( XOR( "paintkit id" ), XOR( "id_daggers" ), 3 );
		id_daggers.SetCallback( callbacks::SkinUpdate );
		id_daggers.AddShowCallback( callbacks::KNIFE_SHADOW_DAGGERS );
		RegisterElement( &id_daggers );

		stattrak_daggers.setup( XOR( "stattrak" ), XOR( "stattrak_daggers" ) );
		stattrak_daggers.SetCallback( callbacks::SkinUpdate );
		stattrak_daggers.AddShowCallback( callbacks::KNIFE_SHADOW_DAGGERS );
		RegisterElement( &stattrak_daggers );

		quality_daggers.setup( XOR( "quality" ), XOR( "quality_daggers" ), 1.f, 100.f, true, 0, 100.f, 1.f, XOR( L"%" ) );
		quality_daggers.SetCallback( callbacks::SkinUpdate );
		quality_daggers.AddShowCallback( callbacks::KNIFE_SHADOW_DAGGERS );
		RegisterElement( &quality_daggers );

		seed_daggers.setup( XOR( "seed" ), XOR( "seed_daggers" ), 0.f, 255.f, true, 0, 0.f, 1.f );
		seed_daggers.SetCallback( callbacks::SkinUpdate );
		seed_daggers.AddShowCallback( callbacks::KNIFE_SHADOW_DAGGERS );
		RegisterElement( &seed_daggers );

		// col 2.
		knife.setup( XOR( "knife model" ), XOR( "skins_knife_model" ), { XOR( "off" ), XOR( "bayonet" ), XOR( "bowie" ), XOR( "butterfly" ), XOR( "falchion" ), XOR( "flip" ), XOR( "gut" ), XOR( "huntsman" ), XOR( "karambit" ), XOR( "m9 bayonet" ), XOR( "daggers" ) } );
		knife.SetCallback( callbacks::SkinUpdate );
		RegisterElement( &knife, 1 );

		glove.setup( XOR( "glove model" ), XOR( "skins_glove_model" ), { XOR( "off" ), XOR( "bloodhound" ), XOR( "sport" ), XOR( "driver" ), XOR( "handwraps" ), XOR( "moto" ), XOR( "specialist" ) } );
		glove.SetCallback( callbacks::ForceFullUpdate );
		RegisterElement( &glove, 1 );

		glove_id.setup( XOR( "glove paintkit id" ), XOR( "skins_glove_id" ), 2 );
		glove_id.SetCallback( callbacks::ForceFullUpdate );
		RegisterElement( &glove_id, 1 );
	}
};


class ConfigTab : public Tab {
public:
	Colorpicker menu_color;
	Checkbox cp_gooka_killsay;
	Checkbox watermark;

	Dropdown config;
	Button   save;
	Button   load;

public:

	void init( ) {
		SetTitle( XOR( "config" ) );

		menu_color.setup( XOR( "menu color" ), XOR( "menu_color" ), colors::burgundy, &g_gui.m_color );
		RegisterElement( &menu_color );

		cp_gooka_killsay.setup( XOR( "cp smug killsay" ), XOR( "cp_gooka_killsay" ) );
		RegisterElement( &cp_gooka_killsay );

		watermark.setup( XOR( "watermark" ), XOR( "watermark" ) );
		RegisterElement( &watermark );

		config.setup( XOR( "configuration" ), XOR( "cfg" ), { XOR( "1" ), XOR( "2" ), XOR( "3" ), XOR( "4" ), XOR( "5" ), XOR( "6" ), XOR( "7" ), XOR( "8" ), XOR( "9" ) } );
		config.RemoveFlags( ElementFlags::SAVE );
		RegisterElement( &config, 1 );

		save.setup( XOR( "save" ) );
		save.SetCallback( callbacks::ConfigSave );
		RegisterElement( &save, 1 );

		load.setup( XOR( "load" ) );
		load.SetCallback( callbacks::ConfigLoad );
		RegisterElement( &load, 1 );
	}
};

class MainForm : public Form {
public:
	// ragaobabo tab
	AimbotTab    aimbot;
	AntiAimTab   antiaim;

	// ihate this
	PlayersTab   players;
	VisualsTab visuals;

	// movement tab
	MovementTab movement;

	// misctab
	MiscTab misc;

	// skins tab
	SkinsTab skins;

	// config
	ConfigTab	 config;

public:
	void init( ) {
		SetPosition( 50, 50 );
		SetSize( 630, 500 );

		// aim.
		RegisterTab( &aimbot );
		aimbot.init( );

		// LLL
		RegisterTab( &antiaim );
		antiaim.init( );

		//rape me
		RegisterTab( &players );
		players.init( );

		RegisterTab( &visuals );
		visuals.init( );

		RegisterTab( &movement );
		movement.init( );

		RegisterTab( &skins );
		skins.init( );

		RegisterTab( &misc );
		misc.init( );

		// configtab
		RegisterTab( &config );
		config.init( );
	}
};

class Menu {
public:
	MainForm main;

public:
	void init( ) {
		main.init( );
		g_gui.RegisterForm( &main, VK_INSERT );
	}
};

extern Menu g_menu;