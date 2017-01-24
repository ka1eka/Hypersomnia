#pragma once
#include <unordered_map>
#include <array>

#include "game/detail/gui/game_gui_context.h"
#include "game/transcendental/entity_handle_declaration.h"
#include "augs/graphics/renderer.h"

struct character_gui;
class viewing_step;

class gui_element_system {
public:
	std::unordered_map<entity_id, item_button> item_buttons;
	std::unordered_map<inventory_slot_id, slot_button> slot_buttons;

	std::unordered_map<entity_id, character_gui> character_guis;

	std::vector<item_slot_transfer_request_data> pending_transfers;

	cosmic_entropy get_and_clear_pending_events();
	void clear_all_pending_events();

	void queue_transfer(const item_slot_transfer_request_data);
	void queue_transfers(const wielding_result);

	character_gui& get_character_gui(const entity_id);
	const character_gui& get_character_gui(const entity_id) const;

	slot_button& get_slot_button(const inventory_slot_id);
	const slot_button& get_slot_button(const inventory_slot_id) const;

	item_button& get_item_button(const entity_id);
	const item_button& get_item_button(const entity_id) const;

	void advance_gui_elements(
		const const_entity_handle root_entity,
		const std::vector<augs::window::event::change>& events
	);

	void handle_hotbar_and_action_button_presses(
		const const_entity_handle root_entity,
		std::vector<key_and_mouse_intent> intents
	);

	void reserve_caches_for_entities(const size_t) const {}

	void resample_state_for_audiovisuals(const cosmos&);
};