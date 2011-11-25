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

import exceptions

from gamera import toolkit
from gamera.core import ONEBIT, GREYSCALE, GREY16, RGB
from gamera.args import *
from gamera.gui import has_gui
if has_gui.has_gui:
	from gamera.gui import var_name
	import wx


class StaffObj:
	"""Contains the following positional information of a staff system:

  *staffno*:
    staff number, from top to bottom, starting with one
  *yposlist*:
    list of y-positions of the stafflines
  *staffrect*:
    rectangle (Gamera type ``Rect``) containing this staff system

A list of ``StaffObj`` is the return value of `MusicStaves.get_staffpos`__.

.. __: gamera.toolkits.musicstaves.musicstaves.MusicStaves.html#get-staffpos
"""
	def __init__(self):
		"""The constructor has no arguments. All values are accessed
directly:

.. code:: Python

   so = StaffObj()
   so.staffno = current_staffno
   so.yposlist = ['12','20','29','36']
   ymin = yposlist[0]; ymax = yposlist[-1]
   so.staffrect = Rect((xmin, ymin), Dim(staffwidth, ymax - ymin))
"""
		self.staffno=0;
		self.yposlist=[];
		self.staffrect=Rect()

##############################################################################
#
#
class MusicStaves:
	"""Virtual class providing a unified interface to staff line removal
from a music/tablature image and for querying the staff positions afterwards.
For typical usage see the `user's manual`__.

.. __: usermanual.html

.. note:: This class cannot be used itself. You must use one of the
   derived classes which actually implement staff removal algorithms.

:Author: Christoph Dalitz and Thomas Karsten
"""
	class MusicStavesIcon(toolkit.CustomIcon):
		def get_icon():
			import musicstaves_icon as ms_icon
			return toolkit.CustomIcon.to_icon(ms_icon.getBitmap())
		get_icon = staticmethod(get_icon)
	
		def check(data):
			return isinstance(data, MusicStaves)

		check = staticmethod(check)
	
		def right_click(self, parent, event, shell):
			self._shell=shell
			x, y=event.GetPoint()
			menu=wx.Menu()

			# create the 'Info' menu point and insert it to
			# the "right click menu"
			info_menu=wx.Menu()
			menu.AppendMenu(wx.NewId(), "Info", info_menu)

			info_menu.Append(wx.NewId(), "Class instance: %s"\
					% str(self.data.__class__).\
					split(".")[-1])

			#
			# display the detected staff line positions
			#
			try:
				staves=self.data.get_staffpos()
			except:
				staves=[]

			if staves:
				ypos_menu=wx.Menu()
				info_menu.AppendMenu(wx.NewId(),"Detected staff lines",ypos_menu)
				y_menu=[]
				for i, staff in enumerate(staves):
					y_menu.append(wx.Menu())
					item="staff %d (%d lines)"\
							% (i+1,\
							len(staff.yposlist))
					ypos_menu.AppendMenu(wx.NewId(), item,\
							y_menu[i])

					# XXX: a for-loop will move the menu
					# to undefined positions (should be
					# investigated...)
					j=0
					while (j < len(staff.yposlist)):
						y_menu[i].Append(wx.NewId(),\
								"%d"\
								% staff.\
								yposlist[j])
						j+=1

			info_menu.Append(wx.NewId(), "staffline_height: %d"\
					% self.data.staffline_height)
			info_menu.Append(wx.NewId(), "staffspace_height: %d"\
					% self.data.staffspace_height)

			# create the rest of the menu
			menu.AppendSeparator()
			index=wx.NewId()
			menu.Append(index, "Remove staves")
			wx.EVT_MENU(parent, index, self.call_remove_staves)
			index=wx.NewId()
			menu.Append(index, "Display image")
			wx.EVT_MENU(parent, index, self.call_display_image)
			index=wx.NewId()
			menu.Append(index, "Copy image")
			wx.EVT_MENU(parent, index, self.call_copy_image)
			parent.PopupMenu(menu, wx.Point(x, y))
	
		def double_click(self):
			return "%s.image.display()" % self.label

		def call_remove_staves(self, event):
			dialog=self.data.remove_staves_args
			command=dialog.show(function = self.label + \
					    ".remove_staves")
			if command:
				self._shell.run(command)

		def call_display_image(self, event):
			self._shell.run("%s.image.display()" % self.label)

		def call_copy_image(self, event):
			img_name=var_name.get(self.label + "_image",\
					self._shell.locals)

			if img_name is not None:
				self._shell.run("%s = %s.image.image_copy()"\
						% (img_name, self.label))

	######################################################################
	# __init__
	#
	# definition of variables
	#
	def __init__(self, img, staffline_height=0, staffspace_height=0):
		"""Constructor of MusicStaves. This base constructor should be
called by all derived classes.

Signature:

  ``__init__(image, staffline_height=0, staffspace_height=0)``

with

  *image*:
    Onebit or greyscale image of which the staves are to be removed.
    MusicStaves creates a copy of the given image, so that the original
    image is not altered. The staff line removal will be done on
    ``MusicStaves.image``

  *staffline_height*:
    average height of a single black staff line. When zero is given,
    MusicStaves computes it itself (recommended, unless you know exactly
    what you are doing).

  *staffspace_height*:
    average distance between adjacent lines of the same staff. When zero is
    given, MusicStaves computes it itself (recommended, unless you know exactly
    what you are doing).
"""
		if img.data.pixel_type == ONEBIT:
			self.image=img.image_copy()
		else:
			self.image=img.to_onebit()
		self.staffline_height=staffline_height
		self.staffspace_height=staffspace_height

		# height of a single staffline
		if 0==self.staffline_height:
			self.staffline_height=\
					self.image.most_frequent_run('black', 'vertical')

		# space between 2 stafflines in the same system
		if 0==self.staffspace_height:
			self.staffspace_height=\
					self.image.most_frequent_run('white', 'vertical')
		# arguments of remove_staves method for GUI dialog
		self.remove_staves_args = Args([ChoiceString("crossing_symbols",\
							     choices=['all','bars','none'],\
							     strict=False),\
						Int("num_lines", default=0)
						])

		if has_gui.has_gui:
			self.MusicStavesIcon.register()

	######################################################################
	# remove_staves
	#
	def remove_staves(self, crossing_symbols='all', num_lines=0):
		"""Detects and removes staff lines from a music/tablature image.

This method is virtual and must be implemented in every derived class.
Each implementation must at least support the following keyword arguments:

  *crossing_symbols*:
    Determines which symbols crossing staff lines the removal should try
    to keep intact. Possible values are 'all', 'bars' (only keeps long vertical
    lines uncut) or 'none' (does not care about crossing smyblos).

  *num_lines*:
    Number of lines within one staff. In general it can be hard to guess this
    number automatically; if the staff removal algorithm supports this, a value
    *num_lines=0* can be used to indicate that this number should be guessed
    automatically.

Additional algorithm specific keyword are possible of course. If
*crossing_symbols* cannot be chosen in the algorithm, this option must
be offered nevertheless (albeit with no effect).
"""
		raise RuntimeError, "Module MusicStaves (virtual class): Method not implemented."

	######################################################################
	# get_staffpos
	#
	def get_staffpos(self, x=0):
		"""Returns the y-positions of all staff lines at a given x-position.
Can only be called *after* ``remove_staves``.

This method is virtual and must be implemented in every derived class.
Each implementation must have the following signature:

  ``get_staffpos(x=0)``

with

  *x*:
    x-position at which the staffpositions are computed. For *x=0*,
    the average y-positions over the full staffwidth is returned.
    A value differnt from zero only makes sense, when the staff removal
    algorithm supports skewed or otherwise deformed staff lines. For most
    algorithms the value of *x* will have no effect on the output.

The return value is a list of `StaffObj`__.

.. __: gamera.toolkits.musicstaves.musicstaves.StaffObj.html#staffobj

blabla

.. note:: This interface for querying the staffposition is not not too
          suitable for OMR applications. Consequently a better interface
          (e.g. returning a stafffinder object or parts thereof) will
          be implemented in the future.

          Consequently this interface is deprecated and you are advised
          to directly copy the staff positions from the appropriate
          member variables described in the documentation of the
          derived classes in case you need the positions for further
          OMR processing.
"""

		raise RuntimeError, "Module MusicStaves (virtual class): Method not implemented."
