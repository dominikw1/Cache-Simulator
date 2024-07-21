add_library(GRA_Cache_lib SubRequest.cpp Simulation.cpp Cache.cpp CPU.cpp RAM.cpp)

set(SYSTEM_C_DIR ../systemc)
include_directories(${SYSTEM_C_DIR}/include/)
link_directories(${SYSTEM_C_DIR}/lib/)