
file(GLOB TESTS "*.test")

foreach(test ${TESTS})
    add_test(NAME ${test} COMMAND eqbool ${test})
endforeach()
