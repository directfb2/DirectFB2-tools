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

executable('dfbg',             'dfbg.c',             dependencies:  directfb_dep,          install: true)
executable('dfbinfo',          'dfbinfo.c',          dependencies:  directfb_dep,          install: true)
executable('dfiffinfo',        'dfiffinfo.c',        dependencies:  directfb_dep,          install: true)
executable('dfiffcolorkeying', 'dfiffcolorkeying.c', dependencies:  directfb_dep,          install: true)

if fusionsound_dep.found()
executable('fsvolume',         'fsvolume.c',         dependencies:  fusionsound_dep,       install: true)
endif

if divine_dep.found()
executable('spooky',           'spooky.c',           dependencies: [divine_dep, libm_dep], install: true)
endif
