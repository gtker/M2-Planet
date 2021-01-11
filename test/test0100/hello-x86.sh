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

TMPDIR="test/test0100/tmp-x86"
mkdir -p ${TMPDIR}

# Build the test
./bin/M2-Planet \
	--architecture x86 \
	-f test/common_x86/functions/exit.c \
	-f test/common_x86/functions/file.c \
	-f functions/file_print.c \
	-f test/common_x86/functions/malloc.c \
	-f functions/calloc.c \
	-f functions/match.c \
	-f test/test0100/blood-elf.c \
	--debug \
	--bootstrap-mode \
	-o ${TMPDIR}/blood-elf.M1 \
	|| exit 1

# Build debug footer
blood-elf \
	-f ${TMPDIR}/blood-elf.M1 \
	--entry _start \
	-o ${TMPDIR}/blood-elf-footer.M1 \
	|| exit 2

# Macro assemble with libc written in M1-Macro
M1 \
	-f test/common_x86/x86_defs.M1 \
	-f test/common_x86/libc-core.M1 \
	-f ${TMPDIR}/blood-elf.M1 \
	-f ${TMPDIR}/blood-elf-footer.M1 \
	--LittleEndian \
	--architecture x86 \
	-o ${TMPDIR}/blood-elf.hex2 \
	|| exit 3

# Resolve all linkages
hex2 \
	-f test/common_x86/ELF-i386-debug.hex2 \
	-f ${TMPDIR}/blood-elf.hex2 \
	--LittleEndian \
	--architecture x86 \
	--BaseAddress 0x8048000 \
	-o test/results/test0100-x86-binary \
	--exec_enable \
	|| exit 4

# Ensure binary works if host machine supports test
if [ "$(get_machine ${GET_MACHINE_FLAGS})" = "x86" ]
then
	# Verify that the compiled program returns the correct result
	out=$(./test/results/test0100-x86-binary --version 2>&1 )
	[ 0 = $? ] || exit 5
	[ "$out" = "blood-elf 0.1
(Basically Launches Odd Object Dump ExecutabLe Files" ] || exit 6

	. ./sha256.sh
	# Verify that the resulting file works
	./test/results/test0100-x86-binary -f test/test0100/test.M1 -o test/test0100/proof || exit 7
	out=$(sha256_check test/test0100/proof.answer)
	[ "$out" = "test/test0100/proof: OK" ] || exit 8
fi
exit 0
