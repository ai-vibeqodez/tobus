# One executable per example; add new ones to the list.
set(TOBUS_EXAMPLES
    01_in_process
)

foreach(example IN LISTS TOBUS_EXAMPLES)
    add_executable(tobus_example_${example} ${example}.cpp)
    target_link_libraries(tobus_example_${example} PRIVATE aw::tobus)
    target_compile_options(tobus_example_${example} PRIVATE ${AW_WARNINGS} ${AW_STRICT_WARNINGS})
endforeach()

# ---- modular examples (directory-per-example) --------------------------------

add_subdirectory(chat)
