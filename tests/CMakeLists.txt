
# Do not use file(GLOB ...) as it doesn't automatically discover
# new files.
set(TESTS
    and.test
    eq.test
    ifelse.test
    not.test
    or.test
    sat.test
    term.test)

foreach(test ${TESTS})
    add_test(NAME ${test} COMMAND tester ${CMAKE_CURRENT_SOURCE_DIR}/${test})
endforeach()
