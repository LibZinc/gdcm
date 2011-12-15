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
#include "gdcmCodeString.h"

namespace gdcm
{
  bool CodeString::IsValid() const {
    if( !Superclass::IsValid() ) return false;
    // Implementation specific:

/*
Uppercase
characters, �0�-
�9�, the SPACE
character, and
underscore �_�, of
the Default
Character
Repertoire
*/
    const_iterator it = begin();
    for( ; it != end(); ++it )
      {
      int c = *it;
      if( !isupper(c) && !isdigit(c) && c != ' ' && c != '_' )
        {
        // char dummy = c;
        return false;
        }
      }
    return true;
  }

}
