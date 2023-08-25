cmake_minimum_required(VERSION 3.5)

include(CheckCXXCompilerFlag)

function(append_if condition value)
  if (${condition})
    foreach(variable ${ARGN})
      set(${variable} "${${variable}} ${value}" PARENT_SCOPE)
    endforeach(variable)
  endif()
endfunction()

project(eqbool)

enable_testing()

check_cxx_compiler_flag("-fdiagnostics-color" DIAGNOSTICS_COLOR_FLAG)
check_cxx_compiler_flag("-fno-exceptions" NO_EXCEPTIONS_FLAG)
check_cxx_compiler_flag("-fno-rtti" NO_RTTI_FLAG)
check_cxx_compiler_flag("-std=c++11" STD_CXX11_FLAG)
check_cxx_compiler_flag("-pedantic" PEDANTIC_FLAG)
check_cxx_compiler_flag("-Wall" WALL_FLAG)
check_cxx_compiler_flag("-Wextra" WEXTRA_FLAG)
check_cxx_compiler_flag("-W" W_FLAG)
check_cxx_compiler_flag("-Wconversion" WCONVERSION_FLAG)
check_cxx_compiler_flag("-Weverything" WEVERYTHING_FLAG)
check_cxx_compiler_flag("-Wno-c++98-compat" WNO_CXX98_COMPAT_FLAG)
check_cxx_compiler_flag("-Wno-c++98-compat-pedantic"
                        WNO_CXX98_COMPAT_PEDANTIC_FLAG)
check_cxx_compiler_flag("-Wno-shadow-field-in-constructor"
                        WNO_SHADOW_FIELD_IN_CONSTRUCTOR_FLAG)
check_cxx_compiler_flag("-Wno-shadow-field" WNO_SHADOW_FIELD)
check_cxx_compiler_flag("-Wno-padded" WNO_PADDED_FLAG)
check_cxx_compiler_flag("-Werror" WERROR_FLAG)
check_cxx_compiler_flag("-ferror-limit=1" ERROR_LIMIT_FLAG)
check_cxx_compiler_flag("-g" G_FLAG)

set(CXX_FLAGS)
append_if(DIAGNOSTICS_COLOR_FLAG "-fdiagnostics-color" CXX_FLAGS)
append_if(NO_EXCEPTIONS_FLAG "-fno-exceptions" CXX_FLAGS)
append_if(NO_RTTI_FLAG "-fno-rtti" CXX_FLAGS)
append_if(STD_CXX11_FLAG "-std=c++11" CXX_FLAGS)
append_if(PEDANTIC_FLAG "-pedantic" CXX_FLAGS)
append_if(WALL_FLAG "-Wall" CXX_FLAGS)
append_if(WEXTRA_FLAG "-Wextra" CXX_FLAGS)
append_if(W_FLAG "-W" CXX_FLAGS)
append_if(WCONVERSION_FLAG "-Wconversion" CXX_FLAGS)
append_if(WEVERYTHING_FLAG "-Weverything" CXX_FLAGS)
append_if(WNO_CXX98_COMPAT_FLAG "-Wno-c++98-compat" CXX_FLAGS)
append_if(WNO_CXX98_COMPAT_PEDANTIC_FLAG "-Wno-c++98-compat-pedantic"
          CXX_FLAGS)
append_if(WNO_SHADOW_FIELD_IN_CONSTRUCTOR_FLAG
          "-Wno-shadow-field-in-constructor" CXX_FLAGS)
append_if(WNO_SHADOW_FIELD "-Wno-shadow-field" CXX_FLAGS)
append_if(WNO_PADDED_FLAG "-Wno-padded" CXX_FLAGS)
append_if(WERROR_FLAG "-Werror" CXX_FLAGS)
append_if(ERROR_LIMIT_FLAG "-ferror-limit=1" CXX_FLAGS)
append_if(G_FLAG "-g" CXX_FLAGS)

set(EQBOOL_SRCS
    eqbool.cpp
    main.cpp)

set_source_files_properties(${EQBOOL_SRCS}
    PROPERTIES COMPILE_FLAGS ${CXX_FLAGS})

set(CADICAL_SRCS
    cadical/src/analyze.cpp
    cadical/src/arena.cpp
    cadical/src/assume.cpp
    cadical/src/averages.cpp
    cadical/src/backtrack.cpp
    cadical/src/backward.cpp
    cadical/src/bins.cpp
    cadical/src/block.cpp
    cadical/src/ccadical.cpp
    cadical/src/checker.cpp
    cadical/src/clause.cpp
    cadical/src/collect.cpp
    cadical/src/compact.cpp
    cadical/src/condition.cpp
    cadical/src/config.cpp
    cadical/src/constrain.cpp
    cadical/src/contract.cpp
    cadical/src/cover.cpp
    cadical/src/decide.cpp
    cadical/src/decompose.cpp
    cadical/src/deduplicate.cpp
    cadical/src/elim.cpp
    cadical/src/ema.cpp
    cadical/src/extend.cpp
    cadical/src/external.cpp
    cadical/src/external_propagate.cpp
    cadical/src/file.cpp
    cadical/src/flags.cpp
    cadical/src/flip.cpp
    cadical/src/format.cpp
    cadical/src/gates.cpp
    cadical/src/instantiate.cpp
    cadical/src/internal.cpp
    cadical/src/ipasir.cpp
    cadical/src/limit.cpp
    cadical/src/logging.cpp
    cadical/src/lookahead.cpp
    cadical/src/lratbuilder.cpp
    cadical/src/lratchecker.cpp
    cadical/src/lucky.cpp
    cadical/src/message.cpp
    cadical/src/minimize.cpp
    cadical/src/occs.cpp
    cadical/src/options.cpp
    cadical/src/parse.cpp
    cadical/src/phases.cpp
    cadical/src/probe.cpp
    cadical/src/profile.cpp
    cadical/src/proof.cpp
    cadical/src/propagate.cpp
    cadical/src/queue.cpp
    cadical/src/random.cpp
    cadical/src/reap.cpp
    cadical/src/reduce.cpp
    cadical/src/rephase.cpp
    cadical/src/report.cpp
    cadical/src/resources.cpp
    cadical/src/restart.cpp
    cadical/src/restore.cpp
    cadical/src/score.cpp
    cadical/src/shrink.cpp
    cadical/src/signal.cpp
    cadical/src/solution.cpp
    cadical/src/solver.cpp
    cadical/src/stats.cpp
    cadical/src/subsume.cpp
    cadical/src/terminal.cpp
    cadical/src/ternary.cpp
    cadical/src/tracer.cpp
    cadical/src/transred.cpp
    cadical/src/util.cpp
    cadical/src/var.cpp
    cadical/src/version.cpp
    cadical/src/vivify.cpp
    cadical/src/walk.cpp
    cadical/src/watch.cpp)

set_source_files_properties(${CADICAL_SRCS}
    PROPERTIES COMPILE_FLAGS "-DNBUILD -DQUIET")

add_executable(eqbool ${EQBOOL_SRCS} ${CADICAL_SRCS})

add_subdirectory(tests)
