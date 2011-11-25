# -*- mode: python; indent-tabs-mode: nil; tab-width: 4 -*-
# vim: set tabstop=4 shiftwidth=4 expandtab:

#
# Copyright (C) 2006 Christoph Dalitz, Michael Droettboom
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

from sets import Set


class EquivalenceGrouper(object):
    """This class provides an easy way to parttion a set into *equivalence
classes*, i.e. disjoint subsets where the members in each subset are
equivalent.

Objects can be joined using ``.join()``, tested for equivalence using
``.joined()``, and all disjoint sets can be retrieved using an iterator
over the class.

The objects being joined must be hashable.

.. code:: Python

   from equivalence_grouper import EquivalenceGrouper

   g = EquivalenceGrouper()
   
   # mark items as equivalent
   g.join('a', 'b')
   g.join('b', 'c')
   g.join('d', 'e')
   
   # all classes can be accessed by iterating over g
   [c for c in g]
   >>> [['a', 'b', 'c'], ['d', 'e']]

   # check for equivalence
   g.joined('a', 'b')
   >>> True
   # transitivity is automatically detected
   g.joined('a', 'c')
   >>> True
   g.joined('a', 'd')
   >>> False

:Author: Michael Droettboom and Christoph Dalitz
"""
    def __init__(self, init=[]):
        """Optionally you can provide a list of items to the constructor which
are first considered not to be equivalent. You can use ``.join()`` afterwards
to mark items as equivalent and/or to add addiitonal items.
"""
        mapping = self._mapping = {}
        for x in init:
            mapping[x] = [x]

    def join(self, a, *args):
        """Join given arguments into the same subset, i.e. mark them as
equivalent.

Items not yet part of any subset are added in a new subset.
Accepts one or more arguments. To just add a new item ``x`` to the superset,
just call ``.join(x)``.
"""
        mapping = self._mapping
        set_a = mapping.setdefault(a, [a])

        for arg in args:
            set_b = mapping.get(arg)
            if set_b is None:
                set_a.append(arg)
                mapping[arg] = set_a
            elif set_b is not set_a:
                if len(set_b) > len(set_a):
                    set_a, set_b = set_b, set_a
                set_a.extend(set_b)
                for elem in set_b:
                    mapping[elem] = set_a

    def joined(self, a, b):
        """Returns True if a and b are equivalent, i.e. are members of the
same subset (equivalence class)."""
        mapping = self._mapping
        try:
            return mapping[a] is mapping[b]
        except KeyError:
            return False

    def __iter__(self):
        """Returns an iterator returning each of the disjoint subsets as a
list."""
        seen = Set()
        for elem, group in self._mapping.iteritems():
            if elem not in seen:
                yield group
                seen.update(group)

