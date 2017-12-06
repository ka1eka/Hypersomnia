#pragma once
#include <vector>

#include "augs/misc/enum/enum_array.h"

#include "game/enums/processing_subjects.h"
#include "game/transcendental/entity_id.h"
#include "game/transcendental/entity_handle_declaration.h"

#include "game/components/processing_component.h"

class cosmos;
struct cosmos_common_state;

class processing_lists_cache {
	friend class component_synchronizer<false, components::processing>;
	template<bool> friend class basic_processing_synchronizer;

	struct cache {
		bool is_constructed = false;
	};
	
	augs::enum_array<std::vector<entity_id>, processing_subjects> lists;
	std::vector<cache> per_entity_cache;
	
	void destroy_cache_of(const const_entity_handle);
	void infer_cache_for(const const_entity_handle);

	void infer_additional_cache(const cosmos_common_state&) {}
	void destroy_additional_cache_of(const cosmos_common_state&) {}

	void reserve_caches_for_entities(const size_t n);

	friend class cosmos;

public:
	const std::vector<entity_id>& get(const processing_subjects) const;
};
