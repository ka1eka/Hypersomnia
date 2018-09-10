#pragma once

enum class capability_relation {
	UNMATCHING,
	THE_SAME,
	PICKUP,
	STORING_DROP,
	DROP,
	ANONYMOUS_TRANSFER,
	ANONYMOUS_DROP
};

enum class containment_result_type {
	INVALID_RESULT,
	THE_SAME_SLOT,
	TOO_MANY_ITEMS,
	INCOMPATIBLE_CATEGORIES,
	INSUFFICIENT_SPACE,

	SUCCESSFUL_REPLACE,
	SUCCESSFUL_CONTAINMENT
};

enum class item_transfer_result_type {
	// GEN INTROSPECTOR enum class item_transfer_result_type
	INVALID_RESULT,

	INVALID_CAPABILITIES,

	TOO_MANY_ITEMS,

	INCOMPATIBLE_CATEGORIES,
	INSUFFICIENT_SPACE,
	THE_SAME_SLOT,

	MOUNTING_CONDITIONS_NOT_MET,

	SUCCESSFUL_TRANSFER
	// END GEN INTROSPECTOR
};