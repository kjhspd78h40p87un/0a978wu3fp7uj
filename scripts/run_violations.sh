#!/bin/bash

BAZEL=bazel-4.1.0

#Mongo copying databases to compare results
mongo --eval 'db.copyDatabase("eesier_evid3_max", "eesier_evid3_max_copy")'
mongo --eval 'db.copyDatabase("eesier_evid3_meet", "eesier_evid3_meet_copy")'
mongo --eval 'db.copyDatabase("eesier_evid3_join", "eesier_evid3_join_copy")'

${BAZEL} run //cli:main -- --db-name eesier_evid3_max checker GetViolationsAll --violation-type unused --confidence-threshold 1 --overwrite
${BAZEL} run //cli:main -- --db-name eesier_evid3_meet checker GetViolationsAll --violation-type unused --confidence-threshold 1 --overwrite
${BAZEL} run //cli:main -- --db-name eesier_evid3_join checker GetViolationsAll --violation-type unused --confidence-threshold 1 --overwrite
${BAZEL} run //cli:main -- --db-name eesier_evid3_max_copy checker GetViolationsAll --violation-type unused --confidence-threshold 100 --overwrite

# Pidgin
${BAZEL} run //cli:main -- --db-name eesier_evid3_max checker ListViolationsDiff --db-1 eesier_evid3_meet --db-2 eesier_evid3_max_copy --violation-type unused --bitcode-uri file:///home/eesier/eesier/testdata/benchmarks/bitcode/pidgin-reg2mem.bc > testdata/benchmarks/violations/meet/pidgin-unused.txt
# Little FS
${BAZEL} run //cli:main -- --db-name eesier_evid3_max checker ListViolationsDiff --db-1 eesier_evid3_meet --db-2 eesier_evid3_max_copy --violation-type unused --bitcode-uri file:///home/eesier/eesier/testdata/benchmarks/bitcode/littlefs-reg2mem.bc > testdata/benchmarks/violations/meet/littlefs-unused.txt
# Netdata
${BAZEL} run //cli:main -- --db-name eesier_evid3_max checker ListViolationsDiff --db-1 eesier_evid3_meet --db-2 eesier_evid3_max_copy --violation-type unused --bitcode-uri file:///home/eesier/eesier/testdata/benchmarks/bitcode/netdata-reg2mem.bc > testdata/benchmarks/violations/meet/netdata-unused.txt
# MbedTLS
${BAZEL} run //cli:main -- --db-name eesier_evid3_max checker ListViolationsDiff --db-1 eesier_evid3_join --db-2 eesier_evid3_max_copy --violation-type unused --bitcode-uri file:///home/eesier/eesier/testdata/benchmarks/bitcode/mbedtls-reg2mem.bc > testdata/benchmarks/violations/join/mbedtls-unused.txt
# OpenSSL
${BAZEL} run //cli:main -- --db-name eesier_evid3_max checker ListViolationsDiff --db-1 eesier_evid3_join --db-2 eesier_evid3_max_copy --violation-type unused --bitcode-uri file:///home/eesier/eesier/testdata/benchmarks/bitcode/openssl-reg2mem.bc > testdata/benchmarks/violations/join/openssl-unused.txt
# Zlib
${BAZEL} run //cli:main -- --db-name eesier_evid3_max checker ListViolationsDiff --db-1 eesier_evid3_meet --db-2 eesier_evid3_max_copy --violation-type unused --bitcode-uri file:///home/eesier/eesier/testdata/benchmarks/bitcode/zlib-reg2mem.bc > testdata/benchmarks/violations/meet/zlib-unused.txt

