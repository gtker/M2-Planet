#! /bin/sh
## Copyright (C) 2017 Jeremiah Orians
## Copyright (C) 2020-2021 deesix <deesix@tuta.io>
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

set -ex

TMPDIR="test/test0013/tmp-aarch64"
mkdir -p ${TMPDIR}

# Build the test
bin/M2-Planet \
	--architecture aarch64 \
	-f M2libc/AArch64/Linux/unistd.h \
	-f M2libc/stdlib.c \
	-f M2libc/AArch64/Linux/fcntl.h \
	-f M2libc/stdio.c \
	-f test/test0013/break-while.c \
	-o ${TMPDIR}/break-while.M1 \
	|| exit 1

# Macro assemble with libc written in M1-Macro
M1 \
	-f M2libc/AArch64/aarch64_defs.M1 \
	-f M2libc/AArch64/libc-full.M1 \
	-f ${TMPDIR}/break-while.M1 \
	--LittleEndian \
	--architecture aarch64 \
	-o ${TMPDIR}/break-while.hex2 \
	|| exit 2

# Resolve all linkages
hex2 \
	-f M2libc/AArch64/ELF-aarch64.hex2 \
	-f ${TMPDIR}/break-while.hex2 \
	--LittleEndian \
	--architecture aarch64 \
	--BaseAddress 0x400000 \
	-o test/results/test0013-aarch64-binary \
	--exec_enable \
	|| exit 3

# Ensure binary works if host machine supports test
if [ "$(get_machine ${GET_MACHINE_FLAGS})" = "aarch64" ]
then
	. ./sha256.sh
	# Verify that the resulting file works
	./test/results/test0013-aarch64-binary >| test/test0013/proof || exit 4
	out=$(sha256_check test/test0013/proof.answer)
	[ "$out" = "test/test0013/proof: OK" ] || exit 5
fi
exit 0
