add_library(turbopalmtree_core STATIC
    engine/green_engine.cpp
    engine/worker_thread.cpp
    vision/robust_meter_detector.cpp
    benchmarks/latency_benchmark.cpp
)

target_include_directories(turbopalmtree_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

if(WIN32)
    target_link_libraries(turbopalmtree_core PRIVATE ws2_32)
endif()

add_executable(latency_benchmark
    benchmarks/latency_benchmark_main.cpp
)

target_link_libraries(latency_benchmark PRIVATE turbopalmtree_core)
