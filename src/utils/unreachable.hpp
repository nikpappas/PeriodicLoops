#ifndef PLOP_SRC_UTILS_UNREACHABLE_HPP
#define PLOP_SRC_UTILS_UNREACHABLE_HPP

namespace plop::utils {
	/// \TODO: Come C++23, replace with ::std::unreachable

#if ( _WIN32 )
#define unreachable()                                                                                                  \
	{                                                                                                                   \
		__assume( false );                                                                                               \
	}
#else
#define unreachable()                                                                                                  \
	{                                                                                                                   \
		__builtin_unreachable();                                                                                         \
	}
#endif

}

#endif // PLOP_SRC_UTILS_UNREACHABLE_HPP
