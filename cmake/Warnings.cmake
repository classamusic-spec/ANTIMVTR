# Project-wide warning configuration applied through an interface target.
add_library(antimatr_warnings INTERFACE)

if (MSVC)
    target_compile_options(antimatr_warnings INTERFACE /W4 /permissive- /Zc:__cplusplus)
    if (ANTIMATR_WARNINGS_AS_ERRORS)
        target_compile_options(antimatr_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(antimatr_warnings INTERFACE
        -Wall -Wextra -Wpedantic
        -Wshadow -Wcast-align -Wunused -Woverloaded-virtual
        -Wnull-dereference -Wdouble-promotion
        -Wno-unused-parameter)
    if (ANTIMATR_WARNINGS_AS_ERRORS)
        target_compile_options(antimatr_warnings INTERFACE -Werror)
    endif()
endif()
