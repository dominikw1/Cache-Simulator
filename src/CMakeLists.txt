add_library(GRA_Cache_lib Simulation.cpp CPU.cpp InstructionCache.cpp)
#add_executable(GRA_Cache main.c)
#target_link_libraries(GRA_Cache GRA_Cache_lib)

set(SYSTEM_C_DIR ../systemc)
include_directories(${SYSTEM_C_DIR}/include/)
link_directories(${SYSTEM_C_DIR}/lib/)