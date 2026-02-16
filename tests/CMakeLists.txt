include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Helper function to add a test executable
function(add_raytracer_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} PRIVATE raytracer_core GTest::gtest_main)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# Tests
add_raytracer_test(test_vec3 test_vec3.cpp)
add_raytracer_test(test_ray test_ray.cpp)
add_raytracer_test(test_interval test_interval.cpp)
add_raytracer_test(test_image test_image.cpp)
add_raytracer_test(test_sphere test_sphere.cpp)
add_raytracer_test(test_plane test_plane.cpp)
add_raytracer_test(test_hittable_list test_hittable_list.cpp)
add_raytracer_test(test_materials test_materials.cpp)
add_raytracer_test(test_camera test_camera.cpp)
add_raytracer_test(test_scene test_scene.cpp)
add_raytracer_test(test_bvh test_bvh.cpp)

# CPU renderer test needs the cpu lib
add_executable(test_cpu_renderer test_cpu_renderer.cpp)
target_link_libraries(test_cpu_renderer PRIVATE raytracer_cpu GTest::gtest_main)
add_test(NAME test_cpu_renderer COMMAND test_cpu_renderer)
