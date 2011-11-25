/*
 *
 * Copyright (C) 2000-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef mgd01042005_staffremovalfujinagapython
#define mgd01042005_staffremovalfujinagapython

// This is just glue code to interface to/from Python.  The real
// meat of the algorithm is in "staff_removal_fujinaga.hpp"

#include "staff_removal_fujinaga.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////
// CONVERSION FUNCTIONS

PyObject* staff_to_python(Aomr::Staff& staff, size_t staff_no) {
  static PyObject* staff_class = NULL;
  if (staff_class == NULL) {
    PyObject* class_dict = PyDict_New();
    PyObject* class_name = PyString_FromString("StaffObj");
    staff_class = PyClass_New(NULL, class_dict, class_name);
  }
  
  PyObject* staff_dict = PyDict_New();

  PyObject* staff_no_py = PyInt_FromLong(long(staff_no));
  PyDict_SetItemString(staff_dict, "staffno", staff_no_py);
  Py_DECREF(staff_no_py);
  
  PyObject* staffline_list = PyList_New(staff.n_stafflines());
  for (size_t i = 0; i < staff.n_stafflines(); ++i)
    PyList_SetItem(staffline_list, i, PyInt_FromLong(staff.staffline_pos[i]));
  PyDict_SetItemString(staff_dict, "yposlist", staffline_list);
  Py_DECREF(staffline_list);

  PyObject* staff_rect = create_RectObject(Rect(staff));
  PyDict_SetItemString(staff_dict, "staffrect", staff_rect);
  Py_DECREF(staff_rect);

  PyObject* staff_instance = (PyObject*)PyInstance_NewRaw(staff_class, staff_dict);
  Py_DECREF(staff_dict);

  return staff_instance;
}

PyObject* page_to_python(Aomr::Page& page) {
  PyObject* staves = PyList_New(page.staves.size());
  for (size_t i = 0; i < page.staves.size(); ++i) {
    PyObject* staff_instance = staff_to_python(page.staves[i], i);
    PyList_SetItem(staves, i, staff_instance);
  }
  
  return staves;
}

Aomr::Staff staff_from_python(Rect& original, PyObject* py_staff) {
  Aomr::Staff staff;
  
  if (!PyInstance_Check(py_staff))
    throw std::runtime_error("Invalid StaffObj");

  PyObject* staffline_list = PyObject_GetAttrString(py_staff, "yposlist");
  if (!PyList_Check(staffline_list))
    throw std::runtime_error("Invalid StaffObj");
  int size = PyList_Size(staffline_list);
  staff.staffline_pos.resize(size);
  for (int i = 0; i < size; ++i) {
    PyObject* value = PyList_GetItem(staffline_list, i);
    if (!PyInt_Check(value))
      throw std::runtime_error("Invalid StaffObj");
    staff.staffline_pos[i] = int(PyInt_AsLong(value));
  }
  Py_DECREF(staffline_list);

  if (PyObject_HasAttrString(py_staff, "staffrect")) {
    PyObject* staff_rect = PyObject_GetAttrString(py_staff, "staffrect");
    if (!is_RectObject(staff_rect))
      throw std::runtime_error("Invalid StaffObj");
    Rect* rect = (((RectObject*)staff_rect)->m_x);
    staff.ul_x(rect->ul_x());
    staff.ul_y(rect->ul_y());
    staff.lr_x(rect->lr_x());
    staff.lr_y(rect->lr_y());
    Py_DECREF(staff_rect);
  } else {
    // If we don't have a rectangle lying around in the StaffObj, as per the version 0.5 definition of
    // StaffObj, just create one extending across the entire image, and holding all of the stafflines.
    staff.ul_x(original.ul_x());
    staff.lr_x(original.lr_x());
    staff.ul_y(staff.staffline_pos[0]);
    staff.lr_y(staff.staffline_pos[size - 1]);
  }
  return staff;
}

Aomr::Page page_from_python(Rect& original, PyObject* py_page) {
  if (!PyList_Check(py_page))
    throw std::runtime_error("Invalid list of StaffObjs");
  
  Aomr::Page page;
  int size = PyList_Size(py_page);
  for (int i = 0; i < size; ++i) {
    Aomr::Staff staff = staff_from_python(original, PyList_GetItem(py_page, i));
    page.add_staff(staff);
  }

  return page;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// TOP-LEVEL FUNCTIONS

template<class T>
Aomr::view_type* global_staffline_deskew(T& original, double staffline_h = 0.0, 
					 double staffspace_h = 0.0, size_t skew_strip_width = 0, 
					 double max_skew = 8.0) {
  Aomr::Param param(0, 0, staffline_h, staffspace_h, skew_strip_width, max_skew, false);
  return Aomr::global_staffline_deskew(original, param);
}

template<class T>
PyObject* find_and_remove_staves_fujinaga(T& original, size_t crossing_symbols, size_t n_stafflines,
					  double staffline_h, double staffspace_h, 
					  size_t skew_strip_width, double max_skew,
					  bool deskew_only, bool find_only,
                                          bool undo_deskew) {

  Aomr::Param param(crossing_symbols, n_stafflines, staffline_h, staffspace_h, skew_strip_width, 
		    max_skew, deskew_only, find_only, undo_deskew);
  Aomr::Page page;

  std::pair<Aomr::view_type*, Aomr::view_type*> result_images = Aomr::find_and_remove_staves_fujinaga
    (original, param, page);

  PyObject* py_page = page_to_python(page);
  PyObject* result;
  if (find_only) {
    result = py_page;
  } else if (deskew_only) {
    PyObject* py_deskewed_image = create_ImageObject(result_images.first);
    result = Py_BuildValue("OO", py_deskewed_image, py_page);
    Py_DECREF(py_deskewed_image);
    Py_DECREF(py_page);
  } else {
    PyObject* py_deskewed_image = create_ImageObject(result_images.first);
    PyObject* py_removed_image = create_ImageObject(result_images.second);
    result = Py_BuildValue("OOO", py_deskewed_image, py_removed_image, py_page);
    Py_DECREF(py_deskewed_image);
    Py_DECREF(py_removed_image);
    Py_DECREF(py_page);
  }

  return result;
}

template<class T>
Aomr::view_type* remove_staves_fujinaga(T& original, PyObject* py_page, size_t crossing_symbols,
				 double staffline_h, double staffspace_h) {
  Aomr::Param param;
  param.crossing_symbols = crossing_symbols;
  param.staffline_h = staffline_h;
  param.staffspace_h = staffspace_h;

  Aomr::Page page = page_from_python(original, py_page);

  Aomr::view_type* result_image = Aomr::remove_staves_fujinaga(original, page, param);

  return result_image;
}

#endif
