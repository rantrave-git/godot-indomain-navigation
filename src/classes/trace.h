#pragma once
#include <exception>

namespace godot {
#define TRACE_PREFIX "\033[33m[T "
#define TRACE_SUFFIX "]:\033[37m "
#define DEBUG_PREFIX "\033[32m[D "
#define DEBUG_SUFFIX "]:\033[37m "
#define STATS_PREFIX "\033[36m["
#define STATS_SUFFIX "]:\033[37m "

#ifdef INDOMAIN_DEBUG_TRACE
	template<class _F>
	void code_reached(int code, _F s) {
		static std::set<int> _logs;
		if (_logs.count(code)) return;
		s();
		_logs.insert(code);
	}
#else
#define code_reached(...)
#endif

#ifdef INDOMAIN_DEBUG_TRACE
#ifdef INDOMAIN_TRACE_VMATH
#define TRACE_VMATH TRACE_PREFIX "VMATH" TRACE_SUFFIX
#else
#define TRACE_VMATH nullptr
#endif

#ifdef INDOMAIN_TRACE_BSP
#define INDOMAIN_DEBUG_BSP
#define TRACE_BSP TRACE_PREFIX "BSP" TRACE_SUFFIX
#else
#define TRACE_BSP nullptr
#endif

#ifdef INDOMAIN_TRACE_MOVE
#define TRACE_MOVE TRACE_PREFIX "MOVE" TRACE_SUFFIX
#else
#define TRACE_MOVE nullptr
#endif

#ifdef INDOMAIN_TRACE_BSP_SPLIT
#define TRACE_BSP_SPLIT TRACE_PREFIX "BSP_SPLIT" TRACE_SUFFIX
#else
#define TRACE_BSP_SPLIT nullptr
#endif

#ifdef INDOMAIN_TRACE_GRID
#define INDOMAIN_DEBUG_GRID
#define TRACE_GRID TRACE_PREFIX "GRID" TRACE_SUFFIX
#else
#define TRACE_GRID nullptr
#endif

#ifdef INDOMAIN_TRACE_NAVDOMAIN
#define INDOMAIN_DEBUG_NAVMESH
#define TRACE_NAVDOMAIN TRACE_PREFIX "NAVDOMAIN" TRACE_SUFFIX
#else
#define TRACE_NAVDOMAIN nullptr
#endif

#ifdef INDOMAIN_TRACE_DOMAINAGENT
#define INDOMAIN_DEBUG_DOMAINAGENT
#define TRACE_DOMAINAGENT TRACE_PREFIX "DOMAIN_AGENT" TRACE_SUFFIX
#else
#define TRACE_DOMAINAGENT nullptr
#endif

#ifdef INDOMAIN_TRACE_COLLISIONS
#define TRACE_COLLISIONS TRACE_PREFIX "DOMAIN COLLISIONS" TRACE_SUFFIX
#else
#define TRACE_COLLISIONS nullptr
#endif

#ifdef INDOMAIN_TRACE_DOMAINREGION
#define INDOMAIN_DEBUG_DOMAINREGION
#define TRACE_DOMAINREGION TRACE_PREFIX "DOMAIN_REGION" TRACE_SUFFIX
#else
#define TRACE_DOMAINREGION nullptr
#endif

#ifdef INDOMAIN_TRACE_STATS
#define TRACE_STATS STATS_PREFIX "STATS" STATS_SUFFIX
#else
#define TRACE_STATS nullptr
#endif

	template<typename... Args>
	constexpr void TRACE(const char* t, const Args&... args) {
		if (!t) return;
		UtilityFunctions::print(t, args...);
	}
#else
#define TRACE_VMATH nullptr
#define TRACE_BSP nullptr
#define TRACE_MOVE nullptr
#define TRACE_BSP_SPLIT nullptr
#define TRACE_GRID nullptr
#define TRACE_NAVDOMAIN nullptr
#define TRACE_DOMAINAGENT nullptr
#define TRACE_DOMAINREGION nullptr
#define TRACE_COLLISIONS nullptr
#define TRACE_STATS nullptr
#define TRACE(...)
#endif

#ifdef INDOMAIN_DEBUG_BSP
#define DEBUG_BSP DEBUG_PREFIX "GRID" DEBUG_SUFFIX
#else
#define DEBUG_BSP nullptr
#endif

#ifdef INDOMAIN_DEBUG_GRID
#define DEBUG_GRID DEBUG_PREFIX "GRID" DEBUG_SUFFIX
#else
#define DEBUG_GRID nullptr
#endif

#ifdef INDOMAIN_DEBUG_NAVMESH
#define DEBUG_NAVMESH DEBUG_PREFIX "GRID" DEBUG_SUFFIX
#else
#define DEBUG_NAVMESH nullptr
#endif

#ifdef INDOMAIN_DEBUG_DOMAINAGENT
#define DEBUG_DOMAINAGENT DEBUG_PREFIX "DOMAIN_AGENT" DEBUG_SUFFIX
#else
#define DEBUG_DOMAINAGENT nullptr
#endif

#ifdef INDOMAIN_DEBUG_DOMAINREGION
#define DEBUG_DOMAINREGION DEBUG_PREFIX "DOMAIN_REGION" DEBUG_SUFFIX
#else
#define DEBUG_DOMAINREGION nullptr
#endif

    template<typename... Args>
    constexpr void DEBUG(const char* t, const Args&... args) {
        if (!t) return;
        UtilityFunctions::print(t, args...);
    }
}