//Copyright (c) 2018 Emil Dotchevski
//Copyright (c) 2018 Second Spectrum, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef UUID_C86E4C4ED0F011E8BB777EB8A659E189
#define UUID_C86E4C4ED0F011E8BB777EB8A659E189

#include <boost/leaf/error.hpp>
#include <boost/leaf/detail/print.hpp>
#include <tuple>

namespace boost { namespace leaf {

	class error_capture;

	template <class... M>
	bool handle_error( error_capture const &, M && ... ) noexcept;

	template <class P>
	decltype(P::value) const * peek( error_capture const & ) noexcept;

	void diagnostic_output( std::ostream &, error_capture const & );

	namespace leaf_detail
	{
		template <class... List>
		struct all_available;

		template <class Car, class... Cdr>
		struct all_available<Car, Cdr...>
		{
			static bool check( error_capture const & cap ) noexcept
			{
				return peek<Car>(cap) && all_available<Cdr...>::check(cap);
			}
		};

		template <>
		struct all_available<>
		{
			constexpr static bool check( error_capture const & ) noexcept { return true; }
		};

		////////////////////////////////////////

		template <int I, class Tuple>
		struct tuple_for_each_capture
		{
			static void const * dynamic_bind( Tuple const & tup, char const * (*type_id)() ) noexcept
			{
				assert(type_id!=0);
				typedef typename std::tuple_element<I-1,Tuple>::type ith_type;
				if( &type<typename ith_type::value_type> == type_id )
					return &std::get<I-1>(tup);
				return tuple_for_each_capture<I-1,Tuple>::dynamic_bind(tup,type_id);
			}

			static void print( std::ostream & os, Tuple const & tup )
			{
				typedef typename std::tuple_element<I-1,Tuple>::type ith_type;
				tuple_for_each_capture<I-1,Tuple>::print(os,tup);
				auto & opt = std::get<I-1>(tup);
				if( opt.has_value() && diagnostic<typename ith_type::value_type>::print(os,opt.value()) )
					os << std::endl;
			}

			static void unload( error const & e, Tuple && tup ) noexcept
			{
				tuple_for_each_capture<I-1,Tuple>::unload(e,std::move(tup));
				auto && opt = std::get<I-1>(std::move(tup));
				if( opt.has_value() )
					(void) e.propagate(std::move(opt).value());
			}
		};

		template <class Tuple>
		struct tuple_for_each_capture<0, Tuple>
		{
			static void const * dynamic_bind( Tuple const &, char const * (*)() ) noexcept { return 0; }
			static void print( std::ostream &, Tuple const & ) noexcept { }
			static void unload( error const &, Tuple && ) noexcept { }
		};

	} //leaf_detail

	////////////////////////////////////////

	class error_capture
	{
		template <class... M>
		friend bool leaf::handle_error( error_capture const &, M && ... ) noexcept;

		template <class P>
		friend decltype(P::value) const * leaf::peek( error_capture const & ) noexcept;

		friend void leaf::diagnostic_output( std::ostream &, error_capture const & );

		////////////////////////////////////////

		class dynamic_store
		{
			mutable std::atomic<int> refcount_;

			virtual void const * bind_( char const * (*)() ) const noexcept = 0;

		protected:

			dynamic_store() noexcept:
				refcount_(0)
			{
			}

		public:

			virtual ~dynamic_store() noexcept
			{
			}

			void addref() const noexcept
			{
				++refcount_;
			}

			void release() const noexcept
			{
				if( !--refcount_ )
					delete this;
			}

			template <class T>
			leaf_detail::optional<T> const * bind() const noexcept
			{
				if( void const * p = bind_(&type<T>) )
					return static_cast<leaf_detail::optional<T> const *>(p);
				else
					return 0;
			}

			virtual void diagnostic_output( std::ostream & ) const = 0;
			virtual void unload( error const & ) noexcept = 0;
		};

