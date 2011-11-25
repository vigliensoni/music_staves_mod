
        
    
  #include "gameramodule.hpp"
  #include "knnmodule.hpp"

        #include "staff_removal_fujinaga.hpp"
      #include "staff_removal_fujinaga_python.hpp"
  
    #include <string>
  #include <stdexcept>
  #include "Python.h"
  #include <list>

  using namespace Gamera;
  
        
      extern "C" {
#ifndef _MSC_VER
    void init_staff_removal_fujinaga(void);
#endif
                  static PyObject* call_find_and_remove_staves_fujinaga(PyObject* self, PyObject* args);
                        static PyObject* call_global_staffline_deskew(PyObject* self, PyObject* args);
                                            static PyObject* call_remove_staves_fujinaga(PyObject* self, PyObject* args);
                      }

          static PyMethodDef _staff_removal_fujinaga_methods[] = {
                  { CHAR_PTR_CAST "find_and_remove_staves_fujinaga",
          call_find_and_remove_staves_fujinaga, METH_VARARGS,
          CHAR_PTR_CAST "**find_and_remove_staves_fujinaga** (``Choice`` [all|bar|none] *crossing_symbols*, int(3, 12) *n_stafflines* = 5, float(0.00, 512.00) *staffline_height* = 0.00, float(0.00, 512.00) *staffspace_height* = 0.00, int(0, 512) *skew_strip_width* = 0, float(1.00, 20.00) *max_skew* = 8.00, ``bool`` *deskew_only* = False, ``bool`` *find_only* = False, ``bool`` *undo_deskew* = False)\n\nLocates staves while deskewing the image, and then removes the\nstaves.  Returns a tuple of the form (*deskewed_image*,\n*removed_image*, *staves*) where *deskewed_image* is the original\nimage deskewed, *removed_image* is the deskewed with the stafflines\nremoved, and *staves* is a list of ``StaffObj`` instances defining\nwhere the stafflines are.\n\n*crossing_symbols*\n   Not yet implemented -- just a placeholder for now.\n\n*n_stafflines* = 5\n   The number of stafflines in each staff.  (Autodetection of number\n   of stafflines not yet implemented).\n\n*staffline_height* = 0\n   The height (in pixels) of the stafflines. If *staffline_height* <=\n   0, the staffline height is autodetected both globally and for each\n   staff.\n\n*staffspace_height* = 0\n   The height (in pixels) of the spaces between the stafflines.  If\n   *staffspace_height* <= 0, the staffspace height is autodetected\n   both globally and for each staff.\n\n*skew_strip_width* = 0\n   The width (in pixels) of vertical strips used to deskew the image.\n   Smaller values will deskew the image with higher resolution, but\n   this may make the deskewing overly sensitive.  Larger values may\n   miss fine detail.  If *skew_strip_width* <= 0, the width will be\n   autodetermined to be the global *staffspace_h* * 2.\n\n*max_skew* = 8.0\n   The maximum amount of skew that will be detected within each\n   vertical strip.  Expressed in degrees.  This value should be fairly\n   small, because deskewing only approximates rotation at very small\n   degrees.\n\n*deskew_only* = ``False``\n   When ``True``, only perform staffline detection and deskewing.  Do\n   not remove the stafflines.\n\n*find_only* = ``False``\n   When ``True``, only perform staffline detection.  Do not deskew the\n   image or remove the stafflines.\n"        },
                        { CHAR_PTR_CAST "global_staffline_deskew",
          call_global_staffline_deskew, METH_VARARGS,
          CHAR_PTR_CAST "**global_staffline_deskew** (float *staffline_height* = 0.00, float *staffspace_height* = 0.00, int *skew_strip_width* = 0, float *max_skew* = 5.00)\n\nPerforms a global deskewing on the entire image by\ncross-correlating thin vertical strips of the image.  Since this\nfunction is global and does not deskew on staff at a time, the results\nare not great, and in particular long slurs can introduce errors.\nThis same deskewing also happens early on in the\nfind_and_remove_staves_fujinaga process.\n\nReturns deskewed copy of the image.\n\nIt is available here for primarily for experimentation rather than as\nanything ultimately useful.\n\n*staffline_height* = 0\n   The height (in pixels) of the stafflines. If *staffline_height* <=\n   0, the staffline height is autodetected both globally and for each\n   staff.\n\n*staffspace_height* = 0\n   The height (in pixels) of the spaces between the stafflines.  If\n   *staffspace_height* <= 0, the staffspace height is autodetected\n   both globally and for each staff.\n\n*skew_strip_width* = 0\n   The width (in pixels) of vertical strips used to deskew the image.\n   Smaller values will deskew the image with higher resolution, but\n   this may make the deskewing overly sensitive.  Larger values may\n   miss fine detail.  If *skew_strip_width* <= 0, the width will be\n   autodetermined to be the global *staffspace_h* * 2.\n\n*max_skew* = 8.0\n   The maximum amount of skew that will be detected within each\n   vertical strip.  Expressed in degrees.  This value should be fairly\n   small, because deskewing only approximates rotation at very small\n   degrees.\n"        },
                                            { CHAR_PTR_CAST "remove_staves_fujinaga",
          call_remove_staves_fujinaga, METH_VARARGS,
          CHAR_PTR_CAST "**remove_staves_fujinaga** (object *staves*, ``Choice`` [all|bars|none] *crossing_symbols*, float *staffline_height* = 0.00, float *staffspace_height* = 0.00)\n\nRemoves the staves from an image that has already been deskewed\nand had its staves detected by find_and_deskew_staves_fujinaga_.\nReturns the image with the staves removed.\n\n*staves*\n   A list of ``StaffObj`` instances as returned from\n   find_and_deskew_staves_fujinaga.  Other lists of StaffObj's may be\n   used, but if they do not have a ``staffrect`` member variable, each\n   staff will be treated as if it extends across the entire page from\n   left to right, and the margins of the image will not be treated\n   differently.\n\n*crossing_symbols* = 0\n   Not yet implemented -- just a placeholder for now.\n\n*staffline_height* = 0\n   The height (in pixels) of the stafflines. If *staffline_height* <=\n   0, the staffline height is autodetected both globally and for each\n   staff.\n\n*staffspace_height* = 0\n   The height (in pixels) of the spaces between the stafflines.  If\n   *staffspace_height* <= 0, the staffspace height is autodetected\n   both globally and for each staff.\n\nNote that the following are essentially (*) equivalent, (though the\nfirst is slightly faster):\n\n.. code:: Python\n\n   # All in one call\n   deskewed, removed, staves = source.find_and_remove_staves_fujinaga(...)\n\n   # Two separate calls, to try different staff removal methods\n   deskewed, staves = source.find_and_deskew_staves_fujinaga(...)\n   removed = deskewed.remove_staves_fujinaga(staves, ...)\n\n(*) There are some slight differences, but none that should affect the\nresult.\n"        },
                        { NULL }
  };

                static PyObject* call_find_and_remove_staves_fujinaga(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                                PyObject* return_arg;
PyObject* return_pyarg;
                                          Image* self_arg;
PyObject* self_pyarg;
                      int crossing_symbols_arg;
                      int n_stafflines_arg;
                      double staffline_height_arg;
                      double staffspace_height_arg;
                      int skew_strip_width_arg;
                      double max_skew_arg;
                      int deskew_only_arg;
                      int find_only_arg;
                      int undo_deskew_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "Oiiddidiii:find_and_remove_staves_fujinaga"
                        ,
             &self_pyarg                        ,
             &crossing_symbols_arg                        ,
             &n_stafflines_arg                        ,
             &staffline_height_arg                        ,
             &staffspace_height_arg                        ,
             &skew_strip_width_arg                        ,
             &max_skew_arg                        ,
             &deskew_only_arg                        ,
             &find_only_arg                        ,
             &undo_deskew_arg                      ) <= 0)
           return 0;
               
              if (!is_ImageObject(self_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'self' must be an image");
          return 0;
        }
        self_arg = ((Image*)((RectObject*)self_pyarg)->m_x);
        image_get_fv(self_pyarg, &self_arg->features, &self_arg->features_len);
                                                                                                                                            
              try {
                      switch(get_image_combination(self_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = find_and_remove_staves_fujinaga(*((OneBitImageView*)self_arg), crossing_symbols_arg, n_stafflines_arg, staffline_height_arg, staffspace_height_arg, skew_strip_width_arg, max_skew_arg, deskew_only_arg, find_only_arg, undo_deskew_arg);
break;
case CC:
return_arg = find_and_remove_staves_fujinaga(*((Cc*)self_arg), crossing_symbols_arg, n_stafflines_arg, staffline_height_arg, staffspace_height_arg, skew_strip_width_arg, max_skew_arg, deskew_only_arg, find_only_arg, undo_deskew_arg);
break;
case ONEBITRLEIMAGEVIEW:
return_arg = find_and_remove_staves_fujinaga(*((OneBitRleImageView*)self_arg), crossing_symbols_arg, n_stafflines_arg, staffline_height_arg, staffspace_height_arg, skew_strip_width_arg, max_skew_arg, deskew_only_arg, find_only_arg, undo_deskew_arg);
break;
case RLECC:
return_arg = find_and_remove_staves_fujinaga(*((RleCc*)self_arg), crossing_symbols_arg, n_stafflines_arg, staffline_height_arg, staffspace_height_arg, skew_strip_width_arg, max_skew_arg, deskew_only_arg, find_only_arg, undo_deskew_arg);
break;
case MLCC:
return_arg = find_and_remove_staves_fujinaga(*((MlCc*)self_arg), crossing_symbols_arg, n_stafflines_arg, staffline_height_arg, staffspace_height_arg, skew_strip_width_arg, max_skew_arg, deskew_only_arg, find_only_arg, undo_deskew_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'find_and_remove_staves_fujinaga' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(self_pyarg));
return 0;
}
                  } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      
                                                                                                                                                                                                              if (return_arg== NULL) {
              if (PyErr_Occurred() == NULL) {
                Py_INCREF(Py_None);
                return Py_None;
               } else
                return NULL;
            } else {
              return_pyarg = return_arg;              return return_pyarg;
            }
                              }
                static PyObject* call_global_staffline_deskew(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                                Image* return_arg;
PyObject* return_pyarg;
                                          Image* self_arg;
PyObject* self_pyarg;
                      double staffline_height_arg;
                      double staffspace_height_arg;
                      int skew_strip_width_arg;
                      double max_skew_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "Oddid:global_staffline_deskew"
                        ,
             &self_pyarg                        ,
             &staffline_height_arg                        ,
             &staffspace_height_arg                        ,
             &skew_strip_width_arg                        ,
             &max_skew_arg                      ) <= 0)
           return 0;
               
              if (!is_ImageObject(self_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'self' must be an image");
          return 0;
        }
        self_arg = ((Image*)((RectObject*)self_pyarg)->m_x);
        image_get_fv(self_pyarg, &self_arg->features, &self_arg->features_len);
                                                                      
              try {
                      switch(get_image_combination(self_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = global_staffline_deskew(*((OneBitImageView*)self_arg), staffline_height_arg, staffspace_height_arg, skew_strip_width_arg, max_skew_arg);
break;
case CC:
return_arg = global_staffline_deskew(*((Cc*)self_arg), staffline_height_arg, staffspace_height_arg, skew_strip_width_arg, max_skew_arg);
break;
case ONEBITRLEIMAGEVIEW:
return_arg = global_staffline_deskew(*((OneBitRleImageView*)self_arg), staffline_height_arg, staffspace_height_arg, skew_strip_width_arg, max_skew_arg);
break;
case RLECC:
return_arg = global_staffline_deskew(*((RleCc*)self_arg), staffline_height_arg, staffspace_height_arg, skew_strip_width_arg, max_skew_arg);
break;
case MLCC:
return_arg = global_staffline_deskew(*((MlCc*)self_arg), staffline_height_arg, staffspace_height_arg, skew_strip_width_arg, max_skew_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'global_staffline_deskew' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(self_pyarg));
return 0;
}
                  } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      
                                                                                                                    if (return_arg== NULL) {
              if (PyErr_Occurred() == NULL) {
                Py_INCREF(Py_None);
                return Py_None;
               } else
                return NULL;
            } else {
              return_pyarg = create_ImageObject(return_arg);              return return_pyarg;
            }
                              }
                            static PyObject* call_remove_staves_fujinaga(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                                Image* return_arg;
PyObject* return_pyarg;
                                          Image* self_arg;
PyObject* self_pyarg;
                      PyObject* staves_arg;
PyObject* staves_pyarg;
                      int crossing_symbols_arg;
                      double staffline_height_arg;
                      double staffspace_height_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "OOidd:remove_staves_fujinaga"
                        ,
             &self_pyarg                        ,
             &staves_pyarg                        ,
             &crossing_symbols_arg                        ,
             &staffline_height_arg                        ,
             &staffspace_height_arg                      ) <= 0)
           return 0;
               
              if (!is_ImageObject(self_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'self' must be an image");
          return 0;
        }
        self_arg = ((Image*)((RectObject*)self_pyarg)->m_x);
        image_get_fv(self_pyarg, &self_arg->features, &self_arg->features_len);
                      staves_arg = staves_pyarg;                                                
              try {
                      switch(get_image_combination(self_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = remove_staves_fujinaga(*((OneBitImageView*)self_arg), staves_arg, crossing_symbols_arg, staffline_height_arg, staffspace_height_arg);
break;
case CC:
return_arg = remove_staves_fujinaga(*((Cc*)self_arg), staves_arg, crossing_symbols_arg, staffline_height_arg, staffspace_height_arg);
break;
case ONEBITRLEIMAGEVIEW:
return_arg = remove_staves_fujinaga(*((OneBitRleImageView*)self_arg), staves_arg, crossing_symbols_arg, staffline_height_arg, staffspace_height_arg);
break;
case RLECC:
return_arg = remove_staves_fujinaga(*((RleCc*)self_arg), staves_arg, crossing_symbols_arg, staffline_height_arg, staffspace_height_arg);
break;
case MLCC:
return_arg = remove_staves_fujinaga(*((MlCc*)self_arg), staves_arg, crossing_symbols_arg, staffline_height_arg, staffspace_height_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'remove_staves_fujinaga' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(self_pyarg));
return 0;
}
                  } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      
                                                                                                                    if (return_arg== NULL) {
              if (PyErr_Occurred() == NULL) {
                Py_INCREF(Py_None);
                return Py_None;
               } else
                return NULL;
            } else {
              return_pyarg = create_ImageObject(return_arg);              return return_pyarg;
            }
                              }
            
  DL_EXPORT(void) init_staff_removal_fujinaga(void) {
    Py_InitModule(CHAR_PTR_CAST "_staff_removal_fujinaga", _staff_removal_fujinaga_methods);
  }
  

