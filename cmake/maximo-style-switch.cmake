# Add the bridge to a private copy of the generated PAL player-update unit.
set(maximo_unit "${PS2_GENERATED}/unit_0019.cpp")
if(NOT EXISTS "${maximo_unit}")
    message(FATAL_ERROR "The Style Switch needs the locally generated Maximo PAL EE units")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${maximo_unit}")
file(READ "${maximo_unit}" maximo_source)
string(REPLACE "\r\n" "\n" maximo_source "${maximo_source}")
# Verify the two lines between these labels by hash. Their contents stay in the local input.
string(REGEX MATCH "L_00165F30:\n[^\n]*\n[^\n]*\nL_00165F34:\n" maximo_site "${maximo_source}")
string(SHA256 maximo_guard "${maximo_site}")
if(NOT maximo_guard STREQUAL "6f670021581cc06dcda295aea14fe2cba68879250cbeed81e410f52ec153eb59"
        OR NOT maximo_source MATCHES "case [0-9]+u: goto L_00165F30;")
    message(FATAL_ERROR "Unexpected PAL player-update unit or missing 0x00165F30 return entry")
endif()
string(REPLACE "L_00165F30:\n" "L_00165F30:\n    if (maximo::weapon_equipment_point(c, depth)) return;\n"
    maximo_hook "${maximo_site}")
string(REPLACE "${maximo_site}" "${maximo_hook}" maximo_source "${maximo_source}")
set(maximo_generated "${CMAKE_CURRENT_BINARY_DIR}/maximo-style-switch/unit_0019.cpp")
file(CONFIGURE OUTPUT "${maximo_generated}" CONTENT "#include \"maximo_weapons.hpp\"\n${maximo_source}" @ONLY)
list(REMOVE_ITEM PS2_UNITS "${maximo_unit}")
list(APPEND PS2_UNITS "${maximo_generated}")
