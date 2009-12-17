#! /bin/sh

LIBTOOLIZE=libtoolize
AUTOMAKE=automake
ACLOCAL=aclocal
AUTOCONF=autoconf
AUTOHEADER=autoheader
GTKDOCIZE=gtkdocize

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

[ "x$(which ${GTKDOCIZE})x" = "xx" ] && {
    echo "${GTKDOCIZE} not found. Please install it."
    exit 1
}

gtkdocize --copy || exit 1

"${ACLOCAL}" \
&& "${LIBTOOLIZE}" \
&& "${AUTOHEADER}" \
&& "${AUTOMAKE}" --add-missing \
&& "${AUTOCONF}"

$(dirname "${0}")/configure "$@"
