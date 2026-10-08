add_library(Briefcase.PlayerCap.UE4SS SHARED PlayerCapUe4ss.cpp)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" MOD_VERSION)
string(STRIP "${MOD_VERSION}" MOD_VERSION)
configure_file(ModVersion.hpp.in generated/ModVersion.hpp @ONLY)
target_include_directories(Briefcase.PlayerCap.UE4SS PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(Briefcase.PlayerCap.UE4SS PRIVATE
  Briefcase::DeceiveInc)
set_target_properties(Briefcase.PlayerCap.UE4SS PROPERTIES OUTPUT_NAME main)

# UE4SS starts C++ mods asynchronously. The dedicated server reads MaxPlayers
# before that lifecycle is guaranteed to run, so this small companion DLL is
# called by the proxy synchronously before the EXE entry point.
add_library(Briefcase.PlayerCap.PreEntry SHARED
  PlayerCapPreEntry.cpp
  PlayerCap.cpp
  PlayerLimitSites.cpp)
target_include_directories(Briefcase.PlayerCap.PreEntry PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Briefcase.PlayerCap.PreEntry PRIVATE Zydis shell32)
target_compile_features(Briefcase.PlayerCap.PreEntry PRIVATE cxx_std_20)
set_target_properties(Briefcase.PlayerCap.PreEntry PROPERTIES OUTPUT_NAME BriefcasePreEntry)

if(BUILD_TESTING)
  add_executable(Briefcase.PlayerCap.Sites tests/PlayerCapSites.cpp PlayerLimitSites.cpp)
  target_link_libraries(Briefcase.PlayerCap.Sites PRIVATE Zydis)
  add_test(NAME Briefcase.PlayerCap.Sites COMMAND Briefcase.PlayerCap.Sites)
endif()
