# -*- mode: python; indent-tabs-mode: nil; tab-width: 4 -*-
# vim: set tabstop=4 shiftwidth=4 expandtab:

#
# Copyright (C) 2006 Christoph Dalitz
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

#----------------------------------------------------------------

from gamera.plugin import *
import _evaluation


#----------------------------------------------------------------

class pixel_error(PluginFunction):
    """Returns the rate of misclassified pixels (*staff*, *non staff*)
during staff removal.

Arguments:

  *self*:
    The full original input image

  *Gstaves*:
    ground truth image containing only the staff segments that should
    be removed

  *Sstaves*:
    image containing the actually removed staff segments

The return value is a tuple (*error, e1, e2, a)* with the following
meaning:

+---------+--------------------------------+
| value   | meaning                        |
+=========+================================+
| *error* | actual error rate              |
+---------+--------------------------------+
| *e1*    | black area of missed staff     |
+---------+--------------------------------+
| *e2*    | count of false positive pixels |
+---------+--------------------------------+
| *a*     | count of black pixels          |
+---------+--------------------------------+
"""
    pure_python = 1
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([ONEBIT], 'Gstaves'), \
                 ImageType([ONEBIT], 'Sstaves')])
    return_type = Class("list", list)
    author = "Christoph Dalitz"

    def __call__(self, Gstaves, Sstaves):
        missedstaff = Gstaves.subtract_images(Sstaves)
        falsepositive = Sstaves.subtract_images(Gstaves)
        # note that black_area() returns array with one element
        e1 = missedstaff.black_area()
        e2 = falsepositive.black_area()
        a = self.black_area()
        return [float(e1[0] + e2[0]) / a[0], e1[0], e2[0], a[0]]

    __call__ = staticmethod(__call__)

#----------------------------------------------------------------

class segment_error(PluginFunction):
    """Returns the rate of misclassified staff segments 
during staff removal.

Compares two segmentations by building equivalence classes of
overlapping segments as described in the reference below. Each
class is assigned an error type depending on how many ground truth
and test segments it contains. The return value is a tuple
(*n1,n2,n3,n4,n5,n6)* where each value is the total number of classes
with the corresponding error type:

+------+-----------------------+---------------+----------------------+
| Nr   | Ground truth segments | Test segments | Error type           |
+======+=======================+===============+======================+
| *n1* | 1                     | 1             | correct              |
+------+-----------------------+---------------+----------------------+
| *n2* | 1                     | 0             | missed segment       |
+------+-----------------------+---------------+----------------------+
| *n3* | 0                     | 1             | false positive       |
+------+-----------------------+---------------+----------------------+
| *n4* | 1                     | > 1           | split                |
+------+-----------------------+---------------+----------------------+
| *n5* | > 1                   | 1             | merge                |
+------+-----------------------+---------------+----------------------+
| *n6* | > 1                   | > 1           | splits and merges    |
+------+-----------------------+---------------+----------------------+

Input arguments:

  *Gstaves*:
    ground truth image containing only the staff segments that should
    be removed

  *Sstaves*:
    image containing the actually removed staff segments

References:

  M. Thulke, V. Margner, A. Dengel:
  *A general approach to quality evaluation of document
  segmentation results.*
  Lecture Notes in Computer Science 1655, pp. 43-57 (1999)
"""
    self_type = None
    args = Args([ImageType([ONEBIT], 'Gstaves'), \
                 ImageType([ONEBIT], 'Sstaves')])
    return_type = IntVector("errornumbers", length=6)
    author = "Christoph Dalitz"

#----------------------------------------------------------------

class interruption_error(PluginFunction):
    """Returns error information for staff line interruptions.

Compares the staff-only image of the "ground truth" image *Gstaves*
with the staff-only image of a staff removal result *Sstaves* (image
that contains exactly the removed pixels). A list of
StafflineSkeleton-objects *Skeletons* is also needed to find the
stafflines in the images. This algorithm recognizes interruptions in
the stafflines, and tries to link them by matching their x-positions.
After that it removes links, so that every interruption is linked at
most once. The return value is a tuple (*n1,n2,n3)* describing the
results:

+------+--------------------------------------------------------------+
|      | Meaning                                                      |
+======+==============================================================+
| *n1* | interruptions without link (either Gstaves or Sstaves)       |
+------+--------------------------------------------------------------+
| *n2* | removed links because of doubly linked interruptions         |
+------+--------------------------------------------------------------+
| *n3* | interruptions in Gstaves                                     |
+------+--------------------------------------------------------------+

In the following image, you can see 2 interruptions without link (blue),
2 removed links (red) and 7 interruptions in Gstaves, so the return value
would be (2,2,7):

.. image:: images/staffinterrupt.png
"""
    self_type = None
    args = Args([ImageType([ONEBIT], 'Gstaves'), \
                 ImageType([ONEBIT], 'Sstaves'), \
                 Class('Skeletons',object,True)])
    return_type = IntVector("errornumbers", length=3)
    author = "Bastian Czerwinski"

#----------------------------------------------------------------

class EvaluationModule(PluginModule):
    cpp_headers = ["evaluation.hpp"]
    category = "MusicStaves/Evaluation"
    functions = [pixel_error, segment_error, interruption_error]
    author = "Christoph Dalitz"

module = EvaluationModule()

#----------------------------------------------------------------

# instantiation of free plugins

segment_error = segment_error()
interruption_error = interruption_error()
