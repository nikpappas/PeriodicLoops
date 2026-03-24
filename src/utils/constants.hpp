#ifndef PLOP_SRC_UTILS_CONSTANTS_HPP
#define PLOP_SRC_UTILS_CONSTANTS_HPP

namespace plop::utils {

	inline constexpr auto PLUGIN_NAME           = "Periodic Loops";
	inline constexpr auto NLOOPS_FOLDER         = "PeriodicLoops";
	inline constexpr auto LOG_EXTENSION         = ".log";
	inline constexpr auto SETTINGS_MACOS_FOLDER = "Application Support";

	/// The three major categories of OS platform
	enum class os_platform : ::std::uint8_t { PL_OS_MACOS, PL_OS_WINDOWS, PL_OS_LINUX };

	/// The OS platform of this build
	inline constexpr os_platform PL_OS_PLATFORM =
#ifdef __APPLE__
	  os_platform::PL_OS_MACOS
#elif _WIN32
	  os_platform::PL_OS_WINDOWS
#else
	  os_platform::PL_OS_LINUX
#endif
	  ;

} // namespace plop::utils

#endif // PLOP_SRC_UTILS_CONSTANTS_HPP
