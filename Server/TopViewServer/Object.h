#pragma once

enum ObjectType
{
	OBJECT_TYPE_PLAYER = 1,
	OBJECT_TYPE_PROJECTILE = 2,
	OBJECT_TYPE_DUST = 3,
	OBJECT_TYPE_OBSTACLE = 4,
};

class Object
{
public:
	uint32_t _objectId;
	pair<float, float> _position;

private:
	ObjectType _type;
};

