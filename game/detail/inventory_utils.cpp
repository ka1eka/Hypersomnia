#include "inventory_utils.h"
#include "game/transcendental/entity_id.h"
#include "game/transcendental/cosmos.h"
#include "game/components/item_component.h"
#include "game/components/damage_component.h"
#include "game/components/fixtures_component.h"
#include "game/components/physics_component.h"
#include "game/components/special_physics_component.h"
#include "game/components/interpolation_component.h"
#include "game/components/item_slot_transfers_component.h"
#include "game/components/name_component.h"
#include "game/detail/entity_scripts.h"
#include "game/messages/queue_destruction.h"
#include "game/transcendental/entity_handle.h"

#include "augs/ensure.h"
#include "game/transcendental/step.h"
#include "augs/templates/string_templates.h"
#include "game/detail/gui/gui_positioning.h"

#include "game/enums/entity_name.h"

item_transfer_result query_transfer_result(const const_item_slot_transfer_request r) {
	item_transfer_result output;
	auto& predicted_result = output.result;
	const auto& item = r.get_item().get<components::item>();

	ensure(r.specified_quantity != 0);

	const auto item_owning_capability = r.get_item().get_owning_transfer_capability();
	const auto target_slot_owning_capability = r.get_target_slot().get_container().get_owning_transfer_capability();

	//ensure(item_owning_capability.alive() || target_slot_owning_capability.alive());

	if (item_owning_capability.alive() && target_slot_owning_capability.alive() &&
		item_owning_capability != target_slot_owning_capability) {
		predicted_result = item_transfer_result_type::INVALID_SLOT_OR_UNOWNED_ROOT;
	}
	else if (r.get_target_slot().alive()) {
		output = containment_result(r);
	}
	else {
		output.result = item_transfer_result_type::SUCCESSFUL_TRANSFER;
		output.transferred_charges = r.specified_quantity == -1 ? item.charges : std::min(r.specified_quantity, item.charges);
	}

	if (predicted_result == item_transfer_result_type::SUCCESSFUL_TRANSFER) {
		if (item.current_mounting == components::item::MOUNTED && !r.force_immediate_mount) {
			predicted_result = item_transfer_result_type::UNMOUNT_BEFOREHAND;
		}
	}

	return output;
}

slot_function detect_compatible_slot(const const_entity_handle item, const const_entity_handle container_entity) {
	const auto* const container = container_entity.find<components::container>();

	if (container) {
		if (container_entity[slot_function::ITEM_DEPOSIT].alive()) {
			return slot_function::ITEM_DEPOSIT;
		}

		for (const auto& s : container->slots) {
			if (s.second.is_category_compatible_with(item)) {
				return s.first;
			}
		}
	}

	return slot_function::INVALID;
}

item_transfer_result containment_result(const const_item_slot_transfer_request r, const bool allow_replacement) {
	item_transfer_result output;
	output.transferred_charges = 0;
	output.result = item_transfer_result_type::NO_SLOT_AVAILABLE;

	const auto& item = r.get_item().get<components::item>();
	const auto& cosmos = r.get_item().get_cosmos();
	const auto& slot = *r.get_target_slot();
	auto& result = output.result;

	if (item.current_slot == r.get_target_slot()) {
		result = item_transfer_result_type::THE_SAME_SLOT;
	}
	else if (slot.always_allow_exactly_one_item && slot.items_inside.size() == 1 && !can_stack_entities(cosmos[slot.items_inside[0]], r.get_item())) {
		//if (allow_replacement) {
		//
		//}
		//auto& item_to_replace = slot.items_inside[0];
		//
		//auto& current_slot_of_replacer = item.current_slot;
		//auto replace_request = r;
		//
		//replace_request.item = slot.items_inside[0];
		//replace_request.target_slot = current_slot_of_replacer;
		//
		//if (containment_result.)
		//
		//	if (current_slot_of_replacer.alive())

		result = item_transfer_result_type::NO_SLOT_AVAILABLE;
	}
	else if (!slot.is_category_compatible_with(r.get_item())) {
		result = item_transfer_result_type::INCOMPATIBLE_CATEGORIES;
	}
	else {
		const auto space_available = r.get_target_slot().calculate_free_space_with_parent_containers();

		if (space_available > 0) {
			const bool item_indivisible = item.charges == 1 || !item.stackable;

			if (item_indivisible) {
				if (space_available >= calculate_space_occupied_with_children(r.get_item())) {
					output.transferred_charges = 1;
				}
			}
			else {
				const int maximum_charges_fitting_inside = space_available / item.space_occupied_per_charge;
				output.transferred_charges = std::min(item.charges, maximum_charges_fitting_inside);

				if (r.specified_quantity > -1) {
					output.transferred_charges = std::min(output.transferred_charges, unsigned(r.specified_quantity));
				}
			}
		}

		if (output.transferred_charges == 0) {
			output.result = item_transfer_result_type::INSUFFICIENT_SPACE;
		}
		else {
			output.result = item_transfer_result_type::SUCCESSFUL_TRANSFER;
		}
	}
		
	return output;
}

