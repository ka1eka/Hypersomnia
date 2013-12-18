#pragma once

#include "utility/timer.h"
#include "entity_system/component.h"
#include "entity_system/entity_ptr.h"

#include "render_component.h"
#include "../game/body_helper.h"

class renderable;
class gun_system;
namespace components {
	struct gun : public augmentations::entity_system::component {
		components::render bullet_render;
		topdown::physics_info bullet_body;

		unsigned max_rounds;

		unsigned bullets_once;
		float spread_degrees;
		std::pair<float, float> bullet_damage;
		float bullet_speed;
		float shooting_interval_ms;
		float velocity_variation;
		float max_bullet_distance;

		augmentations::vec2<> bullet_distance_offset;
		float shake_radius;
		float shake_spread_degrees;

		bool is_automatic;

		unsigned current_rounds;

		bool reloading, trigger;
		
		bool is_melee;
		bool is_swinging;

		float swing_radius;
		float swing_angle;
		float swing_angular_offset;
		int query_vertices;

		augmentations::entity_system::entity_ptr target_camera_to_shake;

		gun()
			: max_rounds(0), bullets_once(0), spread_degrees(0.f), bullet_damage(std::make_pair(0.f, 0.f)), is_automatic(false),
			bullet_distance_offset(0.f), velocity_variation(0.f), shake_radius(0.f), shake_spread_degrees(0.f), max_bullet_distance(1000.f), current_rounds(0),
			is_melee(false), is_swinging(false), swing_radius(0.f), swing_angle(0.f), swing_angular_offset(0.f), query_vertices(7),
			reloading(false), trigger(false), target_camera_to_shake(nullptr) {
				bullet_body.filter.groupIndex = -1;
		}

	private:
		friend class gun_system;

		augmentations::util::timer shooting_timer;
	};
}