#
# Copyright (C) 2005 Christoph Dalitz, Florian Pose
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

from gamera.plugin import *
from math import pi
import _staff_finding_miyao

class miyao_candidate_points(PluginFunction):
        """Returns middle points of blackruns along *scanline_count*
equidistant vertical scan lines, that are *staffline_height* plusminus
*tolerance* pixels high.

When *staffline_height* is given as zero (default), it is automatically
computed as most frequent black vertical runlength.

Returns a list of *scanline_count* lists, each representing a vertical
scanline. The first value in each list is the *x* position, the subsequent
entries are the *y* positions of candidate points for staffline anchors.
"""
        self_type = ImageType([ONEBIT])
        args = Args([Int('scanline_count', range=(4, 100), default=35), \
                                 Int('tolerance', range=(0,100), default=2), \
                                 Int('staffline_height', range=(0, 100), default=0)])
        return_type = Class("candidate_list")
        author = "Florian Pose"

        def __call__(self, scanline_count=35, tolerance=2, staffline_height=0):
                if staffline_height == 0:
                        staffline_height = self.most_frequent_run('black', 'vertical')
                return _staff_finding_miyao.miyao_candidate_points(self, scanline_count, tolerance, staffline_height)

        __call__ = staticmethod(__call__)


class miyao_distance(PluginFunction):
        """Returns 0 when two points *a* and *b* ly on the same staff line
candidate and *penalty* if not (note that Miyao uses 2 for a mismatch instead).
The distance zero is only returned when the following two conditions hold:

- The line angle is *pr_angle* plusminus one degree. When *pr_angle*
  is greater than pi/2, the line angle must be within (-20,+20)
  degrees. *pr_angle* must be given in radian.

- the average blackness of the connection line is at least *blackness*
"""
        self_type = ImageType([ONEBIT])
        #args = Args([Class('a', Point), Class('b', Point),\
        args = Args([Int('xa'), Int('ya'), Int('xb'), Int('yb'),\
                                 Float('blackness', range=(0,1), default=0.8),\
                                 Float('pr_angle', range=(-pi/2,pi/2), default=pi),\
                                 Int('penalty', default=3)])
        return_type = Int("distance")
        author = "Christoph Dalitz"

        # only for default argument
        def __call__(self, xa, ya, xb, yb, blackness=0.8, pr_angle=pi, penalty=3):
            return _staff_finding_miyao.miyao_distance(self, xa, ya, xb, yb, blackness, pr_angle, penalty)
        __call__ = staticmethod(__call__)


class miyao_find_edge(PluginFunction):
        """Looks for a staff line edge point near (*x*,*y*).

The point (*x*,*y*) is extrapolated to the left (*direction* = True)
or right (*direction* = False) with angle *angle* (in radian) until the
pixel and its two vertical neighbors are white. When (*x*,*y*) itself
is white, the edge point is searched in the opposite direction as the
first black pixel among the three vertical neighbors.

Returns the new edge point as a vector [x,y]
"""
        self_type = ImageType([ONEBIT])
        args = Args([Int('x'), Int('y'), Float('angle'),\
                     Check('direction','left')])
        return_type = IntVector("edgepoint", length=2)
        author = "Christoph Dalitz"

class StaffFinder_miyao(PluginFunction):
    """Creates a StaffFinder_miyao__ object.

.. __: gamera.toolkits.musicstaves.stafffinder_miyao.StaffFinder_miyao.html

Leave the parameters *staffspace_height* and *staffline_height* zero
unless you have computed them somehow beforehand. When left zero,
they are computed automatically.
"""
    pure_python = 1
    category = "MusicStaves/Classes"
    self_type = ImageType([ONEBIT, GREYSCALE])
    args = Args([Int('staffline_height', default=0),\
                 Int('staffspace_height', default=0)])
    return_type = Class("stafffinder")

    def __call__(image, staffline_height=0, staffspace_height=0):
        from gamera.toolkits.musicstaves import stafffinder_miyao
        return stafffinder_miyao.StaffFinder_miyao(\
                image, staffline_height, staffspace_height)

    __call__ = staticmethod(__call__)
    author = "Christoph Dalitz"

class staff_finding_miyao_module(PluginModule):
        category = "MusicStaves/Miyao"
        cpp_headers = ["staff_finding_miyao.hpp"]
        functions = [miyao_candidate_points, miyao_distance, miyao_find_edge,
                     StaffFinder_miyao]
        author = "Christoph Dalitz, Florian Pose (after an algorithm by H. Miyao)"

module = staff_finding_miyao_module()

