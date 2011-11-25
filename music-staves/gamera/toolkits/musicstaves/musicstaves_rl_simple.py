# -*- mode: python; indent-tabs-mode: nil; tab-width: 4 -*-
# vim: set tabstop=4 shiftwidth=4 expandtab:

#
# Copyright (C) 2004 Christoph Dalitz, Thomas Karsten
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#  
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

from gamera.gui import has_gui
## from wx import *
from gamera import toolkit
from gamera.core import *
from gamera.toolkits.musicstaves.musicstaves import MusicStaves as _virtual_MusicStaves
from gamera.toolkits.musicstaves.musicstaves import StaffObj
from gamera.toolkits.musicstaves.stafffinder_projections import StaffFinder_projections

##############################################################################
#
#
class MusicStaves_rl_simple(_virtual_MusicStaves):
    """This staff removal algorithm is a simplified implementation of some
ideas in Ichiro Fujinaga's dissertation (see below).

The Method:

  The basic idea is that black horizontal run lengths within a staff line are
  typically greater than staffspace_height, while the musical symbols 
  have shorter runlenghts. Thus only sufficiently short runlengths are kept.
  Stafflines are detected with `StaffFinder_projections`__.
  
.. __: gamera.toolkits.musicstaves.stafffinder_projections.StaffFinder_projections.html

  In contrast to Fujinaga's approach, we do not try a deskewing by
  shearing. Moreover we only care for crossing symbols in a very crude way.

Assumptions and Limitations:

- Staff lines must not be heavily broken

- Skew correction (eg. with ``rotation_angle_projections`` from
  the gamera core distribution) should be applied beforehand

- Symbols crossing staff lines are often distorted

- Staff lines highly varying in thickness are not completely removed

References:

- Ichiro Fujinaga: *Adaptive Optical Music Recognition*. Dissertation,
  McGill University Montreal, Canada (1996)

- Ichiro Fujinaga: *Staff Detection and Removal*. in S. George (editor):
  \"Visual Perception of Music Notation\". Idea Group (2004)

:Author: Christoph Dalitz and Thomas Karsten
"""

    def __init__(self, img, staffline_height=0, staffspace_height=0):
        _virtual_MusicStaves.__init__(self, img,\
                staffline_height, staffspace_height)
        # use external class for staff line finding
        self.stafffinder = StaffFinder_projections(img,\
                             staffline_height=self.staffline_height,\
                             staffspace_height=self.staffspace_height)
        # for caching staff position information
        self.__staffobjlist = None


    ######################################################################
    # remove_staves
    #
    # Thomas Karsten, Christoph Dalitz
    #
    def remove_staves(self, crossing_symbols='all', num_lines=0):
        """Detects and removes staff lines from a music/tablature image.

Signature:

  ``remove_staves(crossing_symbols='all', num_lines=0)``

with

  *crossing_symbols*:
    Determines which symbols crossing staff lines should be kept intact.
    Supported values are 'all', 'bars' or 'none'.

  *num_lines*:
    Number of lines within one staff. A value of zero for automatic
    detection is supported, but if you give the actual number, staff line
    detection will improve.
"""

        # save for later use
        orig_image=self.image.image_copy()

        # save the tall runs of the image for later
        tall_runs=self.image.image_copy()
        if crossing_symbols == 'all':
            # try to save letters/numbers/bars crossing staves
            tall_runs.filter_short_runs(self.staffline_height * 2 + 1, 'black')
        elif crossing_symbols == 'bars':
            # save only bar lines and flag stems
            tall_runs.filter_short_runs(self.staffspace_height + 1, 'black')
        elif crossing_symbols == 'none':
            tall_runs.fill_white()
        else:
            raise RuntimeError, "Invalid value '%s' for crossing_symbols" % crossing_symbols
        #tall_runs.save_tiff("tall_runs.tiff")

        # keep only the stafflines
        staves=self.image.image_copy()
        # Solution by toom (*not* equivalent to Fujinaga):
        staves.filter_narrow_runs(int(1.5 * self.staffspace_height), 'black')
        # Fujinaga removes short cc's, assuming a wide distance
        # between noteheads/fretsymbols:
        #staves.xor_image(tall_runs, True)
        #for c in staves.cc_analysis:
        #   if c.ncols < staffspace_height:
        #       c.fill_white()
        #staves.save_tiff("staves.tiff")

        # use external class for staff line finding
        self.stafffinder.find_staves(num_lines=num_lines, preprocessing=2)

        # remove the stafflines
        self.image.xor_image(staves, True)
        #self.image.save_tiff("staves-xor-orig.tiff")

        # remove remaining speckles from staff edge
        # [Possible improvement (CD):
        #   remove only shortruns/ccs close to staff positions]
        self.image.filter_short_runs(int(self.staffline_height / 3.0 + 0.5), 'black')
        #for c in self.image.cc_analysis():
        #   if c.nrows <= self.staffline_height * 0.5:
        #       c.fill_white()
        #self.image.despeckle(self.staffline_height)

        # copy over previously saved stuff
        self.image.or_image(tall_runs, True)

        # isolate remaining thin staff filaments
        filaments=self.image.image_copy()
        #rgb=self.image.to_rgb()
        # subtract all tall runs in original image so that remaining
        # thin filaments are isolated
        tall_runs=orig_image.image_copy()
        tall_runs.filter_short_runs(int(self.staffline_height * 2), 'black')
        filaments.subtract_images(tall_runs, True)
        #rgb.highlight(filaments, RGBPixel(255, 0, 0))
        for c in filaments.cc_analysis():
            if c.nrows <= self.staffline_height*0.8:
                pass
            elif c.nrows < self.staffline_height * 1.0 and \
                 c.ncols > c.nrows * 2:
                pass
            else:
                #rgb.highlight(c, RGBPixel(0, 255, 0))
                c.fill_white()

        #rgb.save_PNG("filaments.png")
        self.image.subtract_images(filaments, True)
        #self.image.despeckle(self.staffline_height*2)

        del staves
        del tall_runs
        del filaments
        del orig_image


    ######################################################################
    # get_staffpos
    #
    # Christoph Dalitz
    #
    def get_staffpos(self, x=0):
        """Returns the y-positions of all staff lines at a given x-position.
Can only be called *after* ``remove_staves``.

Signature:

  ``get_staffpos(x=0)``

with

  *x*:
    This parameter has no effect, because the staffline is assumed to
    be horizontal.

The return value is a list of `StaffObj`__.

.. __: gamera.toolkits.musicstaves.musicstaves.StaffObj.html#staffobj
"""
        if not self.__staffobjlist and self.stafffinder.linelist != None:
            self.__staffobjlist = []
            for i,staff in enumerate(self.stafffinder.linelist):
                if len(staff) > 0:
                    so = StaffObj()
                    so.staffno = i+1
                    so.yposlist=[avg.average_y for avg in \
                                 [line.to_average() for line in staff]]
                    so.staffrect = Rect(Point(so.yposlist[0],staff[0].left_x),\
                                     Point(so.yposlist[-1],staff[-1].right_x))
                    self.__staffobjlist.append(so)

        return self.__staffobjlist


