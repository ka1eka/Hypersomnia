#pragma once
#include "game/cosmic_entropy.h"

#include "entity_system/storage_for_components_and_aggregates.h"
#include "entity_system/storage_for_message_queues.h"
#include "entity_system/storage_for_systems.h"

#include "game/types_specification/all_components_declaration.h"
#include "game/types_specification/all_messages_declaration.h"
#include "game/types_specification/all_systems_declaration.h"

#include "game/temporary_systems/dynamic_tree_system.h"
#include "game/temporary_systems/physics_system.h"
#include "game/temporary_systems/processing_lists_system.h"

#include "game/stateful_systems/gui_system.h"

#include "misc/delta.h"
#include "misc/pool_handlizer.h"

#include "game/entity_id.h"
#include "game/detail/inventory_slot_id.h"
#include "game/entity_handle_declaration.h"
#include "game/detail/inventory_slot_handle_declaration.h"
#include "game/global/all_settings.h"
#include "game/cosmic_profiler.h"
#include "game/step.h"
#include <functional>

class cosmos : public augs::pool_handlizer<cosmos>
{
	storage_for_all_components_and_aggregates components_and_aggregates;

	void advance_deterministic_schemata(fixed_step& step_state);
	void call_rendering_schemata(viewing_step& step_state) const;

public:
	typedef std::function<void(fixed_step&)> fixed_callback;
	typedef std::function<void(viewing_step&)> variable_callback;

	storage_for_all_temporary_systems temporary_systems;
	storage_for_all_stateful_systems stateful_systems;
	all_settings settings;

	cosmic_profiler profiler;

	unsigned long long current_step_number = 0;

	augs::fixed_delta delta;

	cosmos();

	void advance_deterministic_schemata(cosmic_entropy input,
		fixed_callback pre_solve = fixed_callback(), 
		fixed_callback post_solve = fixed_callback());

	void reserve_storage_for_entities(size_t);

	entity_handle create_entity(std::string debug_name);
	entity_handle clone_entity(entity_id);
	void delete_entity(entity_id);

	void complete_resubstantialization(entity_handle);

	entity_handle get_handle(entity_id);
	const_entity_handle get_handle(entity_id) const;
	inventory_slot_handle get_handle(inventory_slot_id);
	const_inventory_slot_handle get_handle(inventory_slot_id) const;

	randomization get_rng_for(entity_id) const;

	std::vector<entity_handle> get(processing_subjects);
	std::vector<const_entity_handle> get(processing_subjects) const;

	size_t entities_count() const;
	std::wstring summary() const;
	
	template <class T>
	decltype(auto) get_pool(augs::pool_id<T> id) {
		return components_and_aggregates.get_pool(id);
	}

	template <class T>
	decltype(auto) get_pool(augs::pool_id<T> id) const {
		return components_and_aggregates.get_pool(id);
	}
};