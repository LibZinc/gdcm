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
#ifndef __gdcmImageChangeTransferSyntax_h
#define __gdcmImageChangeTransferSyntax_h

#include "gdcmImageToImageFilter.h"
#include "gdcmTransferSyntax.h"

namespace gdcm
{

class DataElement;
class ImageCodec;
/**
 * \brief ImageChangeTransferSyntax class
 * Class to change the transfer syntax of an input DICOM
 *
 * If only Force param is set but no input TransferSyntax is set, it is assumed that user only wants
 * to inspect encapsulated stream (advanced dev. option).
 */
class GDCM_EXPORT ImageChangeTransferSyntax : public ImageToImageFilter
{
public:
  ImageChangeTransferSyntax():TS(TransferSyntax::TS_END),Force(false),CompressIconImage(false),UserCodec(0) {}
  ~ImageChangeTransferSyntax() {}

  /// Set target Transfer Syntax
  void SetTransferSyntax(const TransferSyntax &ts) { TS = ts; }
  /// Get Transfer Syntax
  const TransferSyntax &GetTransferSyntax() const { return TS; }

  /// Change
  bool Change();

  /// Decide whether or not to also compress the Icon Image using the same Transfer Syntax
  /// Default is to simply decompress icon image
  void SetCompressIconImage(bool b) { CompressIconImage = b; }

  /// When target Transfer Syntax is identical to input target syntax, no operation
  /// is actually done
  /// This is an issue when someone wants to recompress using GDCM internal implementation
  /// a JPEG (for example) image
  void SetForce( bool f ) { Force = f; }

  void SetUserCodec(ImageCodec *ic) { UserCodec = ic; }

protected:
  bool TryJPEGCodec(const DataElement &pixelde, Bitmap const &input, Bitmap &output);
  bool TryJPEG2000Codec(const DataElement &pixelde, Bitmap const &input, Bitmap &output);
  bool TryJPEGLSCodec(const DataElement &pixelde, Bitmap const &input, Bitmap &output);
  bool TryRAWCodec(const DataElement &pixelde, Bitmap const &input, Bitmap &output);
  bool TryRLECodec(const DataElement &pixelde, Bitmap const &input, Bitmap &output);

private:
  TransferSyntax TS;
  bool Force;
  bool CompressIconImage;

  ImageCodec *UserCodec;
};

/**
 * \example StandardizeFiles.cs
 * This is a C++ example on how to use gdcm::ImageChangeTransferSyntax
 */

} // end namespace gdcm

#endif //__gdcmImageChangeTransferSyntax_h

