/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library
  Module:  $URL$

  Copyright (c) 2006-2009 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __gdcmImageToImageFilter_h
#define __gdcmImageToImageFilter_h

#include "gdcmPixmapToPixmapFilter.h"

namespace gdcm
{

class Image;
/**
 * \brief ImageToImageFilter class
 * Super class for all filter taking an image and producing an output image
 */
class GDCM_EXPORT ImageToImageFilter : public PixmapToPixmapFilter
{
public:
  ImageToImageFilter();
  ~ImageToImageFilter() {}

  Image &GetInput();

  // NOTE: covariant return-type to preserve backward compatible API
  /// Get Output image
  const Image &GetOutput() const;

protected:
};

} // end namespace gdcm

#endif //__gdcmImageToImageFilter_h

