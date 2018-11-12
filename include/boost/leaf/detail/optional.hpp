//Copyright (c) 2018 Emil Dotchevski
//Copyright (c) 2018 Second Spectrum, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef UUID_47258FCCB6B411E8A1F35AA00C39171A
#define UUID_47258FCCB6B411E8A1F35AA00C39171A

#include <utility>
#include <new>
#include <cassert>

namespace boost { namespace leaf {

	namespace leaf_detail
	{
		template <class T>
		class optional
		{
			union { T value_; };
			bool has_value_;

		public:

			typedef T value_type;

			constexpr optional() noexcept:
				has_value_(false)
			{
			}

			constexpr optional( optional const & x ):
				has_value_(false)
			{
				(void) (*this = x);
			}

			constexpr optional( optional && x ) noexcept:
				has_value_(false)
			{
				(void) (*this = std::move(x));
			}

			constexpr optional( T const & v ):
				has_value_(false)
			{
				put(v);
			}

			constexpr optional( T && v ) noexcept:
				has_value_(false)
			{
				put(std::move(v));
			}

			constexpr optional & operator=( optional const & x )
			{
				reset();
				if( x.has_value() )
					put(x.value());
				return *this;
			}

			constexpr optional & operator=( optional && x ) noexcept
			{
				reset();
				if( x.has_value() )
					put(std::move(x).value());
				return *this;
			}

			~optional() noexcept
			{
				reset();
			}

			constexpr void reset() noexcept
			{
				if( has_value() )
				{
					value_.~T();
					has_value_=false;
				}
			}

			constexpr T & put( T const & v )
			{
				reset();
				(void) new(&value_) T(v);
				has_value_=true;
				return value_;
			}

			constexpr T & put( T && v ) noexcept
			{
				reset();
				(void) new(&value_) T(std::move(v));
				has_value_=true;
				return value_;
			}

			constexpr bool has_value() const noexcept
			{
				return has_value_;
			}

			constexpr T const & value() const & noexcept
			{
				assert(has_value());
				return value_;
			}

			constexpr T & value() & noexcept
			{
				assert(has_value());
				return value_;
			}

			constexpr T const && value() const && noexcept
			{
				assert(has_value());
				return value_;
			}

			constexpr T value() && noexcept
			{
				assert(has_value());
				T tmp(std::move(value_));
				reset();
				return tmp;
			}
		};

	} //leaf_detail

} }

#endif
