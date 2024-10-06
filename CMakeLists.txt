cmake_minimum_required(VERSION 3.30)
project(ShimBoot_Generator)

set(CMAKE_CXX_STANDARD 26)
set(SBGenVersion 1.0.0)

include(FetchContent)

option(SBGEN_DISABLE_BACKUP_URL "Disable the backup url that is used if the main url is down." OFF)

FetchContent_Declare(
        cpr
        GIT_REPOSITORY https://github.com/libcpr/cpr
        GIT_TAG master
)
FetchContent_MakeAvailable(cpr)

FetchContent_Declare(
        ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
        GIT_TAG v5.0.0
)
FetchContent_MakeAvailable(ftxui)

if (NOT SBGEN_DISABLE_BACKUP_URL)
    # We need json for the GitHub API
    FetchContent_Declare(
            json
            GIT_REPOSITORY https://github.com/nlohmann/json
            GIT_TAG v3.11.3
    )
    FetchContent_MakeAvailable(json)
endif ()

add_executable(ShimBoot_Generator src/main.cpp
        src/ui/ui.cpp
        src/ui/ui.h
        src/init.cpp
        src/init.h
        src/inet/inet.cpp
        src/inet/inet.h
)

target_compile_definitions(ShimBoot_Generator PRIVATE VERSION="${SBGenVersion}")

target_link_libraries(ShimBoot_Generator
        PRIVATE cpr::cpr
        PRIVATE ftxui::screen
        PRIVATE ftxui::dom
        PRIVATE ftxui::component
)

if (SBGEN_DISABLE_BACKUP_URL)
    target_compile_definitions(ShimBoot_Generator PRIVATE NO_BACKUP)
else ()
    target_link_libraries(ShimBoot_Generator PRIVATE nlohmann_json::nlohmann_json)
endif ()