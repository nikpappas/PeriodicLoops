#ifndef PLOP_SRC_LOGGING_LOGGING_HPP
#define PLOP_SRC_LOGGING_LOGGING_HPP

#include <iostream>

namespace plop {

	/// \brief Pass-through wrapper to a logger.
	///
	/// ATM, the only purpose of the wrapper is to ensure that the debug log statements
	/// are compile-time disabled in release builds, so the compiler can optimise out the
	/// whole call.
	template <typename T>
	inline void pl_debug( T &&prm_message ) {
		std::cout << prm_message << std::endl;
	}

	/// \brief Pass-through wrapper to a logger.
	///
	template <typename T>
	inline void pl_info( T &&prm_message ) {
		std::cout << prm_message << std::endl;
	}

	/// \brief Pass-through wrapper to a logger.
	///
	template <typename T>
	inline void pl_error( T &&prm_message ) {
		std::cout << prm_message << std::endl;
	}

}
#endif // PLOP_SRC_LOGGING_LOGGING_HPP
