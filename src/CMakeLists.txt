# Core library (shared math, scene, materials, image)
set(CORE_SOURCES
    utils/image.cpp
    utils/denoise.cpp
    scene/primitives.cpp
    scene/material.cpp
    scene/camera.cpp
    scene/scene.cpp
    scene/bvh_builder.cpp
)

add_library(raytracer_core STATIC ${CORE_SOURCES})
target_include_directories(raytracer_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# CPU renderer
set(CPU_SOURCES
    cpu/cpu_renderer.cpp
)

add_library(raytracer_cpu STATIC ${CPU_SOURCES})
target_link_libraries(raytracer_cpu PUBLIC raytracer_core)

# GPU renderer (CUDA)
if(ENABLE_CUDA)
    set(GPU_SOURCES
        gpu/gpu_renderer.cu
    )

    add_library(raytracer_gpu STATIC ${GPU_SOURCES})
    target_include_directories(raytracer_gpu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(raytracer_gpu PUBLIC raytracer_core)
    set_target_properties(raytracer_gpu PROPERTIES CUDA_ARCHITECTURES "${CMAKE_CUDA_ARCHITECTURES}")
    # Fast math : --ftz=true --prec-div=false --prec-sqrt=false --fmad=true --use_fast_math
    target_compile_options(raytracer_gpu PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--use_fast_math>)
    if(WIN32)
        target_link_libraries(raytracer_gpu PUBLIC gdi32 user32)
    endif()
endif()

# Hybrid renderer (CUDA)
if(ENABLE_CUDA)
    set(HYBRID_SOURCES
        hybrid/hybrid_renderer.cu
    )
    add_library(raytracer_hybrid STATIC ${HYBRID_SOURCES})
    target_include_directories(raytracer_hybrid PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(raytracer_hybrid PUBLIC raytracer_core raytracer_cpu raytracer_gpu)
    set_target_properties(raytracer_hybrid PROPERTIES CUDA_ARCHITECTURES "${CMAKE_CUDA_ARCHITECTURES}")
    target_compile_options(raytracer_hybrid PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--use_fast_math>)
endif()

# Main executable
if(ENABLE_CUDA)
    add_executable(raytracer main.cpp gpu/gpu_realtime.cpp)
    target_link_libraries(raytracer PRIVATE raytracer_cpu raytracer_gpu raytracer_hybrid)
    target_compile_definitions(raytracer PRIVATE RT_CUDA_ENABLED)
    if(ENABLE_REALTIME AND TARGET SDL3::SDL3-static)
        target_link_libraries(raytracer PRIVATE SDL3::SDL3-static)
        target_compile_definitions(raytracer PRIVATE RT_REALTIME_ENABLED)
    endif()
else()
    add_executable(raytracer main.cpp)
    target_link_libraries(raytracer PRIVATE raytracer_cpu)
endif()