bool can_stack_entities(const const_entity_handle a, const const_entity_handle b) {
	const bool same_names =
		a.has<components::name>() &&
		b.has<components::name>() &&
		a.get<components::name>().id == b.get<components::name>().id;

	if (same_names) {
		switch (a.get<components::name>().id) {
		case entity_name::GREEN_CHARGE: return true;
		case entity_name::PINK_CHARGE: return true;
		case entity_name::CYAN_CHARGE: return true;
		default: return false;
		}
	}

	return false;
}

unsigned to_space_units(std::string s) {
	unsigned sum = 0;
	unsigned mult = SPACE_ATOMS_PER_UNIT;

	if (s.find(".") == std::string::npos) {
		int l = s.length() - 1;
		
		while(l--)
			mult *= 10;
	}
	else {
		int l = s.find(".") - 1;

		while (l--)
			mult *= 10;
	}

	for (auto& c : s) {
		ensure(mult > 0);
		if (c == '.')
			continue;

		sum += (c - '0') * mult;
		mult /= 10;
	}

	return sum;
}

int count_charges_in_deposit(const const_entity_handle item) {
	return count_charges_inside(item[slot_function::ITEM_DEPOSIT]);
}

int count_charges_inside(const const_inventory_slot_handle id) {
	int charges = 0;

	for (const auto i : id->items_inside) {
		charges += id.get_cosmos()[i].get<components::item>().charges;
	}

	return charges;
}

std::wstring format_space_units(const unsigned u) {
	if (!u) {
		return L"0";
	}

	return to_wstring(u / long double(SPACE_ATOMS_PER_UNIT), 2);
}

void drop_from_all_slots(const entity_handle c, logic_step& step) {
	const auto& container = c.get<components::container>();

	for (const auto& s : container.slots) {
		const auto items_uninvalidated = s.second.items_inside;

		for (const auto item : items_uninvalidated) {
			perform_transfer({ c.get_cosmos()[item], c.get_cosmos()[inventory_slot_id()] }, step);
		}
	}
}

unsigned calculate_space_occupied_with_children(const const_entity_handle item) {
	auto space_occupied = item.get<components::item>().get_space_occupied();

	if (item.find<components::container>()) {
		ensure(item.get<components::item>().charges == 1);

		for (auto& slot : item.get<components::container>().slots) {
			for (auto& entity_in_slot : slot.second.items_inside) {
				space_occupied += calculate_space_occupied_with_children(item.get_cosmos()[entity_in_slot]);
			}
		}
	}

	return space_occupied;
}

void add_item(const inventory_slot_handle handle, const entity_handle new_item) {
	handle->items_inside.push_back(new_item);
	new_item.get<components::item>().current_slot = handle;
}

void remove_item(const inventory_slot_handle handle, const entity_handle removed_item) {
	auto& v = handle->items_inside;
	v.erase(std::remove(v.begin(), v.end(), removed_item), v.end());
	removed_item.get<components::item>().current_slot.unset();
}

