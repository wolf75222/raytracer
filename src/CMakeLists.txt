# Core library (shared math, geometry, materials, image)
set(CORE_SOURCES
    core/image.cpp
    core/sphere.cpp
    core/plane.cpp
    core/hittable_list.cpp
    core/lambertian.cpp
    core/metal.cpp
    core/dielectric.cpp
    core/camera.cpp
    core/scene.cpp
    core/bvh.cpp
)

add_library(raytracer_core STATIC ${CORE_SOURCES})
target_include_directories(raytracer_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# CPU renderer
set(CPU_SOURCES
    cpu/cpu_renderer.cpp
)

add_library(raytracer_cpu STATIC ${CPU_SOURCES})
target_link_libraries(raytracer_cpu PUBLIC raytracer_core)

# Main executable
add_executable(raytracer main.cpp)
target_link_libraries(raytracer PRIVATE raytracer_cpu)
