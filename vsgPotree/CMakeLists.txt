set(INCS
    vsgpotree.h
)
set(SOURCES
    vsgpotree.cpp
    vsgloadpotree.cpp
)

add_executable(vsgPotree ${INCS} ${SOURCES})

target_link_libraries(vsgPotree vsg::vsg)
