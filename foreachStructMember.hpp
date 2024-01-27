#ifndef FOREACH_STRUCT_MEMBER_HPP
#define FOREACH_STRUCT_MEMBER_HPP

#include <cstddef>

namespace Detail {
#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif

// Forward declare ADL functions we will fill later
template <typename Parent, int Index, typename Visitor>
struct StructInfoGetter {
	friend void callOnMember(StructInfoGetter<Parent, Index, Visitor>, Parent& instance, std::ptrdiff_t offset, const Visitor& visitor);
	friend constexpr int getSize(StructInfoGetter<Parent, Index, Visitor>);
	friend constexpr int getAlignment(StructInfoGetter<Parent, Index, Visitor>);
};

// Instantiating this template will define the forward-declared functions
template <typename Parent, int ChildIndex, typename Visitor, typename ChildType>
struct StructInfoSaver {
	friend void callOnMember(StructInfoGetter<Parent, ChildIndex, Visitor>, Parent& instance, std::ptrdiff_t offset, const Visitor& visitor) {
		visitor(*reinterpret_cast<ChildType*>(reinterpret_cast<char*>(&instance) + offset));
	}
	friend constexpr int getSize(StructInfoGetter<Parent, ChildIndex, Visitor>) {
		return sizeof(ChildType);
	}
	friend constexpr int getAlignment(StructInfoGetter<Parent, ChildIndex, Visitor>) {
		return alignof(ChildType);
	}
	constexpr static bool instantiated = true;
};

// Can convert to anything, compiling a conversion to something will instantiate the template with the right type
template<typename T, typename Visitor, int N>
struct ObjectInspector {
	template <typename Inspected, bool okay = StructInfoSaver<T, N, Visitor, Inspected>::instantiated>
	operator Inspected() {
		return Inspected{};
	}
};

// Recursion base
template <typename T, typename sfinae, typename Visitor, std::size_t... indexes>
struct MemberCounter {
	constexpr static std::size_t get() {
		return sizeof...(indexes) - 1;
	}
};

// Try to iteratively initialise the struct with more and more arguments, all of which convert and instantiate the templates
template <typename T, typename Visitor, std::size_t... indexes>
struct MemberCounter<T, decltype( T { ObjectInspector<T, Visitor, indexes>()...} )*, Visitor, indexes...> {
	constexpr static std::size_t get() {
		return MemberCounter<T, T*, Visitor, indexes..., sizeof...(indexes)>::get();
	}
};

// Iterates through elements in the struct, calculating the position of the next element using alignment rules
template <std::size_t Index, std::size_t End, ptrdiff_t Offset, std::size_t MaxAlign, typename T, typename Visitor>
void foreachMemberRange(T& object, const Visitor& callee) {
	if constexpr(Index < End) {
		// Find the offset of the member variable and call the functor on it
		constexpr std::size_t alignment = getAlignment(StructInfoGetter<T, Index, Visitor>{});
		constexpr std::size_t newMaxAlign = (alignment > MaxAlign) ? alignment : MaxAlign;
		constexpr std::size_t aligned = (Offset % alignment == 0) ? Offset : Offset - (Offset % alignment) + alignment;
		callOnMember(StructInfoGetter<T, Index, Visitor>(), object, aligned, callee);

		// Continue recursion
		constexpr std::size_t nextOffset = aligned + getSize(StructInfoGetter<T, Index, Visitor>{});
		foreachMemberRange<Index + 1, End, nextOffset, newMaxAlign, T, Visitor>(object, callee);
	} else {
		// Check if the size is as expected when reaching the end
		constexpr std::size_t expectedSize = (Offset % MaxAlign == 0) ? Offset : Offset - (Offset % MaxAlign) + MaxAlign;
		constexpr std::size_t expectedSizeCorrected = (expectedSize > 0) ? expectedSize : 1;
		static_assert(sizeof(T) == expectedSizeCorrected, "Reflection failed, make sure the type is aggregate initialisable");
	}
}

#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic pop
#endif
} // namespace Detail

template <typename T, typename Visitor>
void foreachStructMember(T& object, const Visitor& callee) {
	constexpr std::size_t size = Detail::MemberCounter<T, T*, Visitor>::get();
	static_assert(size != -1, "Reflection failed, make sure the type is aggregate initialisable");
	Detail::foreachMemberRange<0, size, 0, 1>(object, callee);
}

#endif // FOREACH_STRUCT_MEMBER_HPP
