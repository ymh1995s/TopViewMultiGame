#include "pch.h"
#include "Object.h"

int Object::SetId()
{
	_objectId = nextId.fetch_add(1, memory_order_relaxed);
	return _objectId;
}