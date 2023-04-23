####################### Compiler Options ######################################
if(WIN32)
    if(DEFINED NDEBUG)
        target_compile_options(Project PRIVATE /std:c++20 /O2 /W4 /WX)
    else()
        target_compile_options(Project PRIVATE /std:c++20 /O0 /W4 /WX /DEBUG)
    endif()
else()
    set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
    if(DEFINED NDEBUG)
        target_compile_options(Project PRIVATE -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror)
    else()
        target_compile_options(Project PRIVATE -std=c++20 -O0 -Wall -Wextra -Wpedantic -Werror -g)
    endif()
endif()


