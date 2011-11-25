
        
    
  #include "gameramodule.hpp"
  #include "knnmodule.hpp"

        #include "vector_field.hpp"
  
    #include <string>
  #include <stdexcept>
  #include "Python.h"
  #include <list>

  using namespace Gamera;
  
        
      extern "C" {
#ifndef _MSC_VER
    void init_vector_field(void);
#endif
                  static PyObject* call_compute_longest_chord_vectors(PyObject* self, PyObject* args);
                        static PyObject* call_angles_to_RGB(PyObject* self, PyObject* args);
                        static PyObject* call_keep_vectorfield_runs(PyObject* self, PyObject* args);
            }

          static PyMethodDef _vector_field_methods[] = {
                  { CHAR_PTR_CAST "compute_longest_chord_vectors",
          call_compute_longest_chord_vectors, METH_VARARGS,
          CHAR_PTR_CAST "**compute_longest_chord_vectors** (int *path_length* = 0, float(0.00, 90.00) *resolution* = 3.00)\n\nComputes the vector field from a onebit image.\n\nThis function determines about the longest chord where each black pixel is\nlocated at. A chord is a straight line, consisting of black pixels only. The\nreturn value consists of three FloatImages.\n\n*1st image*:\n    Contains the angle (in radians) of each pixel\n    the line running through the corresponding pixel in the given image. Each\n    pixel value will be in the range (-pi, pi].\n\n*2nd image*:\n    length of the longest chord at each pixel position\n\n*3rd image*:\n    Contains the thickness of the longest chord at each pixel position.\n    Thereby the thickness is the length of the chord perpendicular to the\n    detected longest chord (depending on *resolution* more or less\n    perpendicular).\n\n\nThis function is similar to compute_vector_field_, but is not that\ncomputationally intensive (depending on *resolution*).\n\nArguments:\n    \n*path_length*:\n    maximum length that will be tested for each chord\n\n*resolution*:\n   Angle that is used to scan the image (in degree). When the resolution is\n   set to 0, then the default value will be used instead.\n"        },
                        { CHAR_PTR_CAST "angles_to_RGB",
          call_angles_to_RGB, METH_VARARGS,
          CHAR_PTR_CAST "**angles_to_RGB** ([object *v_images*], int *path_length*)\n\nConverts angle information for each pixel to RGB values.\n\nEach angle is displayed in a specific color, e.g. 0 degrees is visualised in \ncyan (RGB (0, 255, 255)), and an angle of 90 degrees is shown in red\n(RGB (255, 0, 0)). The HSI color space is mapped to 180 degrees. Thus, angles\nof opposed chords will result in same colors, which is shown in the color\ncircle below.\n\nExample:\n\n.. image:: images/angles_to_RGB.png\n\nArgument:\n\n*v_images*:\n  List of 1 to 3 FloatImages, typically returned by\n  compute_longest_chord_vectors_. Their meaning is as follows:\n\n  *1st image*:\n    Affects the hue of the RGB image. The pixel values must be in the range\n    [-pi, pi] and are mapped to [0, 360] degrees.\n\n  *2nd image*:\n    Affects the saturation of the RGB image. The higher the pixel value the\n    higher the saturation. If not given, the saturation defaults to 1.0 (100\n    percent).\n\n  *3rd image*:\n    Affects the intensity of the RGB image. The smaller the pixel value the\n    higher the intensity. If not given, the intensity defaults to 1.0 (100\n    percent).\n\n*path_length*:\n  Maximum pixel value of the 2nd and the 3rd image. This value is used to\n  normalise the pixel information and to keep both the saturation and the\n  intensity in a range from [0, 1.0].\n"        },
                        { CHAR_PTR_CAST "keep_vectorfield_runs",
          call_keep_vectorfield_runs, METH_VARARGS,
          CHAR_PTR_CAST "**keep_vectorfield_runs** ([object *points*], int *height*, ``ChoiceString(strict)`` [up|down] *direction*, int *maxlength*, int *numangles*)\n\nThe returned image only contains those black pixels that belong to black\nruns crossing the given points at an angle equal the vector field value at\nthese points. All runs are only extracted in the given *direction* and up\nto the given *height*.\n\nArguments:\n\n*height*:\n  Black runs are only kept up to *height* in the given *direction* (see below).\n\n*points*:\n  Only tall runs crossing these points are kept.\n\n*direction*:\n  In which direction (``up`` or ``down``) ``height`` takes effect.\n\n*maxlength*, *numangles*:\n  Technical details for the computation of the vector field values\n  which are computed by picking the angle of the longest chord through\n  the point. *numangles* is the number of sample chords tested; chords\n  longer than *maxlength* are truncated (for performance reasons).\n"        },
              { NULL }
  };

                static PyObject* call_compute_longest_chord_vectors(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                                std::list<Image*>* return_arg;
PyObject* return_pyarg;
                                          Image* self_arg;
PyObject* self_pyarg;
                      int path_length_arg;
                      double resolution_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "Oid:compute_longest_chord_vectors"
                        ,
             &self_pyarg                        ,
             &path_length_arg                        ,
             &resolution_arg                      ) <= 0)
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
return_arg = compute_longest_chord_vectors(*((OneBitImageView*)self_arg), path_length_arg, resolution_arg);
break;
case CC:
return_arg = compute_longest_chord_vectors(*((Cc*)self_arg), path_length_arg, resolution_arg);
break;
case ONEBITRLEIMAGEVIEW:
return_arg = compute_longest_chord_vectors(*((OneBitRleImageView*)self_arg), path_length_arg, resolution_arg);
break;
case RLECC:
return_arg = compute_longest_chord_vectors(*((RleCc*)self_arg), path_length_arg, resolution_arg);
break;
case MLCC:
return_arg = compute_longest_chord_vectors(*((MlCc*)self_arg), path_length_arg, resolution_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'compute_longest_chord_vectors' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(self_pyarg));
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
                static PyObject* call_angles_to_RGB(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                                Image* return_arg;
PyObject* return_pyarg;
                                          Image* self_arg;
PyObject* self_pyarg;
                      ImageVector v_images_arg;
PyObject* v_images_pyarg;
                      int path_length_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "OOi:angles_to_RGB"
                        ,
             &self_pyarg                        ,
             &v_images_pyarg                        ,
             &path_length_arg                      ) <= 0)
           return 0;
               
              if (!is_ImageObject(self_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'self' must be an image");
          return 0;
        }
        self_arg = ((Image*)((RectObject*)self_pyarg)->m_x);
        image_get_fv(self_pyarg, &self_arg->features, &self_arg->features_len);
                      
          const char* type_error_v_images = "Argument 'v_images' must be an iterable of images.";
          PyObject* v_images_pyarg_seq = PySequence_Fast(v_images_pyarg, type_error_v_images);
          if (v_images_pyarg_seq == NULL)
            return 0;
          int v_images_arg_size = PySequence_Fast_GET_SIZE(v_images_pyarg_seq);
          v_images_arg.resize(v_images_arg_size);
          for (int i=0; i < v_images_arg_size; ++i) {
            PyObject *element = PySequence_Fast_GET_ITEM(v_images_pyarg_seq, i);
            if (!is_ImageObject(element)) {
              PyErr_SetString(PyExc_TypeError, type_error_v_images);
              return 0;
            }
            v_images_arg[i] = std::pair<Image*, int>((Image*)(((RectObject*)element)->m_x), get_image_combination(element));
            image_get_fv(element, &v_images_arg[i].first->features,
                         &v_images_arg[i].first->features_len);
          }
          Py_DECREF(v_images_pyarg_seq);                    
              try {
                      switch(get_image_combination(self_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = angles_to_RGB(*((OneBitImageView*)self_arg), v_images_arg, path_length_arg);
break;
case CC:
return_arg = angles_to_RGB(*((Cc*)self_arg), v_images_arg, path_length_arg);
break;
case ONEBITRLEIMAGEVIEW:
return_arg = angles_to_RGB(*((OneBitRleImageView*)self_arg), v_images_arg, path_length_arg);
break;
case RLECC:
return_arg = angles_to_RGB(*((RleCc*)self_arg), v_images_arg, path_length_arg);
break;
case MLCC:
return_arg = angles_to_RGB(*((MlCc*)self_arg), v_images_arg, path_length_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'angles_to_RGB' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(self_pyarg));
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
                static PyObject* call_keep_vectorfield_runs(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                                Image* return_arg;
PyObject* return_pyarg;
                                          Image* self_arg;
PyObject* self_pyarg;
                      PointVector* points_arg;
PyObject* points_pyarg;
                      int height_arg;
                      char* direction_arg;
                      int maxlength_arg;
                      int numangles_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "OOisii:keep_vectorfield_runs"
                        ,
             &self_pyarg                        ,
             &points_pyarg                        ,
             &height_arg                        ,
             &direction_arg                        ,
             &maxlength_arg                        ,
             &numangles_arg                      ) <= 0)
           return 0;
               
              if (!is_ImageObject(self_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'self' must be an image");
          return 0;
        }
        self_arg = ((Image*)((RectObject*)self_pyarg)->m_x);
        image_get_fv(self_pyarg, &self_arg->features, &self_arg->features_len);
                      
      points_arg = PointVector_from_python(points_pyarg);
                                                                    
              try {
                      switch(get_image_combination(self_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = keep_vectorfield_runs(*((OneBitImageView*)self_arg), points_arg, height_arg, direction_arg, maxlength_arg, numangles_arg);
break;
case CC:
return_arg = keep_vectorfield_runs(*((Cc*)self_arg), points_arg, height_arg, direction_arg, maxlength_arg, numangles_arg);
break;
case ONEBITRLEIMAGEVIEW:
return_arg = keep_vectorfield_runs(*((OneBitRleImageView*)self_arg), points_arg, height_arg, direction_arg, maxlength_arg, numangles_arg);
break;
case RLECC:
return_arg = keep_vectorfield_runs(*((RleCc*)self_arg), points_arg, height_arg, direction_arg, maxlength_arg, numangles_arg);
break;
case MLCC:
return_arg = keep_vectorfield_runs(*((MlCc*)self_arg), points_arg, height_arg, direction_arg, maxlength_arg, numangles_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'keep_vectorfield_runs' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(self_pyarg));
return 0;
}
                  } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      
                        delete points_arg;                                                                                                              if (return_arg== NULL) {
              if (PyErr_Occurred() == NULL) {
                Py_INCREF(Py_None);
                return Py_None;
               } else
                return NULL;
            } else {
              return_pyarg = create_ImageObject(return_arg);              return return_pyarg;
            }
                              }
      
  DL_EXPORT(void) init_vector_field(void) {
    Py_InitModule(CHAR_PTR_CAST "_vector_field", _vector_field_methods);
  }
  

