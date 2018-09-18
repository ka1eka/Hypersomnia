#include "augs/string/format_enum.h"
#include "augs/templates/enum_introspect.h"
#include "view/mode_gui/arena/arena_buy_menu_gui.h"

#include "augs/misc/imgui/imgui_utils.h"
#include "augs/misc/imgui/imgui_scope_wrappers.h"
#include "augs/misc/imgui/imgui_control_wrappers.h"
#include "augs/misc/imgui/imgui_game_image.h"
#include "view/viewables/images_in_atlas_map.h"
#include "application/config_lua_table.h"
#include "game/detail/entity_handle_mixins/for_each_slot_and_item.hpp"
#include "game/cosmos/for_each_entity.h"

#include "game/cosmos/entity_handle.h"
#include "game/cosmos/cosmos.h"
#include "application/app_intent_type.h"

bool arena_buy_menu_gui::control(app_ingame_intent_input in) {
	using namespace augs::event;
	using namespace augs::event::keys;

	const auto ch = in.e.get_key_change();

	if (ch == key_change::PRESSED) {
		const auto key = in.e.get_key();

		if (const auto it = mapped_or_nullptr(in.controls, key)) {
			if (*it == app_ingame_intent_type::OPEN_BUY_MENU) {
				show = !show;
				return true;
			}
		}
	}

	return false;
}

using input_type = arena_buy_menu_gui::input;
using result_type = std::optional<mode_commands::item_purchase>;

