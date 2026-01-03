# Unit tests

add_executable(depbridge_unit_ids
    unit/test_ids.cpp
)
target_link_libraries(depbridge_unit_ids PRIVATE depbridge_core)
add_test(NAME depbridge.unit.ids COMMAND depbridge_unit_ids)

add_executable(depbridge_unit_normalize
    unit/test_normalize.cpp
)
target_link_libraries(depbridge_unit_normalize PRIVATE depbridge_core)
add_test(NAME depbridge.unit.normalize COMMAND depbridge_unit_normalize)

add_executable(depbridge_unit_cyclonedx
    unit/test_cyclonedx.cpp
)
target_link_libraries(depbridge_unit_cyclonedx PRIVATE depbridge_core)
add_test(NAME depbridge.unit.cyclonedx COMMAND depbridge_unit_cyclonedx)

add_executable(depbridge_unit_filter
    unit/test_filter.cpp
)
target_link_libraries(depbridge_unit_filter PRIVATE depbridge_core)
add_test(NAME depbridge.unit.filter COMMAND depbridge_unit_filter)

add_executable(depbridge_unit_variant_normalize
    unit/test_variant_normalize.cpp
)
target_link_libraries(depbridge_unit_variant_normalize PRIVATE depbridge_core)
add_test(NAME depbridge.unit.variant_normalize COMMAND depbridge_unit_variant_normalize)

add_executable(depbridge_unit_enrichment_noop
    unit/test_enrichment_noop.cpp
)
target_link_libraries(depbridge_unit_enrichment_noop PRIVATE depbridge_core)
add_test(NAME depbridge.unit.enrichment_noop COMMAND depbridge_unit_enrichment_noop)

# Integration tests

add_executable(depbridge_integration_simulated
    integration/test_simulated_end_to_end.cpp
)

target_link_libraries(depbridge_integration_simulated PRIVATE depbridge_core)

add_test(
    NAME depbridge.integration.simulated
    COMMAND depbridge_integration_simulated
)

add_executable(depbridge_unit_classify unit/test_classify.cpp)
target_link_libraries(depbridge_unit_classify PRIVATE depbridge::core)
add_test(NAME depbridge.unit.classify COMMAND depbridge_unit_classify)
