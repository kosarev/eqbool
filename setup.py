#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#   Testing boolean expressions for equivalence.
#   https://github.com/kosarev/eqbool
#
#   Copyright (C) 2023-2025 Ivan Kosarev.
#   mail@ivankosarev.com
#
#   Published under the MIT license.


import inspect
import os
from setuptools import Extension, setup


here = os.path.abspath(os.path.dirname(__file__))

with open(os.path.join(here, 'README.md'), encoding='utf-8') as f:
    long_description = f.read()


eqbool_module = Extension(
    name='eqbool._eqbool',
    extra_compile_args=['-std=c++11', '-Wall', '-fno-exceptions', '-fno-rtti',
                        '-O3',
                        '-UNDEBUG',  # TODO
                        '-DNBUILD', '-DQUIET',
                        ],
    sources=[
        'eqbool/_eqbool.cpp',
        'eqbool.cpp',
        'cadical/src/analyze.cpp',
        'cadical/src/arena.cpp',
        'cadical/src/assume.cpp',
        'cadical/src/averages.cpp',
        'cadical/src/backtrack.cpp',
        'cadical/src/backward.cpp',
        'cadical/src/bins.cpp',
        'cadical/src/block.cpp',
        'cadical/src/ccadical.cpp',
        'cadical/src/checker.cpp',
        'cadical/src/clause.cpp',
        'cadical/src/collect.cpp',
        'cadical/src/compact.cpp',
        'cadical/src/condition.cpp',
        'cadical/src/config.cpp',
        'cadical/src/constrain.cpp',
        'cadical/src/contract.cpp',
        'cadical/src/cover.cpp',
        'cadical/src/decide.cpp',
        'cadical/src/decompose.cpp',
        'cadical/src/deduplicate.cpp',
        'cadical/src/drattracer.cpp',
        'cadical/src/elim.cpp',
        'cadical/src/ema.cpp',
        'cadical/src/extend.cpp',
        'cadical/src/external.cpp',
        'cadical/src/external_propagate.cpp',
        'cadical/src/file.cpp',
        'cadical/src/flags.cpp',
        'cadical/src/flip.cpp',
        'cadical/src/format.cpp',
        'cadical/src/frattracer.cpp',
        'cadical/src/gates.cpp',
        'cadical/src/idruptracer.cpp',
        'cadical/src/instantiate.cpp',
        'cadical/src/internal.cpp',
        'cadical/src/ipasir.cpp',
        'cadical/src/lidruptracer.cpp',
        'cadical/src/limit.cpp',
        'cadical/src/logging.cpp',
        'cadical/src/lookahead.cpp',
        'cadical/src/lratbuilder.cpp',
        'cadical/src/lratchecker.cpp',
        'cadical/src/lrattracer.cpp',
        'cadical/src/lucky.cpp',
        'cadical/src/message.cpp',
        'cadical/src/minimize.cpp',
        'cadical/src/occs.cpp',
        'cadical/src/options.cpp',
        'cadical/src/parse.cpp',
        'cadical/src/phases.cpp',
        'cadical/src/probe.cpp',
        'cadical/src/profile.cpp',
        'cadical/src/proof.cpp',
        'cadical/src/propagate.cpp',
        'cadical/src/queue.cpp',
        'cadical/src/random.cpp',
        'cadical/src/reap.cpp',
        'cadical/src/reduce.cpp',
        'cadical/src/rephase.cpp',
        'cadical/src/report.cpp',
        'cadical/src/resources.cpp',
        'cadical/src/restart.cpp',
        'cadical/src/restore.cpp',
        'cadical/src/score.cpp',
        'cadical/src/shrink.cpp',
        'cadical/src/signal.cpp',
        'cadical/src/solution.cpp',
        'cadical/src/solver.cpp',
        'cadical/src/stats.cpp',
        'cadical/src/subsume.cpp',
        'cadical/src/terminal.cpp',
        'cadical/src/ternary.cpp',
        'cadical/src/transred.cpp',
        'cadical/src/util.cpp',
        'cadical/src/var.cpp',
        'cadical/src/veripbtracer.cpp',
        'cadical/src/version.cpp',
        'cadical/src/vivify.cpp',
        'cadical/src/walk.cpp',
        'cadical/src/watch.cpp'],
    language='c++')


# TODO: Update the URL once we have a published documentation.
setup(name='eqbool',
      version='0.3',
      description='Testing boolean expressions for equivalence',
      long_description=long_description,
      long_description_content_type='text/markdown',
      author='Ivan Kosarev',
      author_email='mail@ivankosarev.com',
      url='https://github.com/kosarev/eqbool/',
      license='MIT',
      ext_modules=[eqbool_module],
      packages=['eqbool'],
      install_requires=[],
      package_data={},
      entry_points={},
      )
