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
	virtual ~Object() = default;
public:
	int SetId();
public:
	int _objectId;
	static inline  atomic<int> nextId { 0 };
	pair<float, float> _position;
	ObjectType _type;

private:
};

