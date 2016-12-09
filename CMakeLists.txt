
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -g")

add_library(efence SHARED efence.c page.c print.c)

add_executable(tstheap tstheap.c)

link_directories(. )
add_executable(eftest eftest.c)
target_link_libraries(eftest pthread  efence)

add_executable(test-efence test-efence.c )
target_link_libraries(test-efence pthread efence)

