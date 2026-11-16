# RetroPad Link - JoypadOS build hook
#
# Injects RetroPad Link into a normal JoypadOS build without modifying
# any JoypadOS-owned files.


# ============================================================================
# ONLY ATTACH TO JOYPADOS
# ============================================================================

if(NOT PROJECT_NAME STREQUAL "joypad")
    return()
endif()


# ============================================================================
# ONLY REGISTER ONCE PER CMAKE CONFIGURE PASS
# ============================================================================
#
# IMPORTANT:
#
# Do NOT use CACHE variables for this.
#
# CMake targets are recreated on every configure pass, while CACHE variables
# survive between configure passes. A cached "already registered" flag would
# therefore prevent RetroPad Link from being recreated during an automatic
# CMake reconfigure.
#
# A GLOBAL property exists only for the current configure process, which is
# exactly what we want.

get_property(
    RETROPAD_LINK_ALREADY_REGISTERED
    GLOBAL
    PROPERTY RETROPAD_LINK_HOOK_REGISTERED
)


if(RETROPAD_LINK_ALREADY_REGISTERED)
    return()
endif()


set_property(
    GLOBAL
    PROPERTY RETROPAD_LINK_HOOK_REGISTERED TRUE
)


message(
    STATUS
    "RetroPad Link: attaching to JoypadOS"
)


# ============================================================================
# RETROPAD LINK LOCATION
# ============================================================================

set(
    RETROPAD_LINK_DIR
    "${CMAKE_CURRENT_LIST_DIR}"
)


# ============================================================================
# DEFER TARGET CREATION
# ============================================================================
#
# At the time CMAKE_PROJECT_INCLUDE runs, JoypadOS has not yet defined all of
# its source lists and helper configuration.
#
# Defer our target definition until the end of the JoypadOS source directory.

cmake_language(
    DEFER
    DIRECTORY "${CMAKE_SOURCE_DIR}"
    CALL include
    "${RETROPAD_LINK_DIR}/retropad_target.cmake"
)
