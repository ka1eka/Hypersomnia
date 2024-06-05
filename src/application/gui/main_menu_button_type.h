#pragma once

enum class main_menu_button_type {
	// GEN INTROSPECTOR enum class main_menu_button_type
	JOIN_DISCORD,
	AVAILABLE_ON_GITHUB,
#if PLATFORM_WEB
	DOWNLOAD_ON_STEAM,
#endif
	DOWNLOAD_MAPS,
	PLAY_RANKED,
	BROWSE_SERVERS,
	HOST_SERVER,
	CONNECT_TO_SERVER,
	SHOOTING_RANGE,
	TUTORIAL,
	EDITOR,
	SETTINGS,
	CREDITS,
#if !PLATFORM_WEB
	QUIT,
#endif

	COUNT
	// END GEN INTROSPECTOR
};