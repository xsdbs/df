#pragma once

class Bones
{
public:
	bool BuildBones( Player* target, int mask, BoneArray* out, LagRecord& record );
	bool SetupBonesSimple( Player* target, int mask, BoneArray* out );
};

extern Bones g_bones;