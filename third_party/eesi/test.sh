./src/build/eesi --command bugs --specs results/min/netdata-specs.txt --bitcode bitcode23/netdata-reg2mem.bc > results/min/bugs/netdata-bugs.txt
./src/build/eesi --command bugs --specs results/max/netdata-specs.txt --bitcode bitcode23/netdata-reg2mem.bc > results/max/bugs/netdata-bugs.txt
./src/build/eesi --command bugs --specs results/artifact/netdata-specs.txt --bitcode bitcode23/netdata-reg2mem.bc > results/artifact/bugs/netdata-bugs.txt

./src/build/eesi --command bugs --specs results/min/openssl-specs.txt --bitcode bitcode23/openssl-reg2mem.bc > results/min/bugs/openssl-bugs.txt
./src/build/eesi --command bugs --specs results/max/openssl-specs.txt --bitcode bitcode23/openssl-reg2mem.bc > results/max/bugs/openssl-bugs.txt
./src/build/eesi --command bugs --specs results/artifact/openssl-specs.txt --bitcode bitcode23/openssl-reg2mem.bc > results/artifact/bugs/openssl-bugs.txt

./src/build/eesi --command bugs --specs results/min/mbedtls-specs.txt --bitcode bitcode23/mbedtls-reg2mem.bc > results/min/bugs/mbedtls-bugs.txt
./src/build/eesi --command bugs --specs results/max/mbedtls-specs.txt --bitcode bitcode23/mbedtls-reg2mem.bc > results/max/bugs/mbedtls-bugs.txt
./src/build/eesi --command bugs --specs results/artifact/mbedtls-specs.txt --bitcode bitcode23/mbedtls-reg2mem.bc > results/artifact/bugs/mbedtls-bugs.txt

./src/build/eesi --command bugs --specs results/min/zlib-specs.txt --bitcode bitcode23/zlib-reg2mem.bc > results/min/bugs/zlib-bugs.txt
./src/build/eesi --command bugs --specs results/max/zlib-specs.txt --bitcode bitcode23/zlib-reg2mem.bc > results/max/bugs/zlib-bugs.txt
./src/build/eesi --command bugs --specs results/artifact/zlib-specs.txt --bitcode bitcode23/zlib-reg2mem.bc > results/artifact/bugs/zlib-bugs.txt

./src/build/eesi --command bugs --specs results/min/pidgin-specs.txt --bitcode bitcode23/pidgin-reg2mem.bc > results/min/bugs/pidgin-bugs.txt
./src/build/eesi --command bugs --specs results/max/pidgin-specs.txt --bitcode bitcode23/pidgin-reg2mem.bc > results/max/bugs/pidgin-bugs.txt
./src/build/eesi --command bugs --specs results/artifact/pidgin-specs.txt --bitcode bitcode23/pidgin-reg2mem.bc > results/artifact/bugs/pidgin-bugs.txt

./src/build/eesi --command bugs --specs results/min/littlefs-specs.txt --bitcode bitcode23/littlefs-reg2mem.bc > results/min/bugs/littlefs-bugs.txt
./src/build/eesi --command bugs --specs results/max/littlefs-specs.txt --bitcode bitcode23/littlefs-reg2mem.bc > results/max/bugs/littlefs-bugs.txt
./src/build/eesi --command bugs --specs results/artifact/littlefs-specs.txt --bitcode bitcode23/littlefs-reg2mem.bc > results/artifact/bugs/littlefs-bugs.txt

