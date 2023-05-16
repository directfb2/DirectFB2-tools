#  This file is part of DirectFB.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA

project('DirectFB2-tools', 'c',
        version: '2.0.0')

directfb_dep = dependency('directfb-internal')

endian_def = []
if host_machine.endian() == 'big'
  endian_def = '-DWORDS_BIGENDIAN'
endif

divine_dep = dependency('divine', required: false)
if divine_dep.found()
  libm_dep = meson.get_compiler('c').find_library('m')
endif

fusionsound_dep = dependency('fusionsound', required: false)

enable_ft2 = get_option('ft2')
enable_png = get_option('png')

if enable_ft2
  ft2_dep = dependency('freetype2', required: false)

  if not ft2_dep.found()
    enable_ft2 = false
  endif
endif

png_def = []
png_dep = []
if enable_png
  png_dep = dependency('libpng', required: false)

  if not png_dep.found()
    enable_png = false
  else
    png_def = ['-DHAVE_PNG', endian_def]
  endif
endif

subdir('src')
