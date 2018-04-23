#ifndef INCLUDED_PVPGN_HASH_TUPLE_H
#define INCLUDED_PVPGN_HASH_TUPLE_H

#include <cstddef>
#include <functional>
#include <tuple>

// https://stackoverflow.com/questions/7110301/generic-hash-for-tuples-in-unordered-map-unordered-set
namespace pvpgn
{
	namespace hash_tuple
	{
		template <typename TT>
		struct hash
		{
			std::size_t operator()(TT const& tt) const
			{
				return std::hash<TT>()(tt);
			}
		};

		namespace
		{
			template <class T>
			inline void hash_combine(std::size_t& seed, T const& v)
			{
				seed ^= hash_tuple::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			}
		}

		namespace
		{
			// Recursive template code derived from Matthieu M.
			template <class Tuple, std::size_t Index = std::tuple_size<Tuple>::value - 1>
			struct HashValueImpl
			{
				static void apply(std::size_t& seed, Tuple const& tuple)
				{
					HashValueImpl<Tuple, Index - 1>::apply(seed, tuple);
					hash_combine(seed, std::get<Index>(tuple));
				}
			};

			template <class Tuple>
			struct HashValueImpl<Tuple, 0>
			{
				static void apply(std::size_t& seed, Tuple const& tuple)
				{
					hash_combine(seed, std::get<0>(tuple));
				}
			};
		}

		template <typename ... TT>
		struct hash<std::tuple<TT...>>
		{
			std::size_t operator()(std::tuple<TT...> const& tt) const
			{
				std::size_t seed = 0;
				HashValueImpl<std::tuple<TT...> >::apply(seed, tt);
				return seed;
			}
		};
	}
}

#endif