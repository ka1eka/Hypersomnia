#include "window_framework/window.h"
#include "texture_baker/texture_baker.h"

#include "entity_system/world.h"
#include "entity_system/entity.h"

#include <lua/lua.hpp>
#include <luabind/luabind.hpp>
#include "script_system.h"

#include "../components/animate_component.h"
#include "../components/camera_component.h"
#include "../components/chase_component.h"
#include "../components/children_component.h"
#include "../components/crosshair_component.h"
#include "../components/damage_component.h"
#include "../components/gun_component.h"
#include "../components/health_component.h"
#include "../components/input_component.h"
#include "../components/lookat_component.h"
#include "../components/movement_component.h"
#include "../components/particle_emitter_component.h"
#include "../components/particle_group_component.h"
#include "../components/physics_component.h"
#include "../components/scriptable_component.h"
#include "../components/transform_component.h"
#include "../components/render_component.h"

#include "../game/body_helper.h"
#include "../game/texture_helper.h"

#include "../resources/animate_info.h"
#include "../resources/scriptable_info.h"

namespace luabind
{
	template <>
	struct default_converter<std::wstring>
		: native_converter_base<std::wstring>
	{
		static int compute_score(lua_State * s, int index) {
			return lua_type(s, index) == LUA_TSTRING ? 0 : -1;
		}

		std::wstring from(lua_State * s, int index) {
			std::string utf8str(lua_tostring(s, index));
			return std::wstring(utf8str.begin(), utf8str.end());
		}

		void to(lua_State * s, const std::wstring & str) {
			lua_pushstring(s, std::string(str.begin(), str.end()).c_str());
		}
	};
	/**/

	template <>
	struct default_converter<const std::wstring>
		: default_converter<std::wstring>
	{};

	template <>
	struct default_converter<const std::wstring&>
		: default_converter<std::wstring>
	{};
}

