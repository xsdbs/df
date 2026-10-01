#pragma once

class InputPrediction
{
public:
	bool m_disable_setup_bones;
	float m_curtime;
	float m_frametime;
	bool m_in_pred;
public:
	datamap_t* cm_start;
	datamap_t* og_predicted;

	void GetDatamap( datamap_t*& map );
	void SetDatamap( datamap_t*& map );

public:
	void update( );
	void store( );
	void simulate_animations( CUserCmd* cmd );
	void run( Entity* ent, CUserCmd* cmd, IMoveHelper* movehelper );
	float get_lowest_inaccuracy( Weapon* weapon );
	void setup_bones( );
	void predict( );
	void restore( );
};

extern InputPrediction g_inputpred;