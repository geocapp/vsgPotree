set(INCS
    Attributes.h
    json.hpp
    vsgpotree.h
    vsgPipelineCache.h
)
set(SOURCES
    vsgPipelineCache.cpp
    vsgpotree.cpp
    vsgloadpotree.cpp
)

add_executable(vsgPotree ${INCS} ${SOURCES})

target_link_libraries(vsgPotree vsg::vsg)
