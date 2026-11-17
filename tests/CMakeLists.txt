include(cmake/Tests)
aw_test_target(NAME tobus
    SOURCES
        test_event_types.cpp
        test_in_process_bus.cpp
        test_subscription.cpp
        test_factory.cpp
        test_stubs.cpp
    LINK_TARGET aw::tobus)
