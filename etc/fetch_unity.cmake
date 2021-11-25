
## fetch dependencies
include(FetchContent)

FetchContent_Declare(
  unity
  GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
  GIT_TAG        v2.5.2
  GIT_SHALLOW    TRUE
  SOURCE_DIR     unity
)
FetchContent_Populate(unity)