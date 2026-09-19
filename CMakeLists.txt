cmake_minimum_required(VERSION 3.16)
project(cache_research LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRCS
    source/main.cpp)

add_library(cache_research STATIC ${SRCS})
target_include_directories(cache_research PUBLIC "headers")

add_executable(cache_research.out source/main.cpp)
target_link_libraries(cache_research.out cache_research)

enable_testing()

foreach(cache IN ITEMS arc lfu 2q)
    string(TOUPPER "${cache}" cache_upper)
    set(${cache_upper}_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/headers" CACHE PATH
        "Directory containing ${cache_upper}_cache.hpp")

    add_executable(${cache}_tests tests/${cache}_tests.cpp)
    target_include_directories(${cache}_tests PRIVATE "${${cache_upper}_INCLUDE_DIR}")

    if(MSVC)
        target_compile_options(${cache}_tests PRIVATE /W4 /utf-8)
    else()
        target_compile_options(${cache}_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    option(${cache_upper}_CHECKED_ITERATORS
        "Enable libstdc++ iterator and optional checks for ${cache_upper}" ON)
    if(${cache_upper}_CHECKED_ITERATORS)
        target_compile_definitions(${cache}_tests PRIVATE _GLIBCXX_DEBUG _GLIBCXX_ASSERTIONS)
    endif()
endforeach()

set(arc_test_cases
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

set(2q_test_cases
    empty_cache
    miss_clears_pointer
    repeated_hit
    duplicate_updates_value
    empty_key_and_value
    embedded_null_key
    integer_data
    non_default_data
    independent_caches
    pointer_survives_update
    update_releases_old_data
    destruction_releases_data
    insert_and_get
    duplicate_at_capacity
    sequential_eviction
    a1in_hit_preserves_fifo
    a1in_update_preserves_fifo
    ghost_lookup_is_miss
    ghost_reload_promotes_page
    forgotten_ghost_returns_to_a1in
    ghost_hit_preserves_fifo
    newest_ghost_survives_history_limit
    kin_boundary_evicts_am
    am_hit_refreshes_recency
    am_update_refreshes_recency
    ghost_reload_at_kin_evicts_am
    pointer_survives_am_hit
    ghost_releases_data
    repeated_reloads
    mixed_workload
    hot_and_cold_workload
)

foreach(cache IN ITEMS arc lfu 2q)
    foreach(test_case IN LISTS ${cache}_test_cases)
        add_test(NAME ${cache}.${test_case} COMMAND ${cache}_tests ${test_case})
        set_tests_properties(${cache}.${test_case} PROPERTIES TIMEOUT 10)
    endforeach()
endforeach()
