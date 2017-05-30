#pragma once
#include "augs/graphics/pixel.h"

#include "augs/misc/stepped_timing.h"
#include "augs/misc/enum_associative_array.h"

#include "game/transcendental/entity_id.h"

#include "game/assets/spell_id.h"

#include "game/assets/sound_buffer_id.h"
#include "game/assets/particle_effect_id.h"

#include "game/components/transform_component.h"

#include "game/detail/all_sentience_meters.h"
#include "game/detail/spells/all_spells.h"
#include "game/detail/perks/all_perks.h"

#include "game/detail/spell_logic.h"
#include "augs/misc/value_meter.h"

#include "augs/templates/type_in_list_id.h"
#include "augs/templates/constexpr_arithmetic.h"

namespace components {
	struct sentience {
		// GEN INTROSPECTOR struct components::sentience
		augs::stepped_timestamp time_of_last_received_damage;
		augs::stepped_timestamp time_of_last_exertion;

		augs::stepped_cooldown cast_cooldown_for_all_spells;

		meter_instance_tuple meters;

		std::array<
			zeroed_pod<bool>, 
			aligned_num_of_bytes_v<num_types_in_list_v<spell_instance_tuple>, 4> 
		> learned_spells;

		spell_instance_tuple spells;
		perk_instance_tuple perks;

		spell_id currently_casted_spell;
		
		components::transform transform_when_spell_casted;
		augs::stepped_timestamp time_of_last_spell_cast;
		augs::stepped_timestamp time_of_last_exhausted_cast;

		augs::stepped_timestamp time_of_last_shake;
		float shake_for_ms = 0.f;

		float comfort_zone = 500.f;
		float minimum_danger_amount_to_evade = 5.f;
		float danger_amount_from_hostile_attitude = 100.f;

		child_entity_id health_damage_particles;
		child_entity_id character_crosshair;

		sound_effect_input health_decrease_sound;
		sound_effect_input consciousness_decrease_sound;

		sound_effect_input death_sound;
		sound_effect_input loss_of_consciousness_sound;

		particle_effect_input health_decrease_particles;

		// END GEN INTROSPECTOR

		bool is_learned(const spell_id id) const {
			ensure(id.is_set());
			return learned_spells[id.get_index()] == true;
		}

		bool is_spell_being_cast() const {
			return currently_casted_spell.is_set();
		}

		rgba calculate_health_color(float time_pulse_multiplier) const;
		bool is_conscious() const;

	private:
		template <class T, class List>
		using can_get_from_t = std::enable_if_t<is_one_of_list_v<T, std::decay_t<List>>>;
	public:

		template <class T>
		auto& get(can_get_from_t<T, meter_instance_tuple>* = nullptr) {
			return std::get<T>(meters);
		}

		template <class T>
		const auto& get(can_get_from_t<T, meter_instance_tuple>* = nullptr) const {
			return std::get<T>(meters);
		}

		template <class T>
		auto& get(can_get_from_t<T, perk_instance_tuple>* = nullptr) {
			return std::get<T>(perks);
		}

		template <class T>
		const auto& get(can_get_from_t<T, perk_instance_tuple>* = nullptr) const {
			return std::get<T>(perks);
		}
	};
}