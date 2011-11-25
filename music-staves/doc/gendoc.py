#!/usr/bin/env python

from gamera import gendoc


if __name__ == '__main__':

   # Step 1:
   # Import all of the plugins to document.
   # Be careful not to load the core plugins, or they
   # will be documented here, too.
   # If the plugins are not already installed, we'll just ignore
   # them and generate the narrative documentation.
   try:
      #from gamera.toolkits.musicstaves import musicstaves
      #from gamera.toolkits.musicstaves import musicstaves_rl_simple
      #from gamera.toolkits.musicstaves.plugins import staff_removal_fujinaga
      #from gamera.toolkits.musicstaves.plugins import roach_tatem_plugins
      from gamera.toolkits.musicstaves.plugins import *
   except ImportError:
      print "WARNING:"
      print "This `musicstaves` toolkit must be installed before generating"
      print "the documentation.  For now, the system will skip generating"
      print "documentation for the plugins."
      print
   else:
      # Step 2:
      # Generate documentation for this toolkit
      # This will handle any commandline arguments if necessary
      # classes/methods defined in this toolkit
      gendoc.gendoc(classes=[("gamera.toolkits.musicstaves.musicstaves",
                              "StaffObj",
                              "__init__"),
                             ("gamera.toolkits.musicstaves.musicstaves",
                              "MusicStaves",
                              "__init__ remove_staves get_staffpos"),
                             ("gamera.toolkits.musicstaves.musicstaves_rl_simple",
                              "MusicStaves_rl_simple",
                              "remove_staves get_staffpos"),
                             ("gamera.toolkits.musicstaves.musicstaves_rl_fujinaga",
                              "MusicStaves_rl_fujinaga",
                              "remove_staves get_staffpos"),
                             ("gamera.toolkits.musicstaves.musicstaves_rl_roach_tatem",
                              "MusicStaves_rl_roach_tatem",
                              "remove_staves get_staffpos"),
                             ("gamera.toolkits.musicstaves.musicstaves_rl_carter",
                              "MusicStaves_rl_carter",
                              "remove_staves get_staffpos"),
                             ("gamera.toolkits.musicstaves.musicstaves_linetracking",
                              "MusicStaves_linetracking",
                              "remove_staves get_staffpos"),
                             ("gamera.toolkits.musicstaves.musicstaves_skeleton",
                              "MusicStaves_skeleton",
                              "__init__ remove_staves get_staffpos"),
                             ("gamera.toolkits.musicstaves.stafffinder",
                              "StafflineAverage",
                              "__init__ to_average to_polygon to_skeleton"),
                             ("gamera.toolkits.musicstaves.stafffinder",
                              "StafflinePolygon",
                              "__init__ to_average to_polygon to_skeleton"),
                             ("gamera.toolkits.musicstaves.stafffinder",
                              "StafflineSkeleton",
                              "__init__ to_average to_polygon to_skeleton"),
                             ("gamera.toolkits.musicstaves.stafffinder",
                              "StaffFinder",
                              "__init__ find_staves show_result get_average get_polygon get_skeleton"),
                             ("gamera.toolkits.musicstaves.stafffinder_miyao",
                              "StaffFinder_miyao",
                              "find_staves"),
                             ("gamera.toolkits.musicstaves.stafffinder_projections",
                              "StaffFinder_projections",
                              "find_staves"),
                             ("gamera.toolkits.musicstaves.stafffinder_dalitz",
                              "StaffFinder_dalitz",
                              "find_staves"),
                             ("gamera.toolkits.musicstaves.plugins.skeleton_utilities",
                              "SkeletonSegment",
                              "__init__"),
                             ("gamera.toolkits.musicstaves.equivalence_grouper",
                              "EquivalenceGrouper",
                              "__init__ join joined __iter__")
                             ],
                    plugins=["MusicStaves"])

