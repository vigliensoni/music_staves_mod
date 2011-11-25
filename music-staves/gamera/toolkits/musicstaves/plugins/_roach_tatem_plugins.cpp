
        
    
  #include "gameramodule.hpp"
  #include "knnmodule.hpp"

        #include "roach_tatem_plugins.hpp"
  
    #include <string>
  #include <stdexcept>
  #include "Python.h"
  #include <list>

  using namespace Gamera;
  
        
      extern "C" {
#ifndef _MSC_VER
    void init_roach_tatem_plugins(void);
#endif
                  static PyObject* call_compute_vector_field(PyObject* self, PyObject* args);
                        static PyObject* call_mark_horizontal_lines_rt(PyObject* self, PyObject* args);
                        static PyObject* call_remove_stafflines_rt(PyObject* self, PyObject* args);
                                }

          static PyMethodDef _roach_tatem_plugins_methods[] = {
                  { CHAR_PTR_CAST "compute_vector_field",
          call_compute_vector_field, METH_VARARGS,
          CHAR_PTR_CAST "**compute_vector_field** (int(1, 1000) *window_radius* = 0)\n\nComputes the vector field from a onebit image.\n\nPart of the Roach and Tatem staff removal algorithm and implements\nSection 3.2 of the paper.\n\nSince the distance and thickness are not used by the staff removal\npart of Roach and Tatem's algorithm, they are not stored.\n\n*radius*\n   The radius of the circular window used to compute the vector\n   field.\n\nThe return value is a FloatImage, where each pixel is the angle (in\nradians) of the line running through the corresponding pixel in the\ngiven image.  Each pixel value will be in the range (-pi, pi].\n\nThis function is very computationally intensive.\n"        },
                        { CHAR_PTR_CAST "mark_horizontal_lines_rt",
          call_mark_horizontal_lines_rt, METH_VARARGS,
          CHAR_PTR_CAST "**mark_horizontal_lines_rt** (``Image`` [Float] *vector_image*, float *angle_threshold*, object *thickness_image*, int *staffline_height* = 0)\n\nGiven the original image and a vector field, returns a tuple of two\nimages: the first containing pixels believed to be part of stafflines,\nand the second of \"questionable pixels\", meaning pixels that could be\neither part of stafflines or musical figures.\n\nWhen only the original image and a vector field are given, this function is\npart of the Roach and Tatem staff removal algorithm, and implements Section\n3.4 of their paper.\n\nWhen the additional FloatImage *thickness* and *staffline_height* are given,\nthe implementation will not mark pixels as part of horizontal lines, which\nthickness is greater than *staffline_height*. When *staffline_height* is set\nto 0 it will default to the size of the most frequent black vertical run\n(considered to be the average staffline height).\n"        },
                        { CHAR_PTR_CAST "remove_stafflines_rt",
          call_remove_stafflines_rt, METH_VARARGS,
          CHAR_PTR_CAST "**remove_stafflines_rt** (``Image`` [OneBit] *lines*)\n\nGiven the original image and an image of probably stafflines,\nremoves the stafflines and returns the result.\n\nPart of the Roach and Tatem staff removal algorithm, and implements\nSection 3.5 of their paper.\n"        },
                                  { NULL }
  };

                static PyObject* call_compute_vector_field(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                                Image* return_arg;
PyObject* return_pyarg;
                                          Image* self_arg;
PyObject* self_pyarg;
                      int window_radius_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "Oi:compute_vector_field"
                        ,
             &self_pyarg                        ,
             &window_radius_arg                      ) <= 0)
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
return_arg = compute_vector_field(*((OneBitImageView*)self_arg), window_radius_arg);
break;
case CC:
return_arg = compute_vector_field(*((Cc*)self_arg), window_radius_arg);
break;
case ONEBITRLEIMAGEVIEW:
return_arg = compute_vector_field(*((OneBitRleImageView*)self_arg), window_radius_arg);
break;
case RLECC:
return_arg = compute_vector_field(*((RleCc*)self_arg), window_radius_arg);
break;
case MLCC:
return_arg = compute_vector_field(*((MlCc*)self_arg), window_radius_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'compute_vector_field' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(self_pyarg));
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
                static PyObject* call_mark_horizontal_lines_rt(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                                std::list<Image*>* return_arg;
PyObject* return_pyarg;
                                          Image* self_arg;
PyObject* self_pyarg;
                      Image* vector_image_arg;
PyObject* vector_image_pyarg;
                      double angle_threshold_arg;
                      PyObject* thickness_image_arg;
PyObject* thickness_image_pyarg;
                      int staffline_height_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "OOdOi:mark_horizontal_lines_rt"
                        ,
             &self_pyarg                        ,
             &vector_image_pyarg                        ,
             &angle_threshold_arg                        ,
             &thickness_image_pyarg                        ,
             &staffline_height_arg                      ) <= 0)
           return 0;
               
              if (!is_ImageObject(self_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'self' must be an image");
          return 0;
        }
        self_arg = ((Image*)((RectObject*)self_pyarg)->m_x);
        image_get_fv(self_pyarg, &self_arg->features, &self_arg->features_len);
                      if (!is_ImageObject(vector_image_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'vector_image' must be an image");
          return 0;
        }
        vector_image_arg = ((Image*)((RectObject*)vector_image_pyarg)->m_x);
        image_get_fv(vector_image_pyarg, &vector_image_arg->features, &vector_image_arg->features_len);
                                    thickness_image_arg = thickness_image_pyarg;                    
              try {
                      switch(get_image_combination(self_pyarg)) {
case ONEBITIMAGEVIEW:
switch(get_image_combination(vector_image_pyarg)) {
case FLOATIMAGEVIEW:
return_arg = mark_horizontal_lines_rt(*((OneBitImageView*)self_arg), *((FloatImageView*)vector_image_arg), angle_threshold_arg, thickness_image_arg, staffline_height_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'vector_image' argument of 'mark_horizontal_lines_rt' can not have pixel type '%s'. Acceptable value is FLOAT.", get_pixel_type_name(vector_image_pyarg));
return 0;
}
break;
case CC:
switch(get_image_combination(vector_image_pyarg)) {
case FLOATIMAGEVIEW:
return_arg = mark_horizontal_lines_rt(*((Cc*)self_arg), *((FloatImageView*)vector_image_arg), angle_threshold_arg, thickness_image_arg, staffline_height_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'vector_image' argument of 'mark_horizontal_lines_rt' can not have pixel type '%s'. Acceptable value is FLOAT.", get_pixel_type_name(vector_image_pyarg));
return 0;
}
break;
case ONEBITRLEIMAGEVIEW:
switch(get_image_combination(vector_image_pyarg)) {
case FLOATIMAGEVIEW:
return_arg = mark_horizontal_lines_rt(*((OneBitRleImageView*)self_arg), *((FloatImageView*)vector_image_arg), angle_threshold_arg, thickness_image_arg, staffline_height_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'vector_image' argument of 'mark_horizontal_lines_rt' can not have pixel type '%s'. Acceptable value is FLOAT.", get_pixel_type_name(vector_image_pyarg));
return 0;
}
break;
case RLECC:
switch(get_image_combination(vector_image_pyarg)) {
case FLOATIMAGEVIEW:
return_arg = mark_horizontal_lines_rt(*((RleCc*)self_arg), *((FloatImageView*)vector_image_arg), angle_threshold_arg, thickness_image_arg, staffline_height_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'vector_image' argument of 'mark_horizontal_lines_rt' can not have pixel type '%s'. Acceptable value is FLOAT.", get_pixel_type_name(vector_image_pyarg));
return 0;
}
break;
case MLCC:
switch(get_image_combination(vector_image_pyarg)) {
case FLOATIMAGEVIEW:
return_arg = mark_horizontal_lines_rt(*((MlCc*)self_arg), *((FloatImageView*)vector_image_arg), angle_threshold_arg, thickness_image_arg, staffline_height_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'vector_image' argument of 'mark_horizontal_lines_rt' can not have pixel type '%s'. Acceptable value is FLOAT.", get_pixel_type_name(vector_image_pyarg));
return 0;
}
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'mark_horizontal_lines_rt' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(self_pyarg));
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
              
      return_pyarg = ImageList_to_python(return_arg);
      delete return_arg;
                    return return_pyarg;
            }
                              }
                static PyObject* call_remove_stafflines_rt(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                                Image* return_arg;
PyObject* return_pyarg;
                                          Image* self_arg;
PyObject* self_pyarg;
                      Image* lines_arg;
PyObject* lines_pyarg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "OO:remove_stafflines_rt"
                        ,
             &self_pyarg                        ,
             &lines_pyarg                      ) <= 0)
           return 0;
               
              if (!is_ImageObject(self_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'self' must be an image");
          return 0;
        }
        self_arg = ((Image*)((RectObject*)self_pyarg)->m_x);
        image_get_fv(self_pyarg, &self_arg->features, &self_arg->features_len);
                      if (!is_ImageObject(lines_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'lines' must be an image");
          return 0;
        }
        lines_arg = ((Image*)((RectObject*)lines_pyarg)->m_x);
        image_get_fv(lines_pyarg, &lines_arg->features, &lines_arg->features_len);
              
              try {
                      switch(get_image_combination(self_pyarg)) {
case ONEBITIMAGEVIEW:
switch(get_image_combination(lines_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = remove_stafflines_rt(*((OneBitImageView*)self_arg), *((OneBitImageView*)lines_arg));
break;
case CC:
return_arg = remove_stafflines_rt(*((OneBitImageView*)self_arg), *((Cc*)lines_arg));
break;
case ONEBITRLEIMAGEVIEW:
return_arg = remove_stafflines_rt(*((OneBitImageView*)self_arg), *((OneBitRleImageView*)lines_arg));
break;
case RLECC:
return_arg = remove_stafflines_rt(*((OneBitImageView*)self_arg), *((RleCc*)lines_arg));
break;
case MLCC:
return_arg = remove_stafflines_rt(*((OneBitImageView*)self_arg), *((MlCc*)lines_arg));
break;
default:
PyErr_Format(PyExc_TypeError,"The 'lines' argument of 'remove_stafflines_rt' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(lines_pyarg));
return 0;
}
break;
case CC:
switch(get_image_combination(lines_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = remove_stafflines_rt(*((Cc*)self_arg), *((OneBitImageView*)lines_arg));
break;
case CC:
return_arg = remove_stafflines_rt(*((Cc*)self_arg), *((Cc*)lines_arg));
break;
case ONEBITRLEIMAGEVIEW:
return_arg = remove_stafflines_rt(*((Cc*)self_arg), *((OneBitRleImageView*)lines_arg));
break;
case RLECC:
return_arg = remove_stafflines_rt(*((Cc*)self_arg), *((RleCc*)lines_arg));
break;
case MLCC:
return_arg = remove_stafflines_rt(*((Cc*)self_arg), *((MlCc*)lines_arg));
break;
default:
PyErr_Format(PyExc_TypeError,"The 'lines' argument of 'remove_stafflines_rt' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(lines_pyarg));
return 0;
}
break;
case ONEBITRLEIMAGEVIEW:
switch(get_image_combination(lines_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = remove_stafflines_rt(*((OneBitRleImageView*)self_arg), *((OneBitImageView*)lines_arg));
break;
case CC:
return_arg = remove_stafflines_rt(*((OneBitRleImageView*)self_arg), *((Cc*)lines_arg));
break;
case ONEBITRLEIMAGEVIEW:
return_arg = remove_stafflines_rt(*((OneBitRleImageView*)self_arg), *((OneBitRleImageView*)lines_arg));
break;
case RLECC:
return_arg = remove_stafflines_rt(*((OneBitRleImageView*)self_arg), *((RleCc*)lines_arg));
break;
case MLCC:
return_arg = remove_stafflines_rt(*((OneBitRleImageView*)self_arg), *((MlCc*)lines_arg));
break;
default:
PyErr_Format(PyExc_TypeError,"The 'lines' argument of 'remove_stafflines_rt' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(lines_pyarg));
return 0;
}
break;
case RLECC:
switch(get_image_combination(lines_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = remove_stafflines_rt(*((RleCc*)self_arg), *((OneBitImageView*)lines_arg));
break;
case CC:
return_arg = remove_stafflines_rt(*((RleCc*)self_arg), *((Cc*)lines_arg));
break;
case ONEBITRLEIMAGEVIEW:
return_arg = remove_stafflines_rt(*((RleCc*)self_arg), *((OneBitRleImageView*)lines_arg));
break;
case RLECC:
return_arg = remove_stafflines_rt(*((RleCc*)self_arg), *((RleCc*)lines_arg));
break;
case MLCC:
return_arg = remove_stafflines_rt(*((RleCc*)self_arg), *((MlCc*)lines_arg));
break;
default:
PyErr_Format(PyExc_TypeError,"The 'lines' argument of 'remove_stafflines_rt' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(lines_pyarg));
return 0;
}
break;
case MLCC:
switch(get_image_combination(lines_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = remove_stafflines_rt(*((MlCc*)self_arg), *((OneBitImageView*)lines_arg));
break;
case CC:
return_arg = remove_stafflines_rt(*((MlCc*)self_arg), *((Cc*)lines_arg));
break;
case ONEBITRLEIMAGEVIEW:
return_arg = remove_stafflines_rt(*((MlCc*)self_arg), *((OneBitRleImageView*)lines_arg));
break;
case RLECC:
return_arg = remove_stafflines_rt(*((MlCc*)self_arg), *((RleCc*)lines_arg));
break;
case MLCC:
return_arg = remove_stafflines_rt(*((MlCc*)self_arg), *((MlCc*)lines_arg));
break;
default:
PyErr_Format(PyExc_TypeError,"The 'lines' argument of 'remove_stafflines_rt' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(lines_pyarg));
return 0;
}
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'remove_stafflines_rt' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(self_pyarg));
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
                  
  DL_EXPORT(void) init_roach_tatem_plugins(void) {
    Py_InitModule(CHAR_PTR_CAST "_roach_tatem_plugins", _roach_tatem_plugins_methods);
  }
  

