#pragma once
#include "game/cosmic_entropy.h"

#include "augs/entity_system/storage_for_components_and_aggregates.h"
#include "augs/entity_system/storage_for_message_queues.h"
#include "augs/entity_system/storage_for_systems.h"

#include "game/types_specification/all_components_declaration.h"
#include "game/types_specification/all_messages_declaration.h"
#include "game/types_specification/all_systems_declaration.h"

#include "game/temporary_systems/dynamic_tree_system.h"
#include "game/temporary_systems/physics_system.h"
#include "game/temporary_systems/processing_lists_system.h"

#include "game/systematic/gui_system.h"

#include "augs/misc/delta.h"
#include "augs/misc/pool_handlizer.h"

#include "game/entity_id.h"
#include "game/detail/inventory_slot_id.h"
#include "game/entity_handle_declaration.h"
#include "game/detail/inventory_slot_handle_declaration.h"
#include "game/global/all_settings.h"
#include "game/cosmic_profiler.h"
#include "game/step.h"
#include <functional>

class cosmos : private storage_for_all_components_and_aggregates, public augs::pool_handlizer<cosmos>
{
	void advance_deterministic_schemata(fixed_step& step_state);
	void call_rendering_schemata(viewing_step& step_state) const;

public:
	typedef std::function<void(fixed_step&)> fixed_callback;
	typedef std::function<void(viewing_step&)> variable_callback;

	storage_for_all_temporary_systems temporary_systems;
	all_settings settings;

	unsigned long long current_step_number = 0;
	augs::fixed_delta delta;

	cosmic_profiler profiler;

	template <class Archive>
	void serialize(Archive& ar) {
		ar(
			CEREAL_NVP(settings),
			CEREAL_NVP(current_step_number),
			CEREAL_NVP(delta),
			CEREAL_NVP(pool_for_aggregates),
			CEREAL_NVP(pools_for_components)
		);
		
		complete_resubstantialization();
	}

	void advance_deterministic_schemata(cosmic_entropy input,
		fixed_callback pre_solve = fixed_callback(), 
		fixed_callback post_solve = fixed_callback());

	void reserve_storage_for_entities(size_t);

	entity_handle create_entity(std::string debug_name);
	entity_handle clone_entity(entity_id);
	void delete_entity(entity_id);

	void complete_resubstantialization();
	void complete_resubstantialization(entity_handle);
	
	template<class System>
	void partial_resubstantialization(entity_handle handle) {
		auto& sys = temporary_systems.get<System>();

		sys.destruct(handle);

		if (handle.has<components::substance>())
			sys.construct(handle);
	}

	entity_handle get_handle(entity_id);
	const_entity_handle get_handle(entity_id) const;
	inventory_slot_handle get_handle(inventory_slot_id);
	const_inventory_slot_handle get_handle(inventory_slot_id) const;

	randomization get_rng_for(entity_id) const;

	std::vector<entity_handle> get(processing_subjects);
	std::vector<const_entity_handle> get(processing_subjects) const;

	size_t entities_count() const;
	std::wstring summary() const;

	using storage_for_all_components_and_aggregates::get_pool;
};
