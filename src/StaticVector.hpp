#ifndef MXC_STATICVECTOR_HPP
#define MXC_STATICVECTOR_HPP

#include <concepts> // various concepts used 
#include <iterator> // std::contiguous_iterator
#include <cstddef> // std::byte
#include <new> // std::aligned_val_t

namespace mxc
{

// TODO create allocator concept. Start from this taken from https://stackoverflow.com/questions/63147287/how-can-i-combine-several-return-type-requirements-of-c20-constraints-into-one
template<typename P>
concept nullable_pointer =
  	std::regular<P> &&
  	std::convertible_to<std::nullptr_t, P> &&
  	std::assignable_from<P&, std::nullptr_t> &&
  	std::equality_comparable_with<P, std::nullptr_t>;
  

// using this instead of T* to support fancy pointers. You could also use allocator::pointer
template<typename P>
concept allocator_pointer =
  	nullable_pointer<P> &&
  	std::contiguous_iterator<P>;

template<typename A>
concept allocator =
	std::copy_constructible<A> &&
  	std::equality_comparable<A> &&
  	requires(A a)
	{
		{ a.allocate(0u) } -> allocator_pointer;
	};
template <typename A>
concept allocatorAligned = allocator<A> && 
    requires(Allocator<std::byte> alloc, std::align_val_t align) {{alloc.allocateAligned(0u, align)} -> allocator_pointer;})

    // since I am using clang 16 you could technically remove the dummy variable code here, but I'm keeping it for safety
    template <typename T, uint32_t capacity> // a const qualified object cannot be moved from
        requires (!std::is_const_v<T> && (std::copyable<T> && std::movable<T>)) || (std::is_const_v<T> && std::is_copy_constructible_v<T>)
    struct StaticVector
    {
        constexpr StaticVector()
        {
            for (uint32_t i = 0; i != capacity; ++i)
            {
                std::construct_at(&privateData.m_buffer[i].dummy, 0);
            }
            privateData.m_size = 0;
        }
        constexpr StaticVector(T const& singleton)
        {
            std::construct_at(std::addressof(privateData.m_buffer[0].x), singleton);
            for (uint32_t i = 1; i != capacity; ++i)
            {
                std::construct_at(&privateData.m_buffer[i].dummy, 0);
            }
            privateData.m_size = 1;
        }
        constexpr StaticVector(T&& singleton) requires (!std::is_const_v<T>)
        {
            std::construct_at(std::addressof(privateData.m_buffer[0].x), std::move(singleton));
            for (uint32_t i = 1; i != capacity; ++i)
            {
                std::construct_at(&privateData.m_buffer[i].dummy, 0);
            }
            privateData.m_size = 1;
        }
        constexpr StaticVector(std::initializer_list<T> const& list)
        {
            privateData.m_size = static_cast<uint32_t>(list.size());
            uint32_t i = 0;
            for (auto const& item: list)
            {
                std::construct_at(std::addressof<T>(privateData.m_buffer[i++].x), item);
            }
            for (uint32_t i = privateData.m_size; i < capacity; ++i)
            {
                std::construct_at(&privateData.m_buffer[i].dummy, 0);
            }
        }
        constexpr StaticVector(std::initializer_list<T>&& list) requires (!std::is_const_v<T>)
        {
            privateData.m_size = static_cast<uint32_t>(list.size());
            uint32_t i = 0;
            for (auto& item : list)
            {
                std::construct_at(std::addressof<T>(privateData.m_buffer[i++].x), std::move(item));
            }
            for (uint32_t i = privateData.m_size; i < capacity; ++i)
            {
                std::construct_at(&privateData.m_buffer[i].dummy, 0);
            }
        }
        constexpr ~StaticVector() 
        { 
            for (uint32_t i = 0; i != privateData.m_size; ++i)
                std::destroy_at(std::addressof(privateData.m_buffer[i].x));
        } 

        constexpr auto size() const -> uint32_t { return privateData.m_size; }
        
        constexpr auto push_back(T const& item) -> void 
        { 
            assert(privateData.m_size + 1 <= capacity); 
            std::construct_at(std::addressof(privateData.m_buffer[privateData.m_size++].x), item);
        }

        constexpr auto pop_back() -> void 
        {
            assert(privateData.m_size > 0);
            std::destroy_at(std::addressof(privateData.m_buffer[privateData.m_size--].x));
        }
 

        [[nodiscard]] constexpr auto operator[](uint32_t i) -> T&
        {
            assert(i < size());
            if constexpr (std::is_const_v<T>)
                return std::launder(privateData.m_buffer.data() + i)->x; // should I use std::launder here?
            else
                return privateData.m_buffer[i].x; // should I use std::launder here?
        }

        [[nodiscard]] constexpr auto operator[](uint32_t i) const -> T const&
        {
            assert(i < size());
            if constexpr (std::is_const_v<T>)
                return std::launder(privateData.m_buffer.data() + i)->x;
            else
                return privateData.m_buffer[i].x;
        }

        // public "privateData", to maintain standard layout
        struct {
            // union to delay initialization of the array
            union U
            {
                constexpr U() {}
                constexpr ~U() {}
                T x;
                uint8_t dummy; // g++ 12.2 complains when there are any non initialized unions in a constant expression
            };
            std::array<U, capacity> m_buffer; // will work with clang 16 std::array, but not gcc 12.2 std::array
            uint32_t m_size = 0;
        } privateData;
    };

}

#endif // MXC_STATICVECTOR_HPP