void perform_transfer(const item_slot_transfer_request r, logic_step& step) {
	auto& cosmos = r.get_item().get_cosmos();
	auto& item = r.get_item().get<components::item>();
	const auto previous_slot_id = item.current_slot;
	const auto previous_slot = cosmos[previous_slot_id];

	const auto result = query_transfer_result(r);

	if (result.result == item_transfer_result_type::UNMOUNT_BEFOREHAND) {
		ensure(false);
		ensure(previous_slot.alive());

		//item.request_unmount(r.get_target_slot());
		//item.mark_parent_enclosing_containers_for_unmount();

		return;
	}
	else if (result.result == item_transfer_result_type::SUCCESSFUL_TRANSFER) {
		const bool is_pickup_or_transfer = r.get_target_slot().alive();
		const bool is_drop_request = !is_pickup_or_transfer;

		components::transform previous_container_transform;

		entity_id target_item_to_stack_with;

		if (is_pickup_or_transfer) {
			for (auto& i : cosmos.get_handle(r.get_target_slot())->items_inside) {
				if (can_stack_entities(r.get_item(), cosmos[i])) {
					target_item_to_stack_with = i;
				}
			}
		}

		const bool whole_item_grabbed = item.charges == result.transferred_charges;

		if (previous_slot.alive()) {
			previous_container_transform = previous_slot.get_container().logic_transform();

			if (whole_item_grabbed) {
				remove_item(previous_slot, r.get_item());
			}

			if (previous_slot.is_input_enabling_slot()) {
				unset_input_flags_of_orphaned_entity(r.get_item());
			}
		}

		if (cosmos[target_item_to_stack_with].alive()) {
			if (whole_item_grabbed) {
				step.transient.messages.post(messages::queue_destruction(r.get_item()));
			}
			else {
				item.charges -= result.transferred_charges;
			}

			cosmos[target_item_to_stack_with].get<components::item>().charges += result.transferred_charges;

			return;
		}

		entity_id grabbed_item_part;

		if (whole_item_grabbed) {
			grabbed_item_part = r.get_item();
		}
		else {
			grabbed_item_part = cosmos.clone_entity(r.get_item());
			item.charges -= result.transferred_charges;
			cosmos[grabbed_item_part].get<components::item>().charges = result.transferred_charges;
		}

		const auto grabbed_item_part_handle = cosmos[grabbed_item_part];

		if (is_pickup_or_transfer) {
			add_item(r.get_target_slot(), grabbed_item_part_handle);
		}

		auto physics_updater = [previous_container_transform](const entity_handle descendant) {
			auto& cosmos = descendant.get_cosmos();

			const auto parent_slot = cosmos[descendant.get<components::item>().current_slot];
			auto def = descendant.get<components::fixtures>().get_data();
			entity_id owner_body;

			if (parent_slot.alive()) {
				def.activated = parent_slot.should_item_inside_keep_physical_body();
				
				if (def.activated) {
					owner_body = parent_slot.get_root_container();
					
					def.offsets_for_created_shapes[colliders_offset_type::ITEM_ATTACHMENT_DISPLACEMENT]
						= parent_slot.sum_attachment_offsets_of_parents(descendant);
				}
				else {
					owner_body = descendant;
				}

				def.offsets_for_created_shapes[colliders_offset_type::SPECIAL_MOVE_DISPLACEMENT].reset();
			}
			else {
				def.activated = true;
				owner_body = descendant;
				def.offsets_for_created_shapes[colliders_offset_type::ITEM_ATTACHMENT_DISPLACEMENT].reset();
				def.offsets_for_created_shapes[colliders_offset_type::SPECIAL_MOVE_DISPLACEMENT].reset();
			}

			descendant.get<components::fixtures>() = def;
			descendant.get<components::fixtures>().set_owner_body(owner_body);
			
			if (descendant.has<components::physics>()) {
				descendant.get<components::physics>().set_transform(previous_container_transform);

				if (descendant.has<components::interpolation>()) {
					descendant.get<components::interpolation>().place_of_birth = descendant.logic_transform();
				}
			}
		};

		physics_updater(grabbed_item_part_handle);
		grabbed_item_part_handle.for_each_contained_item_recursive(physics_updater);

		if (is_pickup_or_transfer) {
			initialize_item_button_for_new_gui_owner(grabbed_item_part_handle);
			grabbed_item_part_handle.for_each_contained_slot_and_item_recursive(initialize_slot_button_for_new_gui_owner, initialize_item_button_for_new_gui_owner);
		}
		
		auto& grabbed_item = grabbed_item_part_handle.get<components::item>();

		if (is_pickup_or_transfer) {
			if (r.get_target_slot()->items_need_mounting) {
				grabbed_item.intended_mounting = components::item::MOUNTED;

				if (r.force_immediate_mount) {
					grabbed_item.current_mounting = components::item::MOUNTED;
				}
			}
		}

		if (is_drop_request) {
			const auto force = vec2().set_from_degrees(previous_container_transform.rotation).set_length(60);
			const auto offset = vec2().random_on_circle(20, cosmos.get_rng_for(r.get_item()));

			auto& physics = grabbed_item_part_handle.get<components::physics>();
			physics.apply_force(force, offset, true);
			auto& special_physics = grabbed_item_part_handle.get<components::special_physics>();
			special_physics.since_dropped.set(200, cosmos.get_timestamp());
		}
	}
}