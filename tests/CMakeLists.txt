cmake_minimum_required(VERSION 3.16)
project(CacheTests LANGUAGES CXX)

enable_testing()
set(ARC_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../headers" CACHE PATH
    "Directory containing ARC_cache.hpp")

add_executable(arc_tests arc_tests.cpp)
target_include_directories(arc_tests PRIVATE "${ARC_INCLUDE_DIR}")
target_compile_features(arc_tests PRIVATE cxx_std_20)
set_target_properties(arc_tests PROPERTIES CXX_EXTENSIONS OFF)

if(MSVC)
    target_compile_options(arc_tests PRIVATE /W4 /utf-8)
else()
    target_compile_options(arc_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(ARC_CHECKED_ITERATORS "Enable libstdc++ iterator and optional checks" ON)
if(ARC_CHECKED_ITERATORS)
    target_compile_definitions(arc_tests PRIVATE _GLIBCXX_DEBUG _GLIBCXX_ASSERTIONS)
endif()

set(test_cases
    empty_cache
    miss_clears_pointer
    insert_and_get
    repeated_hit
    duplicate_preserves_value
    empty_key_and_value
    embedded_null_key
    integer_data
    non_default_data
    independent_caches
    pointer_survives_promotion
    sequential_eviction
    promotion_protects_page
    all_pages_frequent
    ghost_lookup_is_miss
    reload_b1
    reload_b2
    ghost_releases_data
    destruction_releases_data
    repeated_reloads
    mixed_workload
    hot_and_cold_workload
)

foreach(test_case IN LISTS test_cases)
    add_test(NAME arc.${test_case} COMMAND arc_tests ${test_case})
    set_tests_properties(arc.${test_case} PROPERTIES TIMEOUT 10)
endforeach()

set(LFU_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../headers" CACHE PATH
    "Directory containing LFU_cache.hpp")

add_executable(lfu_tests lfu_tests.cpp)
target_include_directories(lfu_tests PRIVATE "${LFU_INCLUDE_DIR}")
target_compile_features(lfu_tests PRIVATE cxx_std_20)
set_target_properties(lfu_tests PROPERTIES CXX_EXTENSIONS OFF)

if(MSVC)
    target_compile_options(lfu_tests PRIVATE /W4 /utf-8)
else()
    target_compile_options(lfu_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(LFU_CHECKED_ITERATORS "Enable libstdc++ iterator checks for LFU" ON)
if(LFU_CHECKED_ITERATORS)
    target_compile_definitions(lfu_tests PRIVATE _GLIBCXX_DEBUG _GLIBCXX_ASSERTIONS)
endif()

set(lfu_test_cases
    empty_cache
    miss_clears_pointer
    insert_and_get
    repeated_hit
    duplicate_updates_value
    duplicate_promotes_page
    duplicate_at_capacity
    empty_key_and_value
    embedded_null_key
    integer_data
    non_default_data
    independent_caches
    pointer_survives_promotion
    pointer_survives_update
    sequential_eviction
    promotion_protects_page
    frequency_beats_recency
    equal_frequency_uses_recency
    all_pages_frequent_reject_new_page
    misses_do_not_change_eviction
    reload_resets_frequency
    eviction_releases_data
    update_releases_old_data
    rejected_insert_releases_data
    destruction_releases_data
    repeated_reloads
    mixed_workload
    hot_and_cold_workload
)

foreach(test_case IN LISTS lfu_test_cases)
    add_test(NAME lfu.${test_case} COMMAND lfu_tests ${test_case})
    set_tests_properties(lfu.${test_case} PROPERTIES TIMEOUT 10)
endforeach()
