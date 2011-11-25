# -*- mode: python; indent-tabs-mode: nil; tab-width: 4 -*-
# vim: set tabstop=4 shiftwidth=4 expandtab:

from gamera.plugin import *
import _skewed_runs

class keep_tall_skewed_runs(PluginFunction):
    """Removes all black pixels except those belonging to black runs taller
than a given height in a given angle range.

Arguments:

.. image:: images/skewed_tall_runs.png

*minangle*, *maxangle*:
  Angle range that is scanned for tall runs. Angles are measured in degrees
  and relative to the vertical axis; thus in the image above *minangle*
  is about -30 and *maxangle* about 45. When both angles are zero, the
  function behaves like ``filter_short_runs``.

*height*:
  When *points* is not given, only black runs taller than *2 * height* are kept.
  Otherwise the condition depends on the parameter *direction* (see below).

*points*:
  When given, only tall runs crossing these points are kept.
  Additionally they must run at least *height* in the given *direction*.
  For performance reasons, of each skewed run
  only the part up to *2 * height* around the given points is kept.
  When empty or omitted, all tall skewed runs are kept.

*direction*:
  When ``both``, skewed runs must run at least *height* both in the upper
  and lower vertical direction. When ``up`` or ``down``, the only need to
  run in one direction starting from the given points. Only has effect
  when additionally *poins* is given.
    """
    self_type = ImageType([ONEBIT])
    args = Args([Float('minangle', range=(-85,85)),\
                 Float('maxangle', range=(-85,85)),\
                 Int('height'), PointVector('points'),\
                 ChoiceString('direction',['both','up','down'])])
    return_type = ImageType([ONEBIT])
    author = "Thomas Karsten and Christoph Dalitz"

    # necessary because this is the only way to specify default arguments
    def __call__(self, minangle, maxangle, height, points=[], direction='both'):
        if direction not in ['both','up','down']:
            raise RuntimeError, "Invalid value for 'direction'"
        return _skewed_runs.keep_tall_skewed_runs(\
            self, minangle, maxangle, height, points, direction)
    __call__ = staticmethod(__call__)

class skewed_runs_module(PluginModule):
	category = "MusicStaves"
	cpp_headers = ["skewed_runs.hpp"]
	functions = [keep_tall_skewed_runs]
	author = "Thomas Karsten"

module = skewed_runs_module()
