
        
    
  #include "gameramodule.hpp"
  #include "knnmodule.hpp"

        #include "evaluation.hpp"
  
    #include <string>
  #include <stdexcept>
  #include "Python.h"
  #include <list>

  using namespace Gamera;
  
        
      extern "C" {
#ifndef _MSC_VER
    void init_evaluation(void);
#endif
                            static PyObject* call_segment_error(PyObject* self, PyObject* args);
                        static PyObject* call_interruption_error(PyObject* self, PyObject* args);
            }

          static PyMethodDef _evaluation_methods[] = {
                            { CHAR_PTR_CAST "segment_error",
          call_segment_error, METH_VARARGS,
          CHAR_PTR_CAST "**segment_error** (``Image`` [OneBit] *Gstaves*, ``Image`` [OneBit] *Sstaves*)\n\nReturns the rate of misclassified staff segments \nduring staff removal.\n\nCompares two segmentations by building equivalence classes of\noverlapping segments as described in the reference below. Each\nclass is assigned an error type depending on how many ground truth\nand test segments it contains. The return value is a tuple\n(*n1,n2,n3,n4,n5,n6)* where each value is the total number of classes\nwith the corresponding error type:\n\n+------+-----------------------+---------------+----------------------+\n| Nr   | Ground truth segments | Test segments | Error type           |\n+======+=======================+===============+======================+\n| *n1* | 1                     | 1             | correct              |\n+------+-----------------------+---------------+----------------------+\n| *n2* | 1                     | 0             | missed segment       |\n+------+-----------------------+---------------+----------------------+\n| *n3* | 0                     | 1             | false positive       |\n+------+-----------------------+---------------+----------------------+\n| *n4* | 1                     | > 1           | split                |\n+------+-----------------------+---------------+----------------------+\n| *n5* | > 1                   | 1             | merge                |\n+------+-----------------------+---------------+----------------------+\n| *n6* | > 1                   | > 1           | splits and merges    |\n+------+-----------------------+---------------+----------------------+\n\nInput arguments:\n\n  *Gstaves*:\n    ground truth image containing only the staff segments that should\n    be removed\n\n  *Sstaves*:\n    image containing the actually removed staff segments\n\nReferences:\n\n  M. Thulke, V. Margner, A. Dengel:\n  *A general approach to quality evaluation of document\n  segmentation results.*\n  Lecture Notes in Computer Science 1655, pp. 43-57 (1999)\n"        },
                        { CHAR_PTR_CAST "interruption_error",
          call_interruption_error, METH_VARARGS,
          CHAR_PTR_CAST "**interruption_error** (``Image`` [OneBit] *Gstaves*, ``Image`` [OneBit] *Sstaves*, [object *Skeletons*])\n\nReturns error information for staff line interruptions.\n\nCompares the staff-only image of the \"ground truth\" image *Gstaves*\nwith the staff-only image of a staff removal result *Sstaves* (image\nthat contains exactly the removed pixels). A list of\nStafflineSkeleton-objects *Skeletons* is also needed to find the\nstafflines in the images. This algorithm recognizes interruptions in\nthe stafflines, and tries to link them by matching their x-positions.\nAfter that it removes links, so that every interruption is linked at\nmost once. The return value is a tuple (*n1,n2,n3)* describing the\nresults:\n\n+------+--------------------------------------------------------------+\n|      | Meaning                                                      |\n+======+==============================================================+\n| *n1* | interruptions without link (either Gstaves or Sstaves)       |\n+------+--------------------------------------------------------------+\n| *n2* | removed links because of doubly linked interruptions         |\n+------+--------------------------------------------------------------+\n| *n3* | interruptions in Gstaves                                     |\n+------+--------------------------------------------------------------+\n\nIn the following image, you can see 2 interruptions without link (blue),\n2 removed links (red) and 7 interruptions in Gstaves, so the return value\nwould be (2,2,7):\n\n.. image:: images/staffinterrupt.png\n"        },
              { NULL }
  };

                      static PyObject* call_segment_error(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                        IntVector* return_arg;
PyObject* return_pyarg;
                                          Image* Gstaves_arg;
PyObject* Gstaves_pyarg;
                      Image* Sstaves_arg;
PyObject* Sstaves_pyarg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "OO:segment_error"
                        ,
             &Gstaves_pyarg                        ,
             &Sstaves_pyarg                      ) <= 0)
           return 0;
               
              if (!is_ImageObject(Gstaves_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'Gstaves' must be an image");
          return 0;
        }
        Gstaves_arg = ((Image*)((RectObject*)Gstaves_pyarg)->m_x);
        image_get_fv(Gstaves_pyarg, &Gstaves_arg->features, &Gstaves_arg->features_len);
                      if (!is_ImageObject(Sstaves_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'Sstaves' must be an image");
          return 0;
        }
        Sstaves_arg = ((Image*)((RectObject*)Sstaves_pyarg)->m_x);
        image_get_fv(Sstaves_pyarg, &Sstaves_arg->features, &Sstaves_arg->features_len);
              
              try {
                      switch(get_image_combination(Gstaves_pyarg)) {
case ONEBITIMAGEVIEW:
switch(get_image_combination(Sstaves_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = segment_error(*((OneBitImageView*)Gstaves_arg), *((OneBitImageView*)Sstaves_arg));
break;
case CC:
return_arg = segment_error(*((OneBitImageView*)Gstaves_arg), *((Cc*)Sstaves_arg));
break;
case ONEBITRLEIMAGEVIEW:
return_arg = segment_error(*((OneBitImageView*)Gstaves_arg), *((OneBitRleImageView*)Sstaves_arg));
break;
case RLECC:
return_arg = segment_error(*((OneBitImageView*)Gstaves_arg), *((RleCc*)Sstaves_arg));
break;
case MLCC:
return_arg = segment_error(*((OneBitImageView*)Gstaves_arg), *((MlCc*)Sstaves_arg));
break;
default:
PyErr_Format(PyExc_TypeError,"The 'Sstaves' argument of 'segment_error' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(Sstaves_pyarg));
return 0;
}
break;
case CC:
switch(get_image_combination(Sstaves_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = segment_error(*((Cc*)Gstaves_arg), *((OneBitImageView*)Sstaves_arg));
break;
case CC:
return_arg = segment_error(*((Cc*)Gstaves_arg), *((Cc*)Sstaves_arg));
break;
case ONEBITRLEIMAGEVIEW:
return_arg = segment_error(*((Cc*)Gstaves_arg), *((OneBitRleImageView*)Sstaves_arg));
break;
case RLECC:
return_arg = segment_error(*((Cc*)Gstaves_arg), *((RleCc*)Sstaves_arg));
break;
case MLCC:
return_arg = segment_error(*((Cc*)Gstaves_arg), *((MlCc*)Sstaves_arg));
break;
default:
PyErr_Format(PyExc_TypeError,"The 'Sstaves' argument of 'segment_error' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(Sstaves_pyarg));
return 0;
}
break;
case ONEBITRLEIMAGEVIEW:
switch(get_image_combination(Sstaves_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = segment_error(*((OneBitRleImageView*)Gstaves_arg), *((OneBitImageView*)Sstaves_arg));
break;
case CC:
return_arg = segment_error(*((OneBitRleImageView*)Gstaves_arg), *((Cc*)Sstaves_arg));
break;
case ONEBITRLEIMAGEVIEW:
return_arg = segment_error(*((OneBitRleImageView*)Gstaves_arg), *((OneBitRleImageView*)Sstaves_arg));
break;
case RLECC:
return_arg = segment_error(*((OneBitRleImageView*)Gstaves_arg), *((RleCc*)Sstaves_arg));
break;
case MLCC:
return_arg = segment_error(*((OneBitRleImageView*)Gstaves_arg), *((MlCc*)Sstaves_arg));
break;
default:
PyErr_Format(PyExc_TypeError,"The 'Sstaves' argument of 'segment_error' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(Sstaves_pyarg));
return 0;
}
break;
case RLECC:
switch(get_image_combination(Sstaves_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = segment_error(*((RleCc*)Gstaves_arg), *((OneBitImageView*)Sstaves_arg));
break;
case CC:
return_arg = segment_error(*((RleCc*)Gstaves_arg), *((Cc*)Sstaves_arg));
break;
case ONEBITRLEIMAGEVIEW:
return_arg = segment_error(*((RleCc*)Gstaves_arg), *((OneBitRleImageView*)Sstaves_arg));
break;
case RLECC:
return_arg = segment_error(*((RleCc*)Gstaves_arg), *((RleCc*)Sstaves_arg));
break;
case MLCC:
return_arg = segment_error(*((RleCc*)Gstaves_arg), *((MlCc*)Sstaves_arg));
break;
default:
PyErr_Format(PyExc_TypeError,"The 'Sstaves' argument of 'segment_error' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(Sstaves_pyarg));
return 0;
}
break;
case MLCC:
switch(get_image_combination(Sstaves_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = segment_error(*((MlCc*)Gstaves_arg), *((OneBitImageView*)Sstaves_arg));
break;
case CC:
return_arg = segment_error(*((MlCc*)Gstaves_arg), *((Cc*)Sstaves_arg));
break;
case ONEBITRLEIMAGEVIEW:
return_arg = segment_error(*((MlCc*)Gstaves_arg), *((OneBitRleImageView*)Sstaves_arg));
break;
case RLECC:
return_arg = segment_error(*((MlCc*)Gstaves_arg), *((RleCc*)Sstaves_arg));
break;
case MLCC:
return_arg = segment_error(*((MlCc*)Gstaves_arg), *((MlCc*)Sstaves_arg));
break;
default:
PyErr_Format(PyExc_TypeError,"The 'Sstaves' argument of 'segment_error' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(Sstaves_pyarg));
return 0;
}
break;
default:
PyErr_Format(PyExc_TypeError,"The 'Gstaves' argument of 'segment_error' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(Gstaves_pyarg));
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
              
      return_pyarg = IntVector_to_python(return_arg);
      delete return_arg;
                    return return_pyarg;
            }
                              }
                static PyObject* call_interruption_error(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                        IntVector* return_arg;
PyObject* return_pyarg;
                                          Image* Gstaves_arg;
PyObject* Gstaves_pyarg;
                      Image* Sstaves_arg;
PyObject* Sstaves_pyarg;
                      PyObject* Skeletons_arg;
PyObject* Skeletons_pyarg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "OOO:interruption_error"
                        ,
             &Gstaves_pyarg                        ,
             &Sstaves_pyarg                        ,
             &Skeletons_pyarg                      ) <= 0)
           return 0;
               
              if (!is_ImageObject(Gstaves_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'Gstaves' must be an image");
          return 0;
        }
        Gstaves_arg = ((Image*)((RectObject*)Gstaves_pyarg)->m_x);
        image_get_fv(Gstaves_pyarg, &Gstaves_arg->features, &Gstaves_arg->features_len);
                      if (!is_ImageObject(Sstaves_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'Sstaves' must be an image");
          return 0;
        }
        Sstaves_arg = ((Image*)((RectObject*)Sstaves_pyarg)->m_x);
        image_get_fv(Sstaves_pyarg, &Sstaves_arg->features, &Sstaves_arg->features_len);
                      Skeletons_arg = Skeletons_pyarg;      
              try {
                      switch(get_image_combination(Gstaves_pyarg)) {
case ONEBITIMAGEVIEW:
switch(get_image_combination(Sstaves_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = interruption_error(*((OneBitImageView*)Gstaves_arg), *((OneBitImageView*)Sstaves_arg), Skeletons_arg);
break;
case CC:
return_arg = interruption_error(*((OneBitImageView*)Gstaves_arg), *((Cc*)Sstaves_arg), Skeletons_arg);
break;
case ONEBITRLEIMAGEVIEW:
return_arg = interruption_error(*((OneBitImageView*)Gstaves_arg), *((OneBitRleImageView*)Sstaves_arg), Skeletons_arg);
break;
case RLECC:
return_arg = interruption_error(*((OneBitImageView*)Gstaves_arg), *((RleCc*)Sstaves_arg), Skeletons_arg);
break;
case MLCC:
return_arg = interruption_error(*((OneBitImageView*)Gstaves_arg), *((MlCc*)Sstaves_arg), Skeletons_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'Sstaves' argument of 'interruption_error' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(Sstaves_pyarg));
return 0;
}
break;
case CC:
switch(get_image_combination(Sstaves_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = interruption_error(*((Cc*)Gstaves_arg), *((OneBitImageView*)Sstaves_arg), Skeletons_arg);
break;
case CC:
return_arg = interruption_error(*((Cc*)Gstaves_arg), *((Cc*)Sstaves_arg), Skeletons_arg);
break;
case ONEBITRLEIMAGEVIEW:
return_arg = interruption_error(*((Cc*)Gstaves_arg), *((OneBitRleImageView*)Sstaves_arg), Skeletons_arg);
break;
case RLECC:
return_arg = interruption_error(*((Cc*)Gstaves_arg), *((RleCc*)Sstaves_arg), Skeletons_arg);
break;
case MLCC:
return_arg = interruption_error(*((Cc*)Gstaves_arg), *((MlCc*)Sstaves_arg), Skeletons_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'Sstaves' argument of 'interruption_error' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(Sstaves_pyarg));
return 0;
}
break;
case ONEBITRLEIMAGEVIEW:
switch(get_image_combination(Sstaves_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = interruption_error(*((OneBitRleImageView*)Gstaves_arg), *((OneBitImageView*)Sstaves_arg), Skeletons_arg);
break;
case CC:
return_arg = interruption_error(*((OneBitRleImageView*)Gstaves_arg), *((Cc*)Sstaves_arg), Skeletons_arg);
break;
case ONEBITRLEIMAGEVIEW:
return_arg = interruption_error(*((OneBitRleImageView*)Gstaves_arg), *((OneBitRleImageView*)Sstaves_arg), Skeletons_arg);
break;
case RLECC:
return_arg = interruption_error(*((OneBitRleImageView*)Gstaves_arg), *((RleCc*)Sstaves_arg), Skeletons_arg);
break;
case MLCC:
return_arg = interruption_error(*((OneBitRleImageView*)Gstaves_arg), *((MlCc*)Sstaves_arg), Skeletons_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'Sstaves' argument of 'interruption_error' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(Sstaves_pyarg));
return 0;
}
break;
case RLECC:
switch(get_image_combination(Sstaves_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = interruption_error(*((RleCc*)Gstaves_arg), *((OneBitImageView*)Sstaves_arg), Skeletons_arg);
break;
case CC:
return_arg = interruption_error(*((RleCc*)Gstaves_arg), *((Cc*)Sstaves_arg), Skeletons_arg);
break;
case ONEBITRLEIMAGEVIEW:
return_arg = interruption_error(*((RleCc*)Gstaves_arg), *((OneBitRleImageView*)Sstaves_arg), Skeletons_arg);
break;
case RLECC:
return_arg = interruption_error(*((RleCc*)Gstaves_arg), *((RleCc*)Sstaves_arg), Skeletons_arg);
break;
case MLCC:
return_arg = interruption_error(*((RleCc*)Gstaves_arg), *((MlCc*)Sstaves_arg), Skeletons_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'Sstaves' argument of 'interruption_error' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(Sstaves_pyarg));
return 0;
}
break;
case MLCC:
switch(get_image_combination(Sstaves_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = interruption_error(*((MlCc*)Gstaves_arg), *((OneBitImageView*)Sstaves_arg), Skeletons_arg);
break;
case CC:
return_arg = interruption_error(*((MlCc*)Gstaves_arg), *((Cc*)Sstaves_arg), Skeletons_arg);
break;
case ONEBITRLEIMAGEVIEW:
return_arg = interruption_error(*((MlCc*)Gstaves_arg), *((OneBitRleImageView*)Sstaves_arg), Skeletons_arg);
break;
case RLECC:
return_arg = interruption_error(*((MlCc*)Gstaves_arg), *((RleCc*)Sstaves_arg), Skeletons_arg);
break;
case MLCC:
return_arg = interruption_error(*((MlCc*)Gstaves_arg), *((MlCc*)Sstaves_arg), Skeletons_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'Sstaves' argument of 'interruption_error' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(Sstaves_pyarg));
return 0;
}
break;
default:
PyErr_Format(PyExc_TypeError,"The 'Gstaves' argument of 'interruption_error' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(Gstaves_pyarg));
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
              
      return_pyarg = IntVector_to_python(return_arg);
      delete return_arg;
                    return return_pyarg;
            }
                              }
      
  DL_EXPORT(void) init_evaluation(void) {
    Py_InitModule(CHAR_PTR_CAST "_evaluation", _evaluation_methods);
  }
  

