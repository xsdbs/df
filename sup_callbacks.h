#pragma once

namespace callbacks {
	void SkinUpdate( );
	void ForceFullUpdate( );

	bool AnglePreference( );

	void ConfigLoad1( );
	void ConfigLoad2( );
	void ConfigLoad3( );
	void ConfigLoad4( );
	void ConfigLoad5( );
	void ConfigLoad6( );
	void ConfigLoad( );
	void ConfigSave( );

	bool IsHitchanceOn( );
	bool IsPenetrationOn( );
	bool IsMultipointOn( );
	bool IsSeparateScaleOn( );
	bool CanApplyLimbsScale( );
	bool IsMultipointBodyOn( );

	bool IsAccuracyTabOn( );
	bool IsLagcompTabOn( );
	bool IsBindsTabOn( );
	bool EarlyInactive( );
	bool IsVisualizeAimbotMatrixOn( );
	bool IsOverrideDamageOn( );

	void ToggleDmgOverride( );
	void ToggleForceBodyAim( );
	bool IsFakeLastMoveOn( );
	void ToggleDesync( );
	bool MetallicChamsEnemy( );
	bool MetallicChamsFriendly( );
	bool MetallicChamsLocal( );

	void ToggleThirdPerson( );
	void ChangeWorld( );
	void ToggleFakeLatency( );

	bool IsAspectRatioOn( );

	// weapon cfgs.
	bool DEAGLE( );
	bool ELITE( );
	bool FIVESEVEN( );
	bool GLOCK( );
	bool AK47( );
	bool AUG( );
	bool AWP( );
	bool FAMAS( );
	bool G3SG1( );
	bool GALIL( );
	bool M249( );
	bool M4A4( );
	bool MAC10( );
	bool P90( );
	bool UMP45( );
	bool XM1014( );
	bool BIZON( );
	bool MAG7( );
	bool NEGEV( );
	bool SAWEDOFF( );
	bool TEC9( );
	bool P2000( );
	bool MP7( );
	bool MP9( );
	bool NOVA( );
	bool P250( );
	bool SCAR20( );
	bool SG553( );
	bool SSG08( );
	bool M4A1S( );
	bool USPS( );
	bool CZ75A( );
	bool REVOLVER( );
	bool KNIFE_BAYONET( );
	bool KNIFE_FLIP( );
	bool KNIFE_GUT( );
	bool KNIFE_KARAMBIT( );
	bool KNIFE_M9_BAYONET( );
	bool KNIFE_HUNTSMAN( );
	bool KNIFE_FALCHION( );
	bool KNIFE_BOWIE( );
	bool KNIFE_BUTTERFLY( );
	bool KNIFE_SHADOW_DAGGERS( );
}