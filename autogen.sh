#! /bin/sh

LIBTOOLIZE=libtoolize
AUTOMAKE=automake-1.8
ACLOCAL=aclocal-1.8
AUTOCONF=autoconf
AUTOHEADER=autoheader

# Check for binaries

[ "x$(which ${LIBTOOLIZE})x" = "xx" ] && {
    echo "${LIBTOOLIZE} not found. Please install it."
    exit 1
}

[ "x$(which ${AUTOMAKE})x" = "xx" ] && {
    echo "${AUTOMAKE} not found. Please install it."
    exit 1
}

[ "x$(which ${ACLOCAL})x" = "xx" ] && {
    echo "${ACLOCAL} not found. Please install it."
    exit 1
}

[ "x$(which ${AUTOCONF})x" = "xx" ] && {
    echo "${AUTOCONF} not found. Please install it."
    exit 1
}

"${ACLOCAL}" \
&& "${LIBTOOLIZE}" \
&& "${AUTOHEADER}" \
&& "${AUTOMAKE}" --add-missing \
&& "${AUTOCONF}"

$(dirname "${0}")/configure "$@"
