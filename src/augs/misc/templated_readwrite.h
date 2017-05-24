#pragma once
#include <type_traits>
#include "augs/ensure.h"

#include "augs/templates/memcpy_safety.h"
#include "augs/templates/type_matching_and_indexing.h"
#include "augs/templates/introspect.h"
#include "augs/templates/container_traits.h"

namespace augs {
	class output_stream_reserver;
	class stream;

	template <class Archive>
	struct is_native_binary_stream 
		: std::bool_constant<
			is_one_of_v<
				std::decay_t<Archive>,
				augs::stream, 
				augs::output_stream_reserver, 
				std::ifstream, 
				std::ofstream
			>
		>
	{
		static_assert(!std::is_const_v<std::remove_reference_t<Archive>>, "Non-modifiable archives are ill-formed.");
	};

	template <class Archive>
	constexpr bool is_native_binary_stream_v = is_native_binary_stream<Archive>::value;

	template <class Archive, class Serialized, class = void>
	struct has_io_overloads 
		: std::false_type 
	{

	};

	template <class Archive, class Serialized>
	struct has_io_overloads <
		Archive,
		Serialized,
		decltype(
			read_object(
				std::declval<Archive>(),
				std::declval<Serialized>()
			),
			write_object(
				std::declval<Archive>(),
				std::declval<const Serialized>()
			),
			void()
		)
	> : std::true_type {
	
	};

	template <class Archive, class Serialized>
	constexpr bool has_io_overloads_v = has_io_overloads<Archive, Serialized>::value;

	template<class Archive, class Serialized>
	struct is_byte_io_safe 
		: std::bool_constant<is_native_binary_stream_v<Archive> && is_memcpy_safe_v<Serialized>> 
	{

	};

	template<class Archive, class Serialized>
	constexpr bool is_byte_io_safe_v = is_byte_io_safe<Archive, Serialized>::value;

	template<class Archive, class Serialized>
	struct is_byte_io_appropriate 
		: std::bool_constant<is_byte_io_safe_v<Archive, Serialized> && !has_io_overloads_v<Archive, Serialized>>
	{

	};

	template<class Archive, class Serialized>
	constexpr bool is_byte_io_appropriate_v = is_byte_io_appropriate<Archive, Serialized>::value;

	template<class Archive, class Serialized>
	void verify_byte_io_safety() {
		static_assert(is_memcpy_safe_v<Serialized>, "Attempt to serialize a non-trivially copyable type");
		static_assert(is_native_binary_stream_v<Archive>, "Byte serialization of trivial structs allowed only on native binary archives");
	}
	
	/*
		Explanation of augs::read/augs::write overloads:

		if the type has augs::read_object and augs::write_object overloads
			call these overloads
		else if the type is trivial and the stream is a native binary stream
			bytewise read/write via std::memcpy
		else if the type is a container with elements contiguous in memory
			call special augs::read_n and augs::write_n that offer potential optimization
		else if the type is other container
			call augs::read and augs::write upon all contained objects
		else
			assert that the type has introspectors
			call augs::read and augs::write upon all members
	*/

	template<class Archive, class Serialized>
	void read_bytes(
		Archive& ar, 
		Serialized* const location, 
		const size_t object_count
	) {
		verify_byte_io_safety<Archive, Serialized>();
		ar.read(reinterpret_cast<char*>(location), object_count * sizeof(Serialized));
	}

	template<class Archive, class Serialized>
	void write_bytes(
		Archive& ar, 
		const Serialized* const location, 
		const size_t object_count
	) {
		verify_byte_io_safety<Archive, Serialized>();
		ar.write(reinterpret_cast<const char*>(location), object_count * sizeof(Serialized));
	}

	template<class Archive, class Serialized>
	void read(
		Archive& ar,
		Serialized& storage,
		const std::enable_if_t<has_io_overloads_v<Archive, Serialized>>* const dummy = nullptr
	) {
		read_object(ar, storage);
	}

	template<class Archive, class Serialized>
	void write(
		Archive& ar,
		const Serialized& storage,
		const std::enable_if_t<has_io_overloads_v<Archive, Serialized>>* const dummy = nullptr
	) {
		write_object(ar, storage);
	}

	template<class Archive, class Serialized>
	void read(
		Archive& ar,
		Serialized& storage,
		const std::enable_if_t<is_byte_io_appropriate_v<Archive, Serialized>>* const dummy = nullptr
	) {
		read_bytes(ar, &storage, 1);
	}

	template<class Archive, class Serialized>
	void write(
		Archive& ar,
		const Serialized& storage,
		const std::enable_if_t<is_byte_io_appropriate_v<Archive, Serialized>>* const dummy = nullptr
	) {
		write_bytes(ar, &storage, 1);
	}

	template<class Archive, class Serialized>
	void read(
		Archive& ar,
		Serialized& storage,
		const std::enable_if_t<
			!has_io_overloads_v<Archive, Serialized> 
			&& !is_byte_io_safe_v<Archive, Serialized>
			&& !has_value_type_v<Serialized>
		>* const dummy = nullptr
	) {
		static_assert(has_introspect_v<Serialized>, "Attempt to read a type in a non-bytesafe context without i/o overloads and without introspectors!");

		augs::introspect(
			[&](auto, auto& member) {
				read(ar, member);
			},
			storage
		);
	}

	template<class Archive, class Serialized>
	void write(
		Archive& ar,
		const Serialized& storage,
		const std::enable_if_t<
			!has_io_overloads_v<Archive, Serialized> 
			&& !is_byte_io_safe_v<Archive, Serialized>
			&& !has_value_type_v<Serialized>
		>* const dummy = nullptr
	) {
		static_assert(has_introspect_v<Serialized>, "Attempt to write a type in a non-bytesafe context without i/o overloads and without introspectors!");

		augs::introspect(
			[&](auto, const auto& member) {
				write(ar, member);
			},
			storage
		);
	}

