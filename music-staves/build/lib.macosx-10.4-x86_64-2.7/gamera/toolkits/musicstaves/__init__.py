#!/usr/bin/python
#
# toom, 2004-11-18
# chris, 2005-2008
#
"""
MusicStaves toolkit (setup)

This file includes all plugins needed for the MusicStaves toolkit.
Furthermore it creates both the icon of the toolkit and its menu (using
the right mouse button on the icon).

Additional MusicStaves_XXX and StaffFinder_XXX classes, derived from the
MusicStaves and StaffFinder class, get their menu entry after adding the
name to *self.classes* (see __init__).
"""

import re
from os import path

from gamera import toolkit
from gamera.args import *
from gamera.core import *
import plugins
from musicstaves_rl_simple import MusicStaves_rl_simple
from musicstaves_rl_fujinaga import MusicStaves_rl_fujinaga
from musicstaves_rl_roach_tatem import MusicStaves_rl_roach_tatem
from musicstaves_rl_carter import MusicStaves_rl_carter
from musicstaves_linetracking import MusicStaves_linetracking
from musicstaves_skeleton import MusicStaves_skeleton
from stafffinder_miyao import StaffFinder_miyao
from stafffinder_gabriel import StaffFinder_gabriel
from stafffinder_projections import StaffFinder_projections
from stafffinder_dalitz import StaffFinder_dalitz

from gamera.gui import has_gui
if has_gui.has_gui:
	from gamera.gui import var_name
	import wx
	import musicstaves_module_icon

	class MusicStavesModuleIcon(toolkit.CustomIcon):

		def __init__(self, *args, **kwargs):
			toolkit.CustomIcon.__init__(self, *args, **kwargs)

			#
			# list containing all classes derived from
			# MusicStaves, add your own class name to this list
			# and it will appear in the menu of the MusicStaves
			# icon
			#
			self.classes = ["MusicStaves_linetracking",
					"MusicStaves_rl_carter",
					"MusicStaves_rl_fujinaga",
					"MusicStaves_rl_roach_tatem",
					"MusicStaves_rl_simple",
					"MusicStaves_skeleton",
					"StaffFinder_dalitz",
					"StaffFinder_miyao",
					"StaffFinder_gabriel",	#GVM
					"StaffFinder_projections"]
			# menu id's for creating classes over popup menu
			self._menuids = []
			for c in self.classes:
				self._menuids.append(wx.NewId())

		def get_icon():
			return toolkit.CustomIcon.to_icon(\
					musicstaves_module_icon.getBitmap())
		get_icon = staticmethod(get_icon)

		def check(data):
			import inspect
			return inspect.ismodule(data) and\
					data.__name__.endswith("musicstaves")
		check = staticmethod(check)

		def right_click(self, parent, event, shell):
			self._shell=shell
			x, y=event.GetPoint()
			menu=wx.Menu()

			# create the menu entry for each class listed in
			# 'classes' (they all point to the same method but
			# can be distinguished by their menu index)
			for index, entry in enumerate(self.classes):
				menu.Append(self._menuids[index],
					    "Create a %s object" % entry)
				wx.EVT_MENU(parent, self._menuids[index],\
						self.createMusicStavesObj)
			parent.PopupMenu(menu, wx.Point(x, y))

		def double_click(self):
			pass

		#
		# creates an instance of a MusicStaves_XXX class
		#
		def createMusicStavesObj(self, event):
			# find class belonging to menu entry
			index = -1
			for i, m in enumerate(self._menuids):
				if m == event.GetId():
					index = i
					break
			if index < 0:
				return
			ms_module=self.classes[index]

			# ask for parameters
			dialog=Args([FileOpen("Image file", "", "*.*"),\
					Int("Staffline height"),\
					Int("Staffspace height")],\
					"Create a %s object" % ms_module)
			params=dialog.show()

			if params != None:
				if params[0] != None:
					#
					# load the image here and load it
					# into the gamera shell, too. this is
					# done because for checking whether
					# it is a onebit image or not.
					#
					filename=params[0]
					imagename = path.basename(params[0])
					imagename=imagename.split('.')[0]
					# substitute special characters
					imagename=re.sub('[^a-zA-Z0-9]', '_',\
							imagename)
					imagename=re.sub('^[0-9]', '_',\
							imagename)
					test = r"test\t"
					test2 = "test\t"
					image=load_image(filename)
					#self._shell.run(imagename + ' = load_image(r"' + filename + '") ') 
					self._shell.run('%s = load_image(r"%s")'\
							% (imagename, filename))
					if image.data.pixel_type != ONEBIT:
						self._shell.run("%s = %s.to_onebit()"\
								% (imagename,\
								imagename))

					# still exists in the gamera shell
					del image

					# choose a name for the variable in
					# the GUI
					if ms_module.startswith("StaffFinder"):
						name=var_name.get("stafffinder",\
							self._shell.locals)
					else:
						name=var_name.get("musicstaves",\
							self._shell.locals)

					if name is "" or name is None:
						return

					# create an instance of the specified
					# MusicStaves_XXX class
					self._shell.run("%s = %s.%s(%s, %d, %d)"\
							% (name,\
							self.label,\
							ms_module,\
							imagename,\
							params[1], params[2]))



	MusicStavesModuleIcon.register()