		////////////////////////////////////////

		template <class... T>
		class dynamic_store_impl:
			public dynamic_store
		{
			std::tuple<leaf_detail::optional<T>...> s_;
			public:

			explicit dynamic_store_impl( std::tuple<leaf_detail::optional<T>...> && s ) noexcept:
				s_(std::move(s))
			{
			}

			void const * bind_( char const * (*type_id)() ) const noexcept
			{
				using namespace leaf_detail;
				assert(type_id!=0);
				return tuple_for_each_capture<sizeof...(T),std::tuple<optional<T>...>>::dynamic_bind(s_,type_id);
			}

			void diagnostic_output( std::ostream & os ) const
			{
				leaf_detail::tuple_for_each_capture<sizeof...(T),decltype(s_)>::print(os,s_);
			}

			void unload( error const & e ) noexcept
			{
				leaf_detail::tuple_for_each_capture<sizeof...(T),decltype(s_)>::unload(e,std::move(s_));
			}
		};

		////////////////////////////////////////

		void free() noexcept
		{
			if( ds_ )
			{
				ds_->release();
				ds_=0;
			}
		}

		template <class F, class... MatchTypes>
		int unwrap( leaf_detail::match_fn<F,MatchTypes...> const & m, bool & matched ) const
		{
			if( !matched && (matched=leaf_detail::all_available<MatchTypes...>::check(*this)) )
				(void) m.f( *peek<MatchTypes>(*this)... );
			return 42;
		}

		template <class... MatchTypes>
		int unwrap( leaf_detail::match_no_fn<MatchTypes...> const & m, bool & matched ) const noexcept
		{
			if( !matched  )
				matched = leaf_detail::all_available<MatchTypes...>::check(*this);
			return 42;
		}

		dynamic_store * ds_;
		error e_;

	protected:

		void set_error( error const & e )
		{
			e_ = e;
		}

	public:			

		error_capture() noexcept:
			ds_(0)
		{
		}

		template <class... E>
		error_capture( error const & e, std::tuple<leaf_detail::optional<E>...> && s ) noexcept:
			ds_(new dynamic_store_impl<E...>(std::move(s))),
			e_(e)
		{
			ds_->addref();
		}

		~error_capture() noexcept
		{
			free();
		}

		error_capture( error_capture const & x ) noexcept:
			ds_(x.ds_),
			e_(x.e_)
		{
			if( ds_ )
				ds_->addref();
		}

		error_capture( error_capture && x ) noexcept:
			ds_(x.ds_),
			e_(std::move(x.e_))
		{
			x.ds_ = 0;
		}

		error_capture & operator=( error_capture const & x ) noexcept
		{
			ds_ = x.ds_;
			ds_->addref();
			e_ = x.e_;
			return *this;
		}

		error_capture & operator=( error_capture && x ) noexcept
		{
			ds_ = x.ds_;
			x.ds_ = 0;
			e_ = x.e_;
			return *this;
		}

		explicit operator bool() const noexcept
		{
			return ds_!=0;
		}

		error unload() noexcept
		{
			if( ds_ )
			{
				ds_->unload(e_);
				free();
			}
			return e_;
		}
	};

	////////////////////////////////////////

	template <class... M>
	bool handle_error( error_capture const & e, M && ... m ) noexcept
	{
		if( e )
		{
			bool matched = false;
			{ using _ = int[ ]; (void) _ { 42, e.unwrap(m,matched)... }; }
			if( matched )
				return true;
		}
		return false;
	}

	template <class P>
	decltype(P::value) const * peek( error_capture const & e ) noexcept
	{
		if( e )
			if( auto * opt = e.ds_->bind<P>() )
				if( opt->has_value() )
					return &opt->value().value;
		return 0;
	}

	inline void diagnostic_output( std::ostream & os, error_capture const & e )
	{
		if( e )
			e.ds_->diagnostic_output(os);
	}

} }

#endif
