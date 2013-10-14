#pragma once
#include <random>
#include "utility/timer.h"

#include "entity_system/processing_system.h"

#include "../components/particle_emitter_component.h"

using namespace augmentations;
using namespace entity_system;

extern int randval(int min, int max);
extern unsigned randval(unsigned min, unsigned max);
extern float randval(float min, float max);

extern unsigned randval(std::pair<unsigned, unsigned>);
extern float randval(std::pair<float, float>);

namespace components {
	struct particle_group;
}

class particle_emitter_system : public processing_system_templated<components::particle_emitter> {
public:
	static void spawn_particle(components::particle_group&, const vec2<>&, float, const resources::emission&);
	void process_events(world&) override;
};