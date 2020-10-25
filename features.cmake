set(JLT_WITH_MEM_CHECKS 1) # Enable for testing only
set(JLT_WITH_DEBUG_LOGGING 1) # Force log level to be debug
set(JLT_WITH_MULTI_WINDOWS 0) # Include support for multiple windows

# Paths
set(JLT_BUILD_DIR ${CMAKE_BINARY_DIR}) # Build path
set(JLT_ASSETS_DIR ${CMAKE_CURRENT_LIST_DIR}/assets) # Assets path

# VFS paths
set(JLT_BUILD_VDIR "/build") # Virtual path for JLT_BUILD_DIR
