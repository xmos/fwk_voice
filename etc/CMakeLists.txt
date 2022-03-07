
## Set up the build options
include(build_options.cmake)
if(${AVONA_STANDALONE_BUILD})
    include(fetch_deps.cmake)
    add_subdirectory(deps)
endif()
