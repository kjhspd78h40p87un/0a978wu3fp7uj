#!/bin/bash

CMD="specs"          # specs or bugs
INPUT="artifact"        # artifact or camera
BC_DIR="bitcode23"
DK_DIR="config23"
TARGET=$1       # project or "all" or "small"
DOCKER="local"       # docker or local

if [ -z "$TARGET" ]; then
  TARGET="all"
fi

if [ -z "$BC_DIR" ] || [ -z "$DK_DIR" ] || [ -z "$TARGET" ]; then
    echo "Usage: tabledata.sh [<project>]"
    exit 1
fi

RUN_PIDGIN_OTRNG=0
RUN_OPENSSL=0
RUN_MBEDTLS=0
RUN_NETDATA=0
RUN_LITTLEFS=0
RUN_ZLIB=0

if [ "$TARGET" = "all" ]; then
    RUN_PIDGIN_OTRNG=1
    RUN_OPENSSL=1
    RUN_MBEDTLS=1
    RUN_NETDATA=1
    RUN_LITTLEFS=1
    RUN_ZLIB=1
elif [ "$TARGET" = "small" ]; then
    RUN_PIDGIN_OTRNG=1
    RUN_OPENSSL=1
    RUN_MBEDTLS=1
    RUN_NETDATA=1
    RUN_ZLIB=1
elif [ "$TARGET" = "pidgin-otrng" ]; then
    RUN_PIDGIN_OTRNG=1
elif [ "$TARGET" = "openssl" ]; then
    RUN_OPENSSL=1
elif [ "$TARGET" = "mbedtls" ]; then
    RUN_MBEDTLS=1
elif [ "$TARGET" = "netdata" ]; then
    RUN_NETDATA=1
elif [ "$TARGET" = "littlefs" ]; then
    RUN_LITTLEFS=1
elif [ "$TARGET" = "zlib" ]; then
    RUN_ZLIB=1
fi

# Get source directory of script
# https://stackoverflow.com/questions/59895/get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

cd "$DIR/../.."

mkdir -p results/$INPUT

if [ "$DOCKER" = "docker" ]; then
    EESI_CMD="docker run --rm -v $PWD:/d defreez/eesi:latest"
elif [ "$DOCKER" = "local" ]; then
    EESI_CMD="src/build/eesi"
fi

if [ "$RUN_PIDGIN_OTRNG" = "1" ]; then
    BC="${BC_DIR}/pidgin-reg2mem.bc"
    INPUTSPECS="${DK_DIR}/input-specs-pidgin.txt"
    OUTPUTSPECS="results/$INPUT/pidgin-specs.txt"
    if [ "$CMD" = "specs" ]; then
        echo "Pidgin OTRNGv4 specs..."
        (time $EESI_CMD --bitcode $BC --command specs --inputspecs $INPUTSPECS | grep -v WARNING | sort -u) > $OUTPUTSPECS
    fi
fi

if [ "$RUN_OPENSSL" = "1" ]; then
    BC="${BC_DIR}/openssl-reg2mem.bc"
    INPUTSPECS="${DK_DIR}/input-specs-openssl.txt"
    ERRORONLY="${DK_DIR}/error-only-openssl.txt"
    ERRORCODES="${DK_DIR}/error-code-openssl.txt"
    OUTPUTSPECS="results/$INPUT/openssl-specs.txt"
    if [ "$CMD" = "specs" ]; then
        echo "OpenSSL specs..."
        (time $EESI_CMD --bitcode $BC --command specs --inputspecs $INPUTSPECS --erroronly $ERRORONLY --errorcodes $ERRORCODES | grep -v WARNING | sort -u) > $OUTPUTSPECS
    fi
fi


if [ "$RUN_MBEDTLS" = "1" ]; then
    BC="${BC_DIR}/mbedtls-reg2mem.bc"
    INPUTSPECS="${DK_DIR}/input-specs-mbedtls.txt"
    ERRORONLY="${DK_DIR}/error-only-mbedtls.txt"
    ERRORCODES="${DK_DIR}/error-code-mbedtls.txt"
    OUTPUTSPECS="results/$INPUT/mbedtls-specs.txt"
    if [ "$CMD" = "specs" ]; then
        echo "mbedTLS specs..."
        (time $EESI_CMD --bitcode $BC --command specs --inputspecs $INPUTSPECS --erroronly $ERRORONLY --errorcodes $ERRORCODES | grep -v WARNING | sort -u) > $OUTPUTSPECS
    fi
fi

if [ "$RUN_NETDATA" = "1" ]; then
    BC="${BC_DIR}/netdata-reg2mem.bc"
    INPUTSPECS="${DK_DIR}/input-specs-netdata.txt"
    OUTPUTSPECS="results/$INPUT/netdata-specs.txt"
    if [ "$CMD" = "specs" ]; then
        echo "netdata specs..."
        (time $EESI_CMD --bitcode $BC --command specs --inputspecs $INPUTSPECS | grep -v WARNING | sort -u) > $OUTPUTSPECS 
    fi
fi

if [ "$RUN_LITTLEFS" = "1" ]; then
    BC="${BC_DIR}/littlefs-reg2mem.bc"
    INPUTSPECS="${DK_DIR}/input-specs-littlefs.txt"
    ERRORCODES="${DK_DIR}/error-code-littlefs.txt"
    OUTPUTSPECS="results/$INPUT/littlefs-specs.txt"
    if [ "$CMD" = "specs" ]; then
        echo "LittleFS specs..."
        (time $EESI_CMD --bitcode $BC --command specs --inputspecs $INPUTSPECS --errorcodes $ERRORCODES | grep -v WARNING | sort -u) > $OUTPUTSPECS 
    fi
fi

if [ "$RUN_ZLIB" = "1" ]; then
    BC="${BC_DIR}/zlib-reg2mem.bc"
    INPUTSPECS="${DK_DIR}/input-specs-zlib.txt"
    ERRORCODES="${DK_DIR}/error-code-zlib.txt"
    OUTPUTSPECS="results/$INPUT/zlib-specs.txt"
    if [ "$CMD" = "specs" ]; then
        echo "zlib specs..."
        (time $EESI_CMD --bitcode $BC --command specs --inputspecs $INPUTSPECS --errorcodes $ERRORCODES | grep -v WARNING | sort -u) > $OUTPUTSPECS
    fi
fi
