#pragma once
#include "entity_system/component.h"
#include "math/vec2d.h"

#include "visibility_component.h"

namespace components {
	struct pathfinding : public augmentations::entity_system::component {
		typedef std::pair<augmentations::vec2<>, augmentations::vec2<>> edge;

		pathfinding() : is_finding_a_path(false), enable_backtracking(true), target_offset(0.f) {}

		bool is_finding_a_path;
		bool enable_backtracking;

		float target_offset;

		struct pathfinding_session {
			augmentations::vec2<> target, navigate_to;

			struct navigation_vertex {
				augmentations::vec2<> location, sensor;
			};

			std::vector<navigation_vertex> discovered_vertices, undiscovered_vertices;

			//std::vector<edge> visible_walls, undiscovered_walls;
			//std::vector<visibility::discontinuity> undiscovered_discontinuities;
		} session;

		std::vector <pathfinding_session> session_stack;

		void start_pathfinding(augmentations::vec2<> target) {
			clear_pathfinding_info();
			session.target = target;
			is_finding_a_path = true;
		}

		augmentations::vec2<> get_current_navigation_target() const {
			return session.navigate_to;
		}

		bool is_still_pathfinding() const {
			return is_finding_a_path;
		}

		void clear_pathfinding_info() {
			//session.undiscovered_discontinuities.clear();
			//session.visible_walls.clear();
			//session.undiscovered_walls.clear();
			session.discovered_vertices.clear();
			session.undiscovered_vertices.clear();
			session_stack.clear();
		}
	};
}