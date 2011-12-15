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
#include "gdcmPixelFormat.h"
#include "gdcmTrace.h"
#include <stdlib.h>

namespace gdcm
{
static const char *ScalarTypeStrings[] = {
  "UINT8",
  "INT8",
  "UINT12",
  "INT12",
  "UINT16",
  "INT16",
  "UINT32",
  "INT32",
  NULL,
};

PixelFormat::PixelFormat(ScalarType st)
{
  SetScalarType( st );
}

unsigned short PixelFormat::GetSamplesPerPixel() const
{
  if ( BitsAllocated == 24 )
    {
    gdcmWarningMacro( "This is illegal in DICOM, assuming a RGB image" );
    assert(0);
    return 3;
    }
  // \postcondition
  assert( SamplesPerPixel == 1 || SamplesPerPixel == 3 || SamplesPerPixel == 4 );
  return SamplesPerPixel;
}

void PixelFormat::SetScalarType(ScalarType st)
{
  SamplesPerPixel = 1;
  switch(st)
    {
  case PixelFormat::UINT8:
    BitsAllocated = 8;
    PixelRepresentation = 0;
    break;
  case PixelFormat::INT8:
    BitsAllocated = 8;
    PixelRepresentation = 1;
    break;
  case PixelFormat::UINT12:
    BitsAllocated = 12;
    PixelRepresentation = 0;
    break;
  case PixelFormat::INT12:
    BitsAllocated = 12;
    PixelRepresentation = 1;
    break;
  case PixelFormat::UINT16:
    BitsAllocated = 16;
    PixelRepresentation = 0;
    break;
  case PixelFormat::INT16:
    BitsAllocated = 16;
    PixelRepresentation = 1;
    break;
  case PixelFormat::UINT32:
    BitsAllocated = 32;
    PixelRepresentation = 0;
    break;
  case PixelFormat::INT32:
    BitsAllocated = 32;
    PixelRepresentation = 1;
    break;
  case PixelFormat::FLOAT16:
    BitsAllocated = 16;
    // secret code:
    PixelRepresentation = 2;
    break;
  case PixelFormat::FLOAT32:
    BitsAllocated = 32;
    // secret code:
    PixelRepresentation = 3;
    break;
  case PixelFormat::FLOAT64:
    BitsAllocated = 64;
    // secret code:
    PixelRepresentation = 4;
    break;
  case PixelFormat::UNKNOWN:
    BitsAllocated = 0;
    PixelRepresentation = 0;
    break;
  default:
    assert(0);
    break;
    }
  BitsStored = BitsAllocated;
  HighBit = BitsStored - 1;
}

PixelFormat::ScalarType PixelFormat::GetScalarType() const
{
  ScalarType type = PixelFormat::UNKNOWN;
  switch( BitsAllocated )
    {
  case 0:
    type = PixelFormat::UNKNOWN;
    break;
  case 8:
    type = PixelFormat::UINT8;
    break;
  case 12:
    type = PixelFormat::UINT12;
    break;
  case 16:
    type = PixelFormat::UINT16;
    break;
  case 32:
    type = PixelFormat::UINT32;
    break;    
  case 64:
    type = PixelFormat::UINT32; // why not ?
    break;    
  case 24:
    gdcmWarningMacro( "This is illegal in DICOM, assuming a RGB image" );
    type = PixelFormat::UINT8;
    assert(0);
    break;
  default:
    gdcmErrorMacro( "I have never seen this before BitsAllocated "
      << BitsAllocated );
    type = PixelFormat::UNKNOWN;
    //assert(0);
    }
  if( PixelRepresentation == 0 )
    {
    // all set !
    }
  else if( PixelRepresentation == 1 )
    {
    assert( type <= INT32 );
    // That's why you need to order properly type in ScalarType
    type = ScalarType(int(type)+1);
    }
  else if( PixelRepresentation == 2 )
    {
    assert( BitsAllocated == 16 );
    return FLOAT16;
    }
  else if( PixelRepresentation == 3 )
    {
    assert( BitsAllocated == 32 );
    return FLOAT32;
    }
  else if( PixelRepresentation == 4 )
    {
    assert( BitsAllocated == 64 );
    return FLOAT64;
    }
  else
    {
    assert(0);
    }
  return type;
}

const char *PixelFormat::GetScalarTypeAsString() const
{
  return ScalarTypeStrings[GetScalarType()];
}

uint8_t PixelFormat::GetPixelSize() const
{
  uint8_t pixelsize = BitsAllocated / 8;
  if( BitsAllocated == 12 )
    {
    pixelsize = 2; // fake a short value
    }
  else
    assert( !(BitsAllocated % 8) );
  pixelsize *= SamplesPerPixel;

  return pixelsize;
}

int64_t PixelFormat::GetMin() const
{
  assert( BitsStored <= 32 );
  if( PixelRepresentation == 1 )
    {
    return (int64_t)(~(((1ull << BitsStored) - 1) >> 1));
    }
  else if( PixelRepresentation == 0 )
    {
    return 0;
    }
  //else if( PixelRepresentation == 3 ) // 32bits float
  //  {
  //  return (float) -1.0e+38f;
  //  }
  //else if( PixelRepresentation == 4 ) // 64bits float
  //  {
  //  return (double) -1.0e+299;
  //  }
  else
    {
    assert(0);
    }
  return 0;
}

int64_t PixelFormat::GetMax() const
{
  assert( BitsStored <= 32 );
  if( PixelRepresentation == 1 )
    {
    return (int64_t)(((1ull << BitsStored) - 1) >> 1);
    }
  else if( PixelRepresentation == 0 )
    {
    return (int64_t)((1ull << BitsStored) - 1);
    }
  //else if( PixelRepresentation == 3 ) // 32bits float
  //  {
  //  return (float)  1.0e+38f;
  //  }
  //else if( PixelRepresentation == 4 ) // 64bits float
  //  {
  //  return (double)  1.0e+299;
  //  }
  else
    {
    assert(0);
    }
  return 0;
}

bool PixelFormat::Validate()
{
  assert( BitsAllocated >= BitsStored );
  assert( BitsAllocated >= HighBit );
  //assert( BitsStored    >= HighBit ); // DigitexAlpha_no_7FE0.dcm
  assert( PixelRepresentation == 0 || PixelRepresentation == 1 );
  assert( SamplesPerPixel == 1 || SamplesPerPixel == 3 || SamplesPerPixel == 4 );
  if ( BitsAllocated == 24 )
    {
    assert( BitsStored == 24 && HighBit == 23 && SamplesPerPixel == 1 );
    BitsAllocated = 8;
    BitsStored = 8;
    HighBit = 7;
    SamplesPerPixel = 3;
    return false;
    }
  return true;
}

void PixelFormat::Print(std::ostream &os) const
{
  os << "SamplesPerPixel    :" << SamplesPerPixel     << "\n"; 
  os << "BitsAllocated      :" << BitsAllocated       << "\n";
  os << "BitsStored         :" << BitsStored          << "\n";
  os << "HighBit            :" << HighBit             << "\n";
  os << "PixelRepresentation:" << PixelRepresentation << "\n";
  os << "ScalarType found   :" << GetScalarTypeAsString() << "\n";
}

} // end namespace gdcm