result_type arena_buy_menu_gui::perform_imgui(const input_type in) {
	using namespace augs::imgui;
	(void)in;

	const auto& subject = in.subject;
	const auto& cosm = subject.get_cosmos();

	if (subject.dead()) {
		return std::nullopt;
	}

	/* if (!show) { */
	/* 	return std::nullopt; */
	/* } */

	ImGui::SetNextWindowPosCenter();

	ImGui::SetNextWindowSize((vec2(ImGui::GetIO().DisplaySize) * 0.5f).operator ImVec2(), ImGuiCond_FirstUseEver);

	const auto window_name = "Buy menu";
	auto window = scoped_window(window_name, nullptr, ImGuiWindowFlags_NoTitleBar);

	{
		const auto s = ImGui::CalcTextSize(window_name, nullptr, true);
		const auto w = ImGui::GetWindowWidth();
		ImGui::SetCursorPosX(w / 2 - s.x / 2);
		text(window_name);
	}

	if (const auto& entry = in.images_in_atlas.at(in.money_icon).diffuse; entry.exists()) {
		game_image_button("#moneyicon", entry);
		ImGui::SameLine();
	}

	text(typesafe_sprintf("%x$"), in.available_money);

	std::vector<entity_id> owned_items;

	auto image_of = [&](const auto& of) {
		if constexpr(std::is_same_v<decltype(of), const entity_flavour_id&>) {
			return cosm.on_flavour(of, [&](const auto& typed_flavour) {
				return typed_flavour.template find<invariants::sprite>()->image_id;
			});
		}
		else {
			return cosm[of].template find<invariants::sprite>()->image_id;
		}
	};

	auto price_of = [&](const auto& of) {
		if constexpr(std::is_same_v<decltype(of), const entity_flavour_id&>) {
			return cosm.on_flavour(of, [&](const auto& typed_flavour) {
				return typed_flavour.template find<invariants::item>()->standard_price;
			});
		}
		else {
			return *cosm[of].find_price();
		}
	};

	auto entity_button = [&](const auto& o, const std::string& label = "#ownedimage") {
		if (const auto& entry = in.images_in_atlas.at(image_of(o)).diffuse; entry.exists()) {
			return game_image_button(label, entry);
		}

		return false;
	};

	auto price_comparator = [&](const auto& a, const auto& b) {
		const auto pa = price_of(a);
		const auto pb = price_of(b);

		if (pa == pb) {
			return image_of(a).indirection_index < image_of(b).indirection_index;
		}

		return pa > pb;
	};

	if (subject.alive()) {
		subject.for_each_contained_item_recursive([&](const auto& typed_item) {
			/* Skip bombs, personal deposits and zero-price items which are considered non-buyable */

			if (typed_item.get_current_slot().get_type() == slot_function::PERSONAL_DEPOSIT) {
				return;
			}

			if (const auto price = typed_item.find_price(); price == std::nullopt || *price == 0) {
				return;
			}

			if (const auto fuse = typed_item.template find<invariants::hand_fuse>()) {
				if (fuse->is_like_plantable_bomb()) {
					return;
				}
			}

			owned_items.push_back(typed_item);
		});

		sort_range(owned_items, price_comparator);

		money_type total = 0;

		for (const auto& o_id : owned_items) {
			total += price_of(o_id);
		}

		text(typesafe_sprintf("Equipment value: %x$", total));
		text("Owned items:");

		for (const auto o_id : owned_items) {
			ImGui::SameLine();
			entity_button(o_id);
		}

		ImGui::Separator();
	}

	result_type result;

	if (in.is_in_buy_zone) {
		if (current_menu == buy_menu_type::MAIN) {
			text("Buy ammo:");

			std::unordered_set<entity_flavour_id> processed_flavours;

			for (const auto o_id : owned_items) {
				if (const auto o = cosm[o_id]) {
					{
						const auto flavour =  o.get_flavour_id();

						if (found_in(processed_flavours, flavour)) {
							continue;
						}

						emplace_element(processed_flavours, flavour);
					}

					if (const auto mag_slot = o[slot_function::GUN_DETACHABLE_MAGAZINE]) {
						if (const auto& f = mag_slot->only_allow_flavour; f.is_set()) {
							const bool choice_was_made = entity_button(entity_flavour_id(f));

							if (choice_was_made) {
								mode_commands::item_purchase msg;
								msg.flavour_id = f;
								result = msg;
							}
						}
					}
				}
			}

			ImGui::Separator();

			augs::for_each_enum_except_bounds([&](const buy_menu_type e) {
				if (e == buy_menu_type::MAIN) {
					return;
				}

				const auto label = format_enum(e);

				if (ImGui::Button(label.c_str())) {
					current_menu = e;
				}
			});
		}
		else {
			if (ImGui::Button("Back")) {
				current_menu = buy_menu_type::MAIN;
			}

			auto do_item_menu = [&](
				const std::optional<item_holding_stance> considered_stance,
				auto&& foreach
			) {
				std::vector<entity_flavour_id> buyable_items;

				foreach([&](const auto& id, const auto& flavour) {
					if (considered_stance != std::nullopt) {
						auto item = flavour.template find<invariants::item>();

						if (item->holding_stance != *considered_stance) {
							return;
						}
					}

					buyable_items.push_back(id);
				});

				sort_range(buyable_items, price_comparator);

				for (const auto& b : reverse(buyable_items)) {
					if (entity_button(b)) {
						mode_commands::item_purchase msg;
						msg.flavour_id = b;
						result = msg;
					}
				}
			};

			auto for_each_gun = [&](auto&& callback) {
				cosm.for_each_flavour_having<invariants::gun>(callback);
			};

			auto for_each_pistol = [&](auto&& callback) {
				cosm.for_each_flavour_having<invariants::gun>([&](const auto& id, const auto& flavour) {
					if (price_of(entity_flavour_id(id)) <= 1000) {
						callback(id, flavour);
					}
				});
			};

			auto for_each_smg = [&](auto&& callback) {
				cosm.for_each_flavour_having<invariants::gun>([&](const auto& id, const auto& flavour) {
					if (price_of(entity_flavour_id(id)) > 1000) {
						callback(id, flavour);
					}
				});
			};

			auto for_each_shotgun = [&](auto&& callback) {
				cosm.for_each_flavour_having<invariants::gun>([&](const auto& id, const auto& flavour) {
					const auto& gun_def = flavour.template get<invariants::gun>();

					if (gun_def.shot_cooldown_ms > 150) {
						/* A heavy gun with such a slow firerate must be a shotgun */
						callback(id, flavour);
					}
				});
			};

			switch (current_menu) {
				case buy_menu_type::PISTOLS: {
					do_item_menu(
						item_holding_stance::PISTOL_LIKE,
						for_each_pistol
					);
					break;
				}

				case buy_menu_type::SUBMACHINE_GUNS: {
					do_item_menu(
						item_holding_stance::PISTOL_LIKE,
						for_each_smg
					);
					break;
				}

				case buy_menu_type::RIFLES: {
					do_item_menu(
						item_holding_stance::RIFLE_LIKE,
						for_each_gun
					);
					break;
				}

				case buy_menu_type::HEAVY_GUNS: {
					do_item_menu(
						item_holding_stance::HEAVY_LIKE,
						for_each_gun
					);
					break;
				}

				case buy_menu_type::SHOTGUNS: {
					do_item_menu(
						item_holding_stance::HEAVY_LIKE,
						for_each_shotgun
					);
					break;
				}

				default: break;
			}
		}
	}
	else {
		text_color("You are not in the designated buy area!", red);
	}

	return result;
}