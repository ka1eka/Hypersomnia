#pragma once
#include "augs/math/vec2.h"
#include "application/setups/editor/resources/editor_typed_resource_id.h"

struct editor_sprite_resource;

struct editor_sprite_node {
	// GEN INTROSPECTOR struct editor_sprite_node
	editor_typed_resource_id<editor_sprite_resource> resource_id;

	vec2 pos;
	real32 rotation = 0.0f;

	bool flip_horizontally = false;
	bool flip_vertically = false;
	// END GEN INTROSPECTOR
};