script_system::script_system() : lua_state(luaL_newstate()) {
	using namespace resources;
	using namespace topdown;

	luabind::open(lua_state);

	luaopen_base(lua_state);
	luaopen_io(lua_state);
	luabind::module(lua_state)[
		luabind::class_<std::pair<float, float>>("minmax_f")
			.def(luabind::constructor<float, float>())
			.def_readwrite("min", &std::pair<float, float>::first)
			.def_readwrite("max", &std::pair<float, float>::second),

		luabind::class_<std::pair<unsigned, unsigned>>("minmax_uint")
			.def(luabind::constructor<unsigned, unsigned>())
			.def_readwrite("min", &std::pair<unsigned, unsigned>::first)
			.def_readwrite("max", &std::pair<unsigned, unsigned>::second),

		luabind::class_<vec2<>>("vec2")
			.def(luabind::constructor<float, float>())
			.def(luabind::constructor<>())
			.def_readwrite("x", &vec2<>::x)
			.def_readwrite("y", &vec2<>::y),

		luabind::class_<graphics::pixel_32>("rgba")
			.def_readwrite("r", &graphics::pixel_32::r)
			.def_readwrite("g", &graphics::pixel_32::g)
			.def_readwrite("b", &graphics::pixel_32::b)
			.def_readwrite("a", &graphics::pixel_32::a),

		luabind::class_<rects::ltrb>("rect_ltrb")
			.def(luabind::constructor<int, int, int, int>())
			.def_readwrite("l", &rects::ltrb::l)
			.def_readwrite("t", &rects::ltrb::t)
			.def_readwrite("r", &rects::ltrb::r)
			.def_readwrite("b", &rects::ltrb::b)
			.property("w", (int (rects::ltrb::*)() const)&rects::ltrb::w, (void (rects::ltrb::*)(int) ) &rects::ltrb::w)
			.property("h", (int (rects::ltrb::*)() const)&rects::ltrb::h, (void (rects::ltrb::*)(int) ) &rects::ltrb::h),

		luabind::class_<rects::xywh>("rect_xywh")
			.def(luabind::constructor<int, int, int, int>())
			.def_readwrite("x", &rects::xywh::x)
			.def_readwrite("y", &rects::xywh::y)
			.def_readwrite("w", &rects::xywh::w)
			.def_readwrite("h", &rects::xywh::h)
			.property("r", (int (rects::xywh::*)() const)&rects::xywh::r, (void (rects::xywh::*)(int) ) &rects::xywh::r)
			.property("b", (int (rects::xywh::*)() const)&rects::xywh::b, (void (rects::xywh::*)(int) ) &rects::xywh::b),

		luabind::class_<window::glwindow>("glwindow")
			.def(luabind::constructor<>())
		,

		luabind::class_<texture_baker::atlas>("atlas")
			.def(luabind::constructor<>())
			.def("build", &texture_baker::atlas::default_build),

		luabind::class_<script>("script")
			.def(luabind::constructor<>())
			.def("associate_filename", (void (script::*)(const std::string&))&script::associate_filename)
			.def("call", &script::call),

		luabind::class_<texture_helper>("texture")
			.def(luabind::constructor<std::wstring, texture_baker::atlas&>()),

		luabind::class_<animation>("animation")
			.def(luabind::constructor<>())
			.def("add_frame", &animation::add_frame)
			.def_readwrite("loop_mode", &animation::loop_mode),

		luabind::class_<world>("world")
			.def(luabind::constructor<>())
			.def("create_entity", &world::create_entity)
			.def("delete_entity", &world::delete_entity),
			
		luabind::class_<entity_ptr>("entity_ptr")
			.def(luabind::constructor<>())
			.def("get", &entity_ptr::get)
			.def("set", &entity_ptr::set),

		luabind::class_<sprite>("sprite")
			.def_readwrite("mask", &sprite::mask)
			.def_readwrite("layer", &sprite::layer)
			.def_readwrite("size", &sprite::size)
			.def_readwrite("image", &sprite::tex)
			.def_readwrite("color", &sprite::color)
			.def("update_size", &sprite::update_size)
			.enum_("mask_type")[
				luabind::value("WORLD", sprite::mask_type::WORLD),
					luabind::value("GUI", sprite::mask_type::GUI)
			],

		luabind::class_<components::render>("render_component")
			.def(luabind::constructor<>())
			.property("info", &components::render::get_render_info<sprite>, &components::render::set_render_info<sprite>)
		,
		
		luabind::class_<components::transform::state>("transform_state")
			.def(luabind::constructor<>())
			.def_readwrite("pos", &components::transform::state::pos)
			.property("rotation", &components::transform::state::get_rotation, &components::transform::state::set_rotation)
		,
		
		luabind::class_<components::transform>("transform_component")
			.def(luabind::constructor<>())
			.def_readwrite("current", &components::transform::current)
			.def_readwrite("previous", &components::transform::previous)
		,

		luabind::class_<components::animate::subscribtion>("animation_subscribtion")
			.def(luabind::constructor<>())
			.def("add", &components::animate::subscribtion::insert),

		luabind::class_<components::animate>("animate_component")
		.def(luabind::constructor<>())
			.def_readwrite("available_animations", &components::animate::available_animations),
			
		luabind::class_<components::camera>("camera_component")
			.def(luabind::constructor<>())
			.def_readwrite("screen_rect", &components::camera::screen_rect)
			.def_readwrite("ortho", &components::camera::ortho)
			.def_readwrite("layer", &components::camera::layer)
			.def_readwrite("mask", &components::camera::mask)
			.def_readwrite("enabled", &components::camera::enabled)
			.def_readwrite("orbit_mode", &components::camera::orbit_mode)
			.def_readwrite("angled_look_length", &components::camera::angled_look_length)
			.def_readwrite("enable_smoothing", &components::camera::enable_smoothing)
			.def_readwrite("smoothing_average_factor", &components::camera::smoothing_average_factor)
			.def_readwrite("averages_per_sec", &components::camera::averages_per_sec)
			.def_readwrite("max_look_expand", &components::camera::max_look_expand)
			.def_readwrite("player", &components::camera::player)
			.def_readwrite("crosshair", &components::camera::crosshair)
			.enum_("orbit_type")[
				luabind::value("NONE", components::camera::orbit_type::NONE),
				luabind::value("ANGLED", components::camera::orbit_type::ANGLED),
				luabind::value("LOOK", components::camera::orbit_type::LOOK)
			]
			,

			luabind::class_<components::chase>("chase_component")
		.def(luabind::constructor<>())
			.def_readwrite("target", &components::chase::target)
			.def_readwrite("type", &components::chase::type)
			.def_readwrite("offset", &components::chase::offset)
			.def_readwrite("rotation_orbit_offset", &components::chase::rotation_orbit_offset)
			.def_readwrite("rotation_offset", &components::chase::rotation_offset)
			.def_readwrite("relative", &components::chase::relative)
			.def_readwrite("chase_rotation", &components::chase::chase_rotation)
			.def_readwrite("track_origin", &components::chase::track_origin)
			.enum_("chase_type")[
				luabind::value("OFFSET", components::chase::chase_type::OFFSET),
				luabind::value("ORBIT", components::chase::chase_type::ORBIT)
			]
			,

			luabind::class_<components::children>("children_component")
			.def(luabind::constructor<>())
			.def("add", &components::children::add),

			luabind::class_<components::crosshair>("crosshair_component")
		.def(luabind::constructor<>())
			//.def_readwrite("bounds", &components::crosshair::bounds)
			//.def_readwrite("blink", &components::crosshair::blink)
			.def_readwrite("should_blink", &components::crosshair::should_blink)
			.def_readwrite("sensitivity", &components::crosshair::sensitivity),

		luabind::class_<components::damage>("damage_component")
			.def(luabind::constructor<>())
			.def_readwrite("amount", &components::damage::amount)
			.def_readwrite("sender", &components::damage::sender)
			.def_readwrite("starting_point", &components::damage::starting_point)
			.def_readwrite("max_distance", &components::damage::max_distance),
			
		luabind::class_<b2Filter>("b2Filter")
			.def_readwrite("categoryBits", &b2Filter::categoryBits)
			.def_readwrite("maskBits", &b2Filter::maskBits)
			.def_readwrite("groupIndex", &b2Filter::groupIndex),
			
		luabind::class_<components::gun::gun_info>("gun_info")
			.def(luabind::constructor<>())
			.def_readwrite("bullets_once", &components::gun::gun_info::bullets_once)
			.def_readwrite("max_rounds", &components::gun::gun_info::max_rounds)
			.def_readwrite("spread_radians", &components::gun::gun_info::spread_radians)
			.def_readwrite("bullet_min_damage", &components::gun::gun_info::bullet_min_damage)
			.def_readwrite("bullet_max_damage", &components::gun::gun_info::bullet_max_damage)
			.def_readwrite("bullet_speed", &components::gun::gun_info::bullet_speed)
			.def_readwrite("shooting_interval_ms", &components::gun::gun_info::shooting_interval_ms)
			.def_readwrite("velocity_variation", &components::gun::gun_info::velocity_variation)
			.def_readwrite("max_bullet_distance", &components::gun::gun_info::max_bullet_distance)
			.def_readwrite("bullet_distance_offset", &components::gun::gun_info::bullet_distance_offset)
			.def_readwrite("shake_radius", &components::gun::gun_info::shake_radius)
			.def_readwrite("shake_spread_radians", &components::gun::gun_info::shake_spread_radians)
			.def_readwrite("is_automatic", &components::gun::gun_info::is_automatic)
			.def_readwrite("bullet_layer", &components::gun::gun_info::bullet_layer)
			.def_readwrite("bullet_collision_filter", &components::gun::gun_info::bullet_collision_filter),
			
		luabind::class_<components::gun>("gun_component")
		.def(luabind::constructor<>())
			.def_readwrite("info", &components::gun::info)
			.def_readwrite("current_rounds", &components::gun::current_rounds)
			.def_readwrite("reloading", &components::gun::reloading)
			.def_readwrite("trigger", &components::gun::trigger)
			.def_readwrite("target_camera_to_shake", &components::gun::target_camera_to_shake),
	
		luabind::class_<components::health::health_info>("health_info")
			.def(luabind::constructor<>())
			.def_readwrite("death_render", &components::health::health_info::death_render)
			.def_readwrite("max_hp", &components::health::health_info::max_hp)
			.def_readwrite("should_disappear", &components::health::health_info::should_disappear)
			.def_readwrite("dead_lifetime_ms", &components::health::health_info::dead_lifetime_ms)
			.def_readwrite("corpse_collision_filter", &components::health::health_info::corpse_collision_filter),

		luabind::class_<components::health>("health_component")
		.def(luabind::constructor<>())
			.def_readwrite("info", &components::health::info)
			.def_readwrite("hp", &components::health::hp)
			.def_readwrite("dead", &components::health::dead),


		luabind::class_<messages::intent_message>("intent_message")
		.def(luabind::constructor<>())
		.enum_("intent_type")[
		luabind::value("MOVE_FORWARD", messages::intent_message::intent_type::MOVE_FORWARD),
			luabind::value("MOVE_BACKWARD", messages::intent_message::intent_type::MOVE_BACKWARD),
			luabind::value("MOVE_LEFT", messages::intent_message::intent_type::MOVE_LEFT),
			luabind::value("MOVE_RIGHT", messages::intent_message::intent_type::MOVE_RIGHT),
			luabind::value("SHOOT", messages::intent_message::intent_type::SHOOT),
			luabind::value("AIM", messages::intent_message::intent_type::AIM),
			luabind::value("SWITCH_LOOK", messages::intent_message::intent_type::SWITCH_LOOK),
			luabind::value("SWITCH_WEAPON", messages::intent_message::intent_type::SWITCH_WEAPON)
		],

		luabind::class_<components::input>("input_component")
			.def(luabind::constructor<>())
			.def("add", &components::input::add),
			luabind::class_<components::lookat>("lookat_component")
			.def(luabind::constructor<>())
			.def_readwrite("look_mode", &components::lookat::look_mode)
			.def_readwrite("target", &components::lookat::target)
			.enum_("chase_type")[
				luabind::value("OFFSET", components::lookat::look_type::POSITION),
					luabind::value("ORBIT", components::lookat::look_type::VELOCITY)
			],

		luabind::class_<components::movement>("movement_component")
		.def(luabind::constructor<>())
			.def("add_animation_receiver", &components::movement::add_animation_receiver)
			.def_readwrite("moving_left", &components::movement::moving_left)
			.def_readwrite("moving_right", &components::movement::moving_right)
			.def_readwrite("moving_forward", &components::movement::moving_forward)
			.def_readwrite("moving_backward", &components::movement::moving_backward)
			.def_readwrite("acceleration", &components::movement::acceleration)
			.def_readwrite("max_speed", &components::movement::max_speed),
			
		luabind::class_<components::particle_emitter::particle>("particle")
			.def(luabind::constructor<>())
			.def_readwrite("pos", &components::particle_emitter::particle::pos)
			.def_readwrite("vel", &components::particle_emitter::particle::vel)
			.def_readwrite("acc", &components::particle_emitter::particle::acc)
			.def_readwrite("face", &components::particle_emitter::particle::face)
			.def_readwrite("rotation", &components::particle_emitter::particle::rotation)
			.def_readwrite("rotation_speed", &components::particle_emitter::particle::rotation_speed)
			.def_readwrite("linear_damping", &components::particle_emitter::particle::linear_damping)
			.def_readwrite("angular_damping", &components::particle_emitter::particle::angular_damping)
			.def_readwrite("lifetime_ms", &components::particle_emitter::particle::lifetime_ms)
			.def_readwrite("max_lifetime_ms", &components::particle_emitter::particle::max_lifetime_ms)
			.def_readwrite("should_disappear", &components::particle_emitter::particle::should_disappear),


		luabind::class_<components::particle_emitter::emission>("emission")
			.def(luabind::constructor<>())
			.def_readwrite("type", &components::particle_emitter::emission::type)
			.def_readwrite("spread_radians", &components::particle_emitter::emission::spread_radians)
			.def_readwrite("velocity", &components::particle_emitter::emission::velocity)
			.def_readwrite("angular_velocity", &components::particle_emitter::emission::angular_velocity)
			.def_readwrite("particles_per_sec", &components::particle_emitter::emission::particles_per_sec)
			.def_readwrite("stream_duration_ms", &components::particle_emitter::emission::stream_duration_ms)
			.def_readwrite("particle_lifetime_ms", &components::particle_emitter::emission::particle_lifetime_ms)
			.def_readwrite("size_multiplier", &components::particle_emitter::emission::size_multiplier)
			.def_readwrite("acceleration", &components::particle_emitter::emission::acceleration)
			.def_readwrite("particles_per_burst", &components::particle_emitter::emission::particles_per_burst)
			.def_readwrite("initial_rotation_variation", &components::particle_emitter::emission::initial_rotation_variation)
			.def_readwrite("randomize_acceleration", &components::particle_emitter::emission::randomize_acceleration)
			.def_readwrite("offset", &components::particle_emitter::emission::offset)
			.def_readwrite("angular_offset", &components::particle_emitter::emission::angular_offset)
			.def_readwrite("particle_group_layer", &components::particle_emitter::emission::particle_group_layer)
			.def("add_particle_template", &components::particle_emitter::emission::add_particle_template)
			.enum_("emission_type")[
				luabind::value("BURST", components::particle_emitter::emission::BURST),
				luabind::value("STREAM", components::particle_emitter::emission::STREAM)
			]
			,

		luabind::class_<components::particle_emitter::particle_effect>("particle_effect")
			.def(luabind::constructor<>())
			.def("add", &components::particle_emitter::add),

		luabind::class_<components::particle_emitter::subscribtion>("particle_effect_subscribtion")
			.def(luabind::constructor<>())
			.def("add", &components::particle_emitter::subscribtion::insert),

		luabind::class_<components::particle_emitter>("particle_emitter_component")
		.def(luabind::constructor<>())
		.def(luabind::constructor<components::particle_emitter::subscribtion*>())
			.def_readwrite("available_particle_effects", &components::particle_emitter::available_particle_effects),

			luabind::class_<components::physics>("physics_component")
		.def(luabind::constructor<>())
		.def_readwrite("body", &components::physics::body),

		luabind::class_<components::scriptable>("scriptable_component")
		.def(luabind::constructor<>())
		.def(luabind::constructor<components::scriptable::subscribtion*>())
			.def_readwrite("available_scripts", &components::scriptable::available_scripts)
			.enum_("script_type")[
				luabind::value("COLLISION_MESSAGE", components::scriptable::script_type::COLLISION_MESSAGE),
				luabind::value("DAMAGE_MESSAGE", components::scriptable::script_type::DAMAGE_MESSAGE),
				luabind::value("LOOP", components::scriptable::script_type::LOOP)
			]
			,

		luabind::class_<messages::animate_message>("animate_message")
		.def(luabind::constructor<>())
		.def(luabind::constructor<messages::animate_message::animation, messages::animate_message::type, bool, float>())
		.def_readwrite("animation_type", &messages::animate_message::animation_type)
		.def_readwrite("preserve_state_if_animation_changes", &messages::animate_message::preserve_state_if_animation_changes)
		.def_readwrite("message_type", &messages::animate_message::message_type)
		.def_readwrite("change_animation", &messages::animate_message::change_animation)
		.def_readwrite("change_speed", &messages::animate_message::change_speed)
		.def_readwrite("speed_factor", &messages::animate_message::speed_factor)
		.def_readwrite("animation_priority", &messages::animate_message::animation_priority)
		,

		luabind::class_<entity>("entity")
			.def("clear", &entity::clear)

			.def("add", &entity::add<components::render>)
			.property("render", &entity::find<components::render>, &entity::set<components::render>)
			.def("add", &entity::add<components::transform>)
			.property("transform", &entity::find<components::transform>, &entity::set<components::transform>)
			.def("add", &entity::add<components::animate>)
			.property("animate", &entity::find<components::animate>, &entity::set<components::animate>)
			.def("add", &entity::add<components::camera>)
			.property("camera", &entity::find<components::camera>, &entity::set<components::camera>)
			.def("add", &entity::add<components::chase>)
			.property("chase", &entity::find<components::chase>, &entity::set<components::chase>)
			.def("add", &entity::add<components::children>)
			.property("children", &entity::find<components::children>, &entity::set<components::children>)
			.def("add", &entity::add<components::crosshair>)
			.property("crosshair", &entity::find<components::crosshair>, &entity::set<components::crosshair>)
			.def("add", &entity::add<components::damage>)
			.property("damage", &entity::find<components::damage>, &entity::set<components::damage>)
			.def("add", &entity::add<components::gun>)
			.property("gun", &entity::find<components::gun>, &entity::set<components::gun>)
			.def("add", &entity::add<components::health>)
			.property("health", &entity::find<components::health>, &entity::set<components::health>)
			.def("add", &entity::add<components::input>)
			.property("input", &entity::find<components::input>, &entity::set<components::input>)
			.def("add", &entity::add<components::lookat>)
			.property("lookat", &entity::find<components::lookat>, &entity::set<components::lookat>)
			.def("add", &entity::add<components::movement>)
			.property("movement", &entity::find<components::movement>, &entity::set<components::movement>)
			.def("add", &entity::add<components::particle_emitter>)
			.property("particle_emitter", &entity::find<components::particle_emitter>, &entity::set<components::particle_emitter>)
			.def("add", &entity::add<components::physics>)
			.property("physics", &entity::find<components::physics>, &entity::set<components::physics>)
			.def("add", &entity::add<components::scriptable>)
			.property("scriptable", &entity::find<components::scriptable>, &entity::set<components::scriptable>),

			luabind::def("create_physics_component", topdown::create_physics_component_str)
	];
}

script_system::~script_system() {
	lua_close(lua_state);
}

void script_system::process_entities(world& owner) {


}