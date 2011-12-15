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
#ifndef __gdcmBase64_h
#define __gdcmBase64_h

#include "gdcmTypes.h"

namespace gdcm
{
/**
 * \brief Class for Base64
 *
 */
//-----------------------------------------------------------------------------
class Base64Internals;
class GDCM_EXPORT Base64
{
public :
  Base64();
  ~Base64();

/**
 *                 Call this function with dlen = 0 to obtain the
 *                 required buffer size in dlen
 */
static int GetEncodeLength(const char *src, int  slen );

/**
 * \brief          Encode a buffer into base64 format
 *
 * \param dst      destination buffer
 * \param dlen     size of the buffer
 * \param src      source buffer
 * \param slen     amount of data to be encoded
 *
 * \return         0 if successful, or POLARSSL_ERR_BASE64_BUFFER_TOO_SMALL.
 *
 */
static int Encode( char *dst, int dlen,
                   const char *src, int  slen );

/**
 *                 Call this function with *dlen = 0 to obtain the
 *                 required buffer size in *dlen
 */
static int GetDecodeLength( const char *src, int  slen );

/**
 * \brief          Decode a base64-formatted buffer
 *
 * \param dst      destination buffer
 * \param dlen     size of the buffer
 * \param src      source buffer
 * \param slen     amount of data to be decoded
 *
 * \return         0 if successful, POLARSSL_ERR_BASE64_BUFFER_TOO_SMALL, or
 *                 POLARSSL_ERR_BASE64_INVALID_DATA if the input data is not
 *                 correct.
 *
 */
static int Decode( char *dst, int dlen,
                   const char *src, int  slen );

private:
  Base64Internals *Internals;
private:
  Base64(const Base64&);  // Not implemented.
  void operator=(const Base64&);  // Not implemented.
};
} // end namespace gdcm
//-----------------------------------------------------------------------------
#endif //__gdcmBase64_h
