/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library
  Module:  $URL: https://gdcm.svn.sourceforge.net/svnroot/gdcm/tags/gdcm-2-0-12/Applications/Cxx/gdcmtar.cxx $

  Copyright (c) 2006-2009 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * tar is a UNIX program for archiving.
 * Two types of operations are possible: concatenate / extract
 * Thus the name of 'gdcmtar' to concatenate a list of 2D slices into a multi frames
 * and the other way around: extract 2D slices from a multi frames image
 * It also support the fake multi frame image (CSA MOSAIC)
 */

#include "gdcmReader.h"
#include "gdcmVersion.h"
#include "gdcmImageReader.h"
#include "gdcmDataElement.h"
#include "gdcmImageWriter.h"
#include "gdcmSplitMosaicFilter.h"
#include "gdcmFilenameGenerator.h"
#include "gdcmDirectionCosines.h"
#include "gdcmImageHelper.h"

#include <string>
#include <iostream>

#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <getopt.h>
#include <string.h>

void PrintVersion()
{
  std::cout << "gdcmtar: gdcm " << gdcm::Version::GetVersion() << " ";
  const char date[] = "$Date: 2009-05-19 15:59:08 +0200 (Tue, 19 May 2009) $";
  std::cout << date << std::endl;
}

void PrintHelp()
{
  PrintVersion();
  std::cout << "Usage: gdcmtar [OPTION] [FILE]" << std::endl;
  std::cout << "Concatenate/Extract DICOM files.\n";
  std::cout << "Parameter (required):" << std::endl;
  std::cout << "  -i --input     DICOM filename" << std::endl;
  std::cout << "  -o --output    DICOM filename" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -M --mosaic     Split SIEMENS Mosaic image into multiple frames." << std::endl;
  std::cout << "  -p --pattern    Specify trailing file pattern." << std::endl;
  std::cout << "General Options:" << std::endl;
  std::cout << "  -V --verbose    more verbose (warning+error)." << std::endl;
  std::cout << "  -W --warning    print warning info." << std::endl;
  std::cout << "  -D --debug      print debug info." << std::endl;
  std::cout << "  -E --error      print error info." << std::endl;
  std::cout << "  -h --help       print help." << std::endl;
  std::cout << "  -v --version    print version." << std::endl;
}


