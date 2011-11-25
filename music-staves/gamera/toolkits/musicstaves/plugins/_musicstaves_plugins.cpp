
        
    
  #include "gameramodule.hpp"
  #include "knnmodule.hpp"

        #include "musicstaves_plugins.hpp"
  
    #include <string>
  #include <stdexcept>
  #include "Python.h"
  #include <list>

  using namespace Gamera;
  
        
      extern "C" {
#ifndef _MSC_VER
    void init_musicstaves_plugins(void);
#endif
                                                                    static PyObject* call_fill_horizontal_line_gaps(PyObject* self, PyObject* args);
                        static PyObject* call_fill_vertical_line_gaps(PyObject* self, PyObject* args);
                        static PyObject* call_match_staff_template(PyObject* self, PyObject* args);
                      }

          static PyMethodDef _musicstaves_plugins_methods[] = {
                                                                    { CHAR_PTR_CAST "fill_horizontal_line_gaps",
          call_fill_horizontal_line_gaps, METH_VARARGS,
          CHAR_PTR_CAST "**fill_horizontal_line_gaps** (int *windowwidth*, int(1, 100) *blackness*, ``bool`` *fill_average* = False)\n\nFills white gaps in horizontal lines with black.\n\nThe image is scanned with a line of *windowwidth* pixels.\nWhen the average blackness within this window is at least *blackness*\npercent, the middle pixel is turned black.\n\nThe parameter *fill_average* only applies to greyscale images: when *True*,\nthe middle pixel is not set to black but to the window average,\nif this average value is darker than the current pixel value.\nNote that this is different from a convolution because pixels\nare never turned lighter.\n    "        },
                        { CHAR_PTR_CAST "fill_vertical_line_gaps",
          call_fill_vertical_line_gaps, METH_VARARGS,
          CHAR_PTR_CAST "**fill_vertical_line_gaps** (int *windowheight*, int(1, 100) *blackness*, ``bool`` *fill_average* = False)\n\nFills white gaps in vertical lines with black.\n\nThe image is scanned with a vertical line of *windowwidth* pixels.\nWhen the average blackness within this window is at least *blackness*\npercent, the middle pixel is turned black.\n\nThe parameter *fill_average* only applies to greyscale images: when *True*,\nthe middle pixel is not set to black but to the window average,\nif this average value is darker than the current pixel value.\nNote that this is different from a convolution because pixels\nare never turned lighter.\n    "        },
                        { CHAR_PTR_CAST "match_staff_template",
          call_match_staff_template, METH_VARARGS,
          CHAR_PTR_CAST "**match_staff_template** (int *width*, int *line_distance*, int *nlines* = 2, int(1, 100) *blackness* = 70)\n\nExtracts all points from an image that match a staff template.\n\nThe template consists of *nlines* horizontal lines of width *width* with\na vertical distance *line_distance*. The midpoint of each line is extracted\nif each line contains more than *blackness* percent black pixels. In order\nto avoid cutting off the staff lines at the beginning and end of each staff,\nthe lines are extrapolated at the ends until no further black point is found.\n\nReasonable parameter values are *3 * staffspace_height / 2* for *width* and\n*staffspace_height + staffline_height* for *line_distance*.\n"        },
                        { NULL }
  };

                                              static PyObject* call_fill_horizontal_line_gaps(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                          Image* self_arg;
PyObject* self_pyarg;
                      int windowwidth_arg;
                      int blackness_arg;
                      int fill_average_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "Oiii:fill_horizontal_line_gaps"
                        ,
             &self_pyarg                        ,
             &windowwidth_arg                        ,
             &blackness_arg                        ,
             &fill_average_arg                      ) <= 0)
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
fill_horizontal_line_gaps(*((OneBitImageView*)self_arg), windowwidth_arg, blackness_arg, fill_average_arg);
break;
case CC:
fill_horizontal_line_gaps(*((Cc*)self_arg), windowwidth_arg, blackness_arg, fill_average_arg);
break;
case ONEBITRLEIMAGEVIEW:
fill_horizontal_line_gaps(*((OneBitRleImageView*)self_arg), windowwidth_arg, blackness_arg, fill_average_arg);
break;
case RLECC:
fill_horizontal_line_gaps(*((RleCc*)self_arg), windowwidth_arg, blackness_arg, fill_average_arg);
break;
case MLCC:
fill_horizontal_line_gaps(*((MlCc*)self_arg), windowwidth_arg, blackness_arg, fill_average_arg);
break;
case GREYSCALEIMAGEVIEW:
fill_horizontal_line_gaps(*((GreyScaleImageView*)self_arg), windowwidth_arg, blackness_arg, fill_average_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'fill_horizontal_line_gaps' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, ONEBIT, and GREYSCALE.", get_pixel_type_name(self_pyarg));
return 0;
}
                  } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      
                                                                                      Py_INCREF(Py_None);
          return Py_None;
                    }
                static PyObject* call_fill_vertical_line_gaps(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                          Image* self_arg;
PyObject* self_pyarg;
                      int windowheight_arg;
                      int blackness_arg;
                      int fill_average_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "Oiii:fill_vertical_line_gaps"
                        ,
             &self_pyarg                        ,
             &windowheight_arg                        ,
             &blackness_arg                        ,
             &fill_average_arg                      ) <= 0)
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
fill_vertical_line_gaps(*((OneBitImageView*)self_arg), windowheight_arg, blackness_arg, fill_average_arg);
break;
case CC:
fill_vertical_line_gaps(*((Cc*)self_arg), windowheight_arg, blackness_arg, fill_average_arg);
break;
case ONEBITRLEIMAGEVIEW:
fill_vertical_line_gaps(*((OneBitRleImageView*)self_arg), windowheight_arg, blackness_arg, fill_average_arg);
break;
case RLECC:
fill_vertical_line_gaps(*((RleCc*)self_arg), windowheight_arg, blackness_arg, fill_average_arg);
break;
case MLCC:
fill_vertical_line_gaps(*((MlCc*)self_arg), windowheight_arg, blackness_arg, fill_average_arg);
break;
case GREYSCALEIMAGEVIEW:
fill_vertical_line_gaps(*((GreyScaleImageView*)self_arg), windowheight_arg, blackness_arg, fill_average_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'fill_vertical_line_gaps' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, ONEBIT, and GREYSCALE.", get_pixel_type_name(self_pyarg));
return 0;
}
                  } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      
                                                                                      Py_INCREF(Py_None);
          return Py_None;
                    }
                static PyObject* call_match_staff_template(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                                Image* return_arg;
PyObject* return_pyarg;
                                          Image* self_arg;
PyObject* self_pyarg;
                      int width_arg;
                      int line_distance_arg;
                      int nlines_arg;
                      int blackness_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "Oiiii:match_staff_template"
                        ,
             &self_pyarg                        ,
             &width_arg                        ,
             &line_distance_arg                        ,
             &nlines_arg                        ,
             &blackness_arg                      ) <= 0)
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
return_arg = match_staff_template(*((OneBitImageView*)self_arg), width_arg, line_distance_arg, nlines_arg, blackness_arg);
break;
case CC:
return_arg = match_staff_template(*((Cc*)self_arg), width_arg, line_distance_arg, nlines_arg, blackness_arg);
break;
case ONEBITRLEIMAGEVIEW:
return_arg = match_staff_template(*((OneBitRleImageView*)self_arg), width_arg, line_distance_arg, nlines_arg, blackness_arg);
break;
case RLECC:
return_arg = match_staff_template(*((RleCc*)self_arg), width_arg, line_distance_arg, nlines_arg, blackness_arg);
break;
case MLCC:
return_arg = match_staff_template(*((MlCc*)self_arg), width_arg, line_distance_arg, nlines_arg, blackness_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'match_staff_template' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(self_pyarg));
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
            
  DL_EXPORT(void) init_musicstaves_plugins(void) {
    Py_InitModule(CHAR_PTR_CAST "_musicstaves_plugins", _musicstaves_plugins_methods);
  }
  

