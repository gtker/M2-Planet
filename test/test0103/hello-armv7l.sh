#! /bin/sh
## Copyright (C) 2017 Jeremiah Orians
## Copyright (C) 2021 deesix <deesix@tuta.io>
## This file is part of M2-Planet.
##
## M2-Planet is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## M2-Planet is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with M2-Planet.  If not, see <http://www.gnu.org/licenses/>.

set -x

ARCH="armv7l"
. test/env.inc.sh
TMPDIR="test/test0103/tmp-${ARCH}"

mkdir -p ${TMPDIR}

# Build the test
bin/M2-Planet \
	--architecture ${ARCH} \
	-f M2libc/${ARCH}/Linux/unistd.h \
	-f M2libc/stdlib.c \
	-f M2libc/${ARCH}/Linux/fcntl.h \
	-f M2libc/stdio.c \
	-f functions/file_print.c \
	-f functions/match.c \
	-f test/test0103/get_machine.c \
	--debug \
	-o ${TMPDIR}/get_machine.M1 \
	|| exit 1

# Build debug footer
blood-elf \
	-f ${TMPDIR}/get_machine.M1 \
	--entry _start \
	-o ${TMPDIR}/get_machine-footer.M1 \
	|| exit 2

# Macro assemble with libc written in M1-Macro
M1 \
	-f M2libc/${ARCH}/${ARCH}_defs.M1 \
	-f M2libc/${ARCH}/libc-full.M1 \
	-f ${TMPDIR}/get_machine.M1 \
	-f ${TMPDIR}/get_machine-footer.M1 \
	${ENDIANNESS_FLAG} \
	--architecture ${ARCH} \
	-o ${TMPDIR}/get_machine.hex2 \
	|| exit 3

# Resolve all linkages
hex2 \
	-f M2libc/${ARCH}/ELF-${ARCH}-debug.hex2 \
	-f ${TMPDIR}/get_machine.hex2 \
	${ENDIANNESS_FLAG} \
	--architecture ${ARCH} \
	--base-address ${BASE_ADDRESS} \
	-o test/results/test0103-${ARCH}-binary \
	|| exit 4

# Ensure binary works if host machine supports test
if [ "$(get_machine ${GET_MACHINE_FLAGS})" = "${ARCH}" ]
then
	# Verify that the compiled program returns the correct result
	out=$(./test/results/test0103-${ARCH}-binary ${GET_MACHINE_FLAGS} 2>&1 )
	[ 0 = $? ] || exit 5
	[ "$out" = "${ARCH}" ] || exit 6
fi
exit 0