int main (int argc, char *argv[])
{
  int c;
  //int digit_optind = 0;

  std::string filename;
  std::string outfilename;
  std::string root;
  int mosaic = 0;

  int verbose = 0;
  int warning = 0;
  int debug = 0;
  int error = 0;
  int help = 0;
  int version = 0;

  std::string pattern;
  while (1) {
    //int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = {
        {"input", 1, 0, 0},
        {"output", 1, 0, 0},
        {"mosaic", 0, &mosaic, 1}, // split siemens mosaic into multiple frames
        {"pattern", 1, 0, 0},               // p

// General options !
        {"verbose", 0, &verbose, 1},
        {"warning", 0, &warning, 1},
        {"debug", 0, &debug, 1},
        {"error", 0, &error, 1},
        {"help", 0, &help, 1},
        {"version", 0, &version, 1},

        {0, 0, 0, 0}
    };

    c = getopt_long (argc, argv, "i:o:Mp:VWDEhv",
      long_options, &option_index);
    if (c == -1)
      {
      break;
      }

    switch (c)
      {
    case 0:
        {
        const char *s = long_options[option_index].name;
        printf ("option %s", s);
        if (optarg)
          {
          if( option_index == 0 ) /* input */
            {
            assert( strcmp(s, "input") == 0 );
            assert( filename.empty() );
            filename = optarg;
            }
          else if( option_index == 3 ) /* pattern */
            {
            assert( strcmp(s, "pattern") == 0 );
            assert( pattern.empty() );
            pattern = optarg;
            }
          printf (" with arg %s, index = %d", optarg, option_index);
          }
        printf ("\n");
        }
      break;

    case 'i':
      printf ("option i with value '%s'\n", optarg);
      assert( filename.empty() );
      filename = optarg;
      break;

    case 'o':
      assert( outfilename.empty() );
      outfilename = optarg;
      break;

    case 'M':
      //assert( outfilename.empty() );
      //outfilename = optarg;
      mosaic = 1;
      break;

    case 'p':
      assert( pattern.empty() );
      pattern = optarg;
      break;

    case 'V':
      verbose = 1;
      break;

    case 'W':
      warning = 1;
      break;

    case 'D':
      debug = 1;
      break;

    case 'E':
      error = 1;
      break;

    case 'h':
      help = 1;
      break;

    case 'v':
      version = 1;
      break;

    case '?':
      break;

    default:
      printf ("?? getopt returned character code 0%o ??\n", c);
      }
  }

  // For now only support one input / one output
  if (optind < argc)
    {
    //printf ("non-option ARGV-elements: ");
    //while (optind < argc)
    //  {
    //  printf ("%s ", argv[optind++]);
    //  }
    //printf ("\n");
    PrintHelp();
    return 1;
    }

  if( version )
    {
    //std::cout << "version" << std::endl;
    PrintVersion();
    return 0;
    }

  if( help )
    {
    //std::cout << "help" << std::endl;
    PrintHelp();
    return 0;
    }

  if( filename.empty() )
    {
    //std::cerr << "Need input file (-i)\n";
    PrintHelp();
    return 1;
    }
  if( outfilename.empty() )
    {
    //std::cerr << "Need output file (-o)\n";
    PrintHelp();
    return 1;
    }

  // Debug is a little too verbose
  gdcm::Trace::SetDebug( debug );
  gdcm::Trace::SetWarning( warning );
  gdcm::Trace::SetError( error );
  // when verbose is true, make sure warning+error are turned on:
  if( verbose )
    {
    gdcm::Trace::SetWarning( verbose );
    gdcm::Trace::SetError( verbose);
    }

  gdcm::FileMetaInformation::SetSourceApplicationEntityTitle( "gdcmtar" );

  if( mosaic )
    {
    gdcm::ImageReader reader;
    reader.SetFileName( filename.c_str() );
    if( !reader.Read() )
      {
      std::cerr << "could not read: " << filename << std::endl;
      return 1;
      }

    gdcm::SplitMosaicFilter filter;
    filter.SetImage( reader.GetImage() );
    filter.SetFile( reader.GetFile() );
    bool b = filter.Split();
    if( !b )
      {
      std::cerr << "Could not split << " << filename << std::endl;
      return 1;
      }

    const gdcm::Image &image = filter.GetImage();
    const unsigned int *dims = image.GetDimensions();
    const gdcm::DataElement &pixeldata = image.GetDataElement();
    const gdcm::ByteValue *bv = pixeldata.GetByteValue();
    unsigned long slice_len = image.GetBufferLength() / dims[2];
    //assert( image.GetBufferLength() == bv->GetLength() );

    gdcm::FilenameGenerator fg;
    fg.SetNumberOfFilenames( dims[2] );
    fg.SetPrefix( outfilename.c_str() );
    fg.SetPattern( pattern.c_str() );
    if( !fg.Generate() )
      {
      std::cerr << "could not generate" << std::endl;
      return 1;
      }
    const double *cosines = image.GetDirectionCosines();
    gdcm::DirectionCosines dc( cosines );
    double normal[3];
    dc.Cross( normal );
    const double *origin = image.GetOrigin();
    double zspacing = image.GetSpacing(2);

    for(unsigned int i = 0; i < dims[2]; ++i)
      {
      double new_origin[3];
      for (int j = 0; j < 3; j++)
        {
        // the n'th slice is n * z-spacing aloung the IOP-derived
        // z-axis
        new_origin[j] = origin[j] + normal[j] * i * zspacing;
        }

      const char *outfilenamei = fg.GetFilename(i);
      gdcm::ImageWriter writer;
      writer.SetFileName( outfilenamei );
      //writer.SetFile( filter.GetFile() );
      writer.SetFile( reader.GetFile() );

      //
      //writer.SetImage( filter.GetImage() );
      gdcm::Image &slice = writer.GetImage();
      slice = filter.GetImage();
      slice.SetOrigin( new_origin );
      slice.SetNumberOfDimensions( 2 );
      assert( slice.GetPixelFormat() == filter.GetImage().GetPixelFormat() );
      slice.SetSpacing(2, filter.GetImage().GetSpacing(2) );
      //slice.Print( std::cout );
      gdcm::DataElement &pd = slice.GetDataElement();
      const char *sliceptr = bv->GetPointer() + i * slice_len;
      pd.SetByteValue( sliceptr, slice_len);

      if( !writer.Write() )
        {
        std::cerr << "Failed to write: " << outfilename << std::endl;
        return 1;
        }
      }
    
    return 0;
    }
  else
    {
    std::cerr << "Not implemented for now" << std::endl;
    return 1;
    }


  return 0;
}