	template<class Archive, class Serialized>
	void read_n(
		Archive& ar,
		Serialized* const storage,
		const size_t n,
		const std::enable_if_t<is_byte_io_appropriate_v<Archive, Serialized>>* const dummy = nullptr
	) {
		read_bytes(ar, storage, n);
	}

	template<class Archive, class Serialized>
	void write_n(
		Archive& ar,
		const Serialized* const storage,
		const size_t n,
		const std::enable_if_t<is_byte_io_appropriate_v<Archive, Serialized>>* const dummy = nullptr
	) {
		write_bytes(ar, storage, n);
	}

	template<class Archive, class Serialized>
	void read_n(
		Archive& ar,
		Serialized* const storage,
		const size_t n,
		const std::enable_if_t<!is_byte_io_appropriate_v<Archive, Serialized>>* const dummy = nullptr
	) {
		for (size_t i = 0; i < n; ++i) {
			read(ar, storage[i]);
		}
	}

	template<class Archive, class Serialized>
	void write_n(
		Archive& ar,
		const Serialized* const storage,
		const size_t n,
		const std::enable_if_t<!is_byte_io_appropriate_v<Archive, Serialized>>* const dummy = nullptr
	) {
		for (size_t i = 0; i < n; ++i) {
			write(ar, storage[i]);
		}
	}

	/*
		Special i/o for containers whose memory is contiguous (e.g. strings and vectors)
		We call read_n/write_n that can optimize for trivial elements continuous in memory
	*/

	template <class Archive, class Container, class container_size_type = std::size_t>
	void read_object(
		Archive& ar, 
		Container& storage, 
		container_size_type = container_size_type(),
		const std::enable_if_t<can_access_data_v<Container>>* const dummy = nullptr
	) {
		container_size_type s;
		read(ar, s);

		storage.resize(s);
		read_n(ar, storage.data(), storage.size());
	}

	template<class Archive, class Container, class container_size_type = std::size_t>
	void write_object(
		Archive& ar, 
		const Container& storage, 
		container_size_type = container_size_type(),
		const std::enable_if_t<can_access_data_v<Container>>* const dummy = nullptr
	) {
		ensure(storage.size() <= std::numeric_limits<container_size_type>::max());

		write(ar, static_cast<container_size_type>(storage.size()));
		write_n(ar, storage.data(), storage.size());
	}

	/*
		Utility functions
	*/

	template<class Archive, class Container, class...>
	void read_with_capacity(Archive& ar, Container& storage) {
		size_t c;
		size_t s;

		read(ar, c);
		read(ar, s);

		storage.reserve(c);
		storage.resize(s);

		read_n(ar, storage.data(), storage.size());
	}

	template<class Archive, class Container>
	void write_with_capacity(Archive& ar, const Container& storage) {
		write(ar, storage.capacity());
		write(ar, storage.size());

		write_n(ar, storage.data(), storage.size());
	}

	/*
		General i/o for containers without memory contiguity of elements
	*/

	template <class Serialized>
	void reserve_num_elements(Serialized& container, const size_t n) {
		container.reserve(n);
	}

	template <class Key, class Value>
	void reserve_num_elements(std::map<Key, Value>& container, const size_t) {

	}

	template <class Archive, class Container, class container_size_type = std::size_t>
	void read_object(
		Archive& ar,
		Container& storage,
		container_size_type = container_size_type(),
		const std::enable_if_t<!can_access_data_v<Container> && has_value_type_v<Container>>* const dummy = nullptr
	) {
		container_size_type s;

		read(ar, s);

		reserve_num_elements(storage, s);

		while (s--) {
			typename Container::value_type val;
			read(ar, val);
			storage.emplace(std::move(val));
		}
	}

	template<class Archive, class Container, class container_size_type = std::size_t>
	void write_object(
		Archive& ar,
		const Container& storage,
		container_size_type = container_size_type(),
		const std::enable_if_t<!can_access_data_v<Container> && has_value_type_v<Container>>* const dummy = nullptr
	) {
		ensure(storage.size() <= std::numeric_limits<container_size_type>::max());

		write(ar, static_cast<container_size_type>(storage.size()));

		for (const auto& obj : storage) {
			write(ar, obj);
		}
	}

	template<class Archive, size_t count>
	void read_flags(Archive& ar, std::array<bool, count>& storage) {
		static_assert(count > 0, "Can't read a null array");

		std::array<char, (count - 1) / 8 + 1> compressed_storage;
		read(ar, compressed_storage);

		for (size_t bit = 0; bit < count; ++bit) {
			storage[bit] = (compressed_storage[bit / 8] >> (bit % 8)) & 1;
		}
	}

	template<class Archive, size_t count>
	void write_flags(Archive& ar, const std::array<bool, count>& storage) {
		static_assert(count > 0, "Can't write a null array");

		std::array<char, (count - 1) / 8 + 1> compressed_storage;
		std::fill(compressed_storage.begin(), compressed_storage.end(), 0);

		for (size_t bit = 0; bit < count; ++bit) {
			if (storage[bit]) {
				compressed_storage[bit / 8] |= 1 << (bit % 8);
			}
		}

		write(ar, compressed_storage);
	}

	template<class Archive, class Serialized>
	void read_members_from_istream(
		Archive& ar,
		Serialized& storage
	) {
		augs::introspect_recursive<
			bind_types_t<can_stream_right, Archive>,
			is_not_predicate_t<padding_byte>,
			stop_recursion_if_valid
		>(
			[&](auto, auto& member) {
				ar >> member;
			},
			storage
		);
	}
}