# -*- mode: python; indent-tabs-mode: nil; tab-width: 4 -*-
# vim: set tabstop=4 shiftwidth=4 expandtab:

#
# Copyright (C) 2004-2006 Christoph Dalitz, Florian Pose
#               2007-2011 Christoph Dalitz
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

from gamera import toolkit
from gamera.args import *
from gamera.core import *

from gamera.toolkits.musicstaves.musicstaves \
     import MusicStaves as _virtual_MusicStaves
from gamera.toolkits.musicstaves.musicstaves import StaffObj

from gamera.toolkits.musicstaves.stafffinder_dalitz \
     import StaffFinder_dalitz
from gamera.toolkits.musicstaves.plugins.line_tracking \
     import remove_line_around_skeletons

#----------------------------------------------------------------

class MusicStaves_linetracking(_virtual_MusicStaves):
    """This staff removal class implements the most obvious approach
to staff line removal: first find the staff lines, then follow them
(\"line tracking\") and remove pixels based upon some decision criterion.

For an overview of decision criteria, whether a pixel belongs to
a symbol, see the reference below. Note that this class does *not* use
the staff line finding method described in this reference, because we
have found this \"wobble\" method to be rather unstable. Hence we
use the more robust `StaffFinder_dalitz`_ instead. To use a different
staff finding algorithm, set the property ``staffinder`` to a class
derived from `StaffFinder`_, as in the following example:

   .. code:: Python

      sf = StaffFinder_projections(image)
      ms = MusicStaves_linetracking(image)
      ms.stafffinder = sf

.. _StaffFinder_dalitz: gamera.toolkits.musicstaves.stafffinder_dalitz.StaffFinder_dalitz.html

.. _StaffFinder: gamera.toolkits.musicstaves.stafffinder.StaffFinder.html


References:

- D Bainbridge and T C Bell: `Dealing with superimposed objects in optical
  music recognition`__. 6th International Conference on Image Processing
  and its Applications (1997)

.. __: http://www.cs.waikato.ac.nz/~davidb/publications/ipa97/

:Author: Christoph Dalitz and Florian Pose
"""
    #------------------------------------------------------------

    def __init__(self, img, \
                 staffline_height = 0, staffspace_height = 0):

        _virtual_MusicStaves.__init__(self, img, \
                                      staffline_height, \
                                      staffspace_height)

        # Use external class for staff line finding
        self.stafffinder = StaffFinder_dalitz(img, \
                                              self.staffline_height, \
                                              self.staffspace_height)

        crossing_symbols_list = ['all', 'bars', 'none']
        symbol_criterion_list = ['runlength', 'secondchord', \
                                 'mask', 'skewedruns', 'vectorfieldruns']

        self.remove_staves_args = Args([
            ChoiceString("crossing_symbols", \
                         crossing_symbols_list), \
            Int("num_lines", default = 0), \
            ChoiceString("symbol_criterion", \
                         symbol_criterion_list, \
                         default = "runlength"), \
            Int("threshold", default = 0), \
            Int("debug", default = 0)])

        # for caching staff position information
        self.__staffobjlist = []

    #------------------------------------------------------------

    def remove_staves(self, crossing_symbols = 'all', \
                      num_lines = 0, symbol_criterion = 'runlength', \
                      threshold = 0, debug = 0, dx = 0, dy = 0):

        """Detects and removes staff lines from a music/tablature image.

The stafflines are found using ``self.staffinder``, which is a
``StaffFinder_dalitz`` by default. When the ``self.stafffinder.linelist``
is non empty, the staff lines form this list are used; otherwise
``self.stafffinder.find_staves`` is called.

Signature:

  ``remove_staves(crossing_symbols='all', num_lines=0, symbol_criterion='runlength')``

with

  *crossing_symbols*:
    Determines which symbols crossing staff lines should be kept intact.
    Supported values are 'all', 'bars' or 'none'.

  *num_lines*:
    Number of lines within one staff. A value of zero for automatic
    detection is supported, but if you give the actual number, staff line
    detection will improve.

  *symbol_criterion*:
    Determines which criterion is used for identifying symbol pixels which
    are not to be removed (only has effect when *crossing_symbols* = ``'all'``).
    Options are

      ``runlength``
        Removes only vertical runlengths shorter than 2 * *staffline_height*.
      ``secondchord``
        Checks whether a second long chord different from the chord from
        the staff line passes a pixel (Martin and Bellissant). Only the
        staff skeleton pixels are tested and when the criterion is true,
        the entire black vertical runlength is kept, otherwise it is removed.
      ``mask``
        Checks for pixels within a `T`-shaped mask (Clarke, Brown and Thorne)
      ``skewedruns``
        Checks whether the pixel is part of a skewed run that extents
        beyond the staff line. The skewed runs are found by moving parallel
        to the staff skeleton at a distance plusminus 1.5 * *staffline_height*
        and keeping tall skewed runs through the found black pixels
        (see `keep_tall_skewed_runs`_)
      ``vectorfieldruns``
        Similar to ``skewedruns``, but only skewed runs in the direction
        of the vector field, as computed by `compute_longest_chord_vectors`_,
        are considered.

.. _keep_tall_skewed_runs: musicstaves.html#keep-tall-skewed-runs
.. _compute_longest_chord_vectors: musicstaves.html#compute-longest-chord-vectors

  *threshold*:
    The height up to which a horizontal black runlength crossing the
    skeleton is interpreted as part of the staff line. Setting 
    to zero results in 2 \* *staffline_height*.

  *debug*:
    Debug level. 0 = No debug messages. 1 = Only text messages. 2 = Create
    debug image 'musicstaves_linetracking_debug.png' in current directory.

For details and references see D Bainbridge and T C Bell: `Dealing with
superimposed objects in optical music recognition`__. 6th International
Conference on Image Processing and its Applications (1997)

.. __: http://www.cs.waikato.ac.nz/~davidb/publications/ipa97/
"""

        if threshold == 0:
            if (1.0*self.staffspace_height)/self.staffline_height < 2.5:
                threshold = int(1.5*self.staffline_height+1)
            else:
                threshold = self.staffline_height * 2

        #max_gap_height = self.staffline_height / 3
        max_gap_height = 0

        if debug >= 1:
            print "Staffline height:", self.staffline_height
            print "Threshold:", threshold
            print "Maximum gap height:", max_gap_height

        if debug > 0:
            find_staves_debug = 1
        else:
            find_staves_debug = 0

        # Find the staves
        if not self.stafffinder.linelist:
            self.stafffinder.find_staves(num_lines = num_lines, \
                                             debug = find_staves_debug)

        if debug >= 1:
            print "Saving images for later use..."

        # Save for later use
        orig_image = self.image.image_copy()

        if crossing_symbols == 'all':

            if symbol_criterion == 'runlength':

                if debug >= 1:
                    print "   Rescueing vertical black runs..."

                # Try to save letters/numbers/bars crossing staves
                rescue_image = self.image.image_copy()
                rescue_image.filter_short_runs(threshold, 'black')

            elif symbol_criterion == 'secondchord':

                rescue_image = Image(self.image);

                if debug >= 1:
                    print "   Rescueing using second chords..."

                # remove definitive vertical runs (bars, stems) beforehand
                # in order to avoid cross shaped noise
                notallruns = self.image.image_copy()
                notallruns.filter_tall_runs(2 * self.staffspace_height \
                                            + self.staffline_height, 'black')
                
                # Copy every chord found by the
                # algorithm to the rescue image
                rescue_image.rescue_stafflines_using_secondchord( \
                    notallruns, \
                    self.stafffinder.get_skeleton(), \
                    self.staffline_height, \
                    self.staffspace_height, \
                    threshold, \
                    max_gap_height, \
                    int(self.staffline_height * 1.5), \
                    direction=0) #, \
                    #debug, \
                    #dy, \
                    #dx)

                # recopy tall vertical runs into rescue image
                tallruns = self.image.image_copy()
                tallruns.filter_short_runs(1 + 2 * self.staffspace_height \
                                            + self.staffline_height, 'black')
                rescue_image.or_image(tallruns,in_place=True)

            elif symbol_criterion == 'mask':

                rescue_image = Image(self.image);

                if debug >= 1:
                    print "   Rescue operation using T-shaped mask..."

                # Copy every line slice found by the mask
                # algorithm to the rescue image
                rescue_image.rescue_stafflines_using_mask( \
                    self.image, \
                    self.stafffinder.get_skeleton(), \
                    self.staffline_height, \
                    threshold)

            elif symbol_criterion == 'skewedruns':

                if debug >= 1:
                    print "   Rescue operation using keep_tall_skewed_runs..."

                # build list of center points for skewed runs:
                # staffskeleton plus and minus staffline_height
                centerpoints_up = []
                centerpoints_down = []
                if (1.0*self.staffspace_height)/self.staffline_height < 2.5:
                    dist = int(1.5*self.staffline_height + 1)
                    checkheight = int(1.5*self.staffline_height + 1)
                else:
                    dist = 2*self.staffline_height + 1
                    checkheight = int(2*self.staffline_height + 1)
                for staff in self.stafffinder.get_skeleton():
                    for skel in staff:
                        x = skel.left_x
                        for y in skel.y_list:
                            centerpoints_down.append(Point(x,y+dist))
                            centerpoints_up.append(Point(x,y-dist))
                            x += 1
                # remove definitive vertical runs (bars, stems) beforehand
                # in order to avoid cross shaped noise
                notallruns = self.image.image_copy()
                notallruns.filter_tall_runs(2 * self.staffspace_height \
                                            + self.staffline_height, 'black')
                rescue_image = self.image.image_copy()
                rescue_image.filter_short_runs(threshold, 'black')
                rescue_image2 = notallruns.keep_tall_skewed_runs(-60.0,60.0,checkheight,centerpoints_up,'down')
                rescue_image.or_image(rescue_image2,in_place=True)
                rescue_image2 = notallruns.keep_tall_skewed_runs(-60.0,60.0,checkheight,centerpoints_down,'up')
                rescue_image.or_image(rescue_image2,in_place=True)

            elif symbol_criterion == 'vectorfieldruns':

                if debug >= 1:
                    print "   Rescue operation using runs in vector field direction..."

                # build list of center points for vector field runs:
                # staffskeleton plus and minus staffline_height
                centerpoints_up = []
                centerpoints_down = []
                if (1.0*self.staffspace_height)/self.staffline_height < 2.5:
                    dist = int(1.5*self.staffline_height + 1)
                    checkheight = int(2*self.staffline_height + 1)
                else:
                    dist = 2*self.staffline_height + 1
                    checkheight = int(2.5*self.staffline_height + 1)
                for staff in self.stafffinder.get_skeleton():
                    for skel in staff:
                        x = skel.left_x
                        for y in skel.y_list:
                            centerpoints_down.append(Point(x,y+dist))
                            centerpoints_up.append(Point(x,y-dist))
                            x += 1

                # keep long vertical runlengths plus runs in vector field direction
                rescue_image = self.image.image_copy()
                rescue_image.filter_short_runs(threshold, 'black')
                rescue_image2 = self.image.keep_vectorfield_runs(centerpoints_up, \
                                                                checkheight, 'down', \
                                                                2*self.staffspace_height, 30)
                rescue_image.or_image(rescue_image2,in_place=True)
                rescue_image2 = self.image.keep_vectorfield_runs(centerpoints_down, \
                                                                 checkheight, 'up', \
                                                                 2*self.staffspace_height, 30)
                rescue_image.or_image(rescue_image2,in_place=True)


            else:
                raise RuntimeError, \
                      "Invalid value '%s' for symbol_criterion" \
                      % symbol_criterion


        elif crossing_symbols == 'bars':

            if debug >= 1:
                print "   Rescueing long vertical black runs..."

            # Save only bar lines and flag stems
            rescue_image = self.image.image_copy()
            rescue_image.filter_short_runs(self.staffspace_height + 1, 'black')

        elif crossing_symbols == 'none':

            if debug >= 1:
                print "   No rescue operation."

            rescue_image = Image(self.image,ONEBIT)

        else:
            raise RuntimeError, \
                  "Invalid value '%s' for crossing_symbols" \
                  % crossing_symbols

        if debug >= 2:
            print "Creating debug images..."
            skeleton_in_image = Image(self.image)
            skeleton_out_image = Image(self.image)

        if debug >= 1:
            print "Removing staff lines around skeletons..."

        # Remove around staff lines
        if crossing_symbols == 'all':
            removethreshold = threshold
        else:
            removethreshold = int(self.staffline_height * 1.3) + 1
        self.image.remove_line_around_skeletons( \
            self.stafffinder.get_skeleton(), \
            self.staffline_height, \
            removethreshold, \
            max_gap_height)

        if debug >= 2:

            # Mark skeletons in debug image
            for staff in self.stafffinder.get_skeleton():
                for skel in staff:
                    for i in range(len(skel.y_list)):
                        row = skel.y_list[i]
                        col = skel.left_x + i
                        if orig_image.get((col, row)) == 1:
                            skeleton_in_image.set((col, row), 1)
                        else:
                            skeleton_out_image.set((col, row), 1)

        if debug >= 1:
            print "   Done."
            print "Combining image with rescued parts..."

        self.image = self.image.or_image(rescue_image)

        if debug >= 2:
            print "Subtracting images..."
            removed_image = orig_image.subtract_images(self.image)

            print "Drawing debug image..."
            debug_image = Image(self.image, RGB)
            debug_image.highlight(orig_image, RGBPixel(100, 100, 100))
            debug_image.highlight(removed_image, RGBPixel(255, 0, 0))
            debug_image.highlight(rescue_image, RGBPixel(0, 255, 0))
            debug_image.highlight(skeleton_in_image, RGBPixel(0, 0, 255))
            debug_image.highlight(skeleton_out_image, RGBPixel(0, 255, 255))

            print "Saving debug image..."
            debug_image.save_PNG('musicstaves_linetracking_debug.png')

        if debug >= 1:
            print "Done."

    #------------------------------------------------------------

    def get_staffpos(self, x = 0):
        """Returns the y-positions of all staff lines at a given x-position.
Can only be called *after* ``remove_staves``.

Signature:

  ``get_staffpos(x=0)``

The value for *x* is currently ignored and the average y-positions are
always returned. The return value is a list of `StaffObj`__.

.. __: gamera.toolkits.musicstaves.musicstaves.StaffObj.html#staffobj
"""

        avglist = self.stafffinder.get_average()
        if not self.__staffobjlist and avglist:
            self.__staffobjlist = []
            for i,staff in enumerate(avglist):
                if len(staff) > 0:
                    so = StaffObj()
                    so.staffno = i+1
                    so.yposlist=[avg.average_y for avg in staff]
                    so.staffrect = Rect(Point(so.yposlist[0],staff[0].left_x),\
                                     Point(so.yposlist[-1],staff[-1].right_x))
                    self.__staffobjlist.append(so)

        return self.__staffobjlist


#----------------------------------------------------------------
