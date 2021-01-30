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

# To be sourced from test scripts, with ARCH already set.

case "${ARCH}" in
	aarch64)
		BASE_ADDRESS="0x00400000"
		ENDIANNESS_FLAG="--little-endian"
		;;
	amd64)
		BASE_ADDRESS="0x00600000"
		ENDIANNESS_FLAG="--little-endian"
		;;
	armv7l)
		BASE_ADDRESS="0x00010000"
		ENDIANNESS_FLAG="--little-endian"
		;;
	x86)
		BASE_ADDRESS="0x08048000"
		ENDIANNESS_FLAG="--little-endian"
		;;
	*)
		echo "$0: Unknown architecture (${ARCH})."
		exit 77
		;;
esac
