#pragma once
#include "utility/delta_accumulator.h"

#include "../messages/collision_message.h"

#include "entity_system/processing_system.h"
#include "../components/physics_component.h"
#include "../components/transform_component.h"

using namespace augmentations;
using namespace entity_system;

class physics_system : public processing_system_templated<components::physics, components::transform> {
	util::delta_accumulator accumulator;

	void reset_states();
	void smooth_states();
public:
	b2World b2world;
	physics_system();

	void process_entities(world&) override;
	void add(entity*) override;
	void remove(entity*) override;
};