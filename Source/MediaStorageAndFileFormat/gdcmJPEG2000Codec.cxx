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
#include "gdcmJPEG2000Codec.h"
#include "gdcmTransferSyntax.h"
#include "gdcmTrace.h"
#include "gdcmDataElement.h"
#include "gdcmSequenceOfFragments.h"

#include "gdcm_openjpeg.h"

namespace gdcm
{

/**
sample error callback expecting a FILE* client object
*/
void error_callback(const char *msg, void *) {
  gdcmErrorMacro( "Error in gdcmopenjpeg" << msg );
}
/**
sample warning callback expecting a FILE* client object
*/
void warning_callback(const char *msg, void *) {
  gdcmWarningMacro( "Warning in gdcmopenjpeg" << msg );
}
/**
sample debug callback expecting no client object
*/
void info_callback(const char *msg, void *) {
  gdcmDebugMacro( "Info in gdcmopenjpeg" << msg );
}

#define J2K_CFMT 0
#define JP2_CFMT 1
#define JPT_CFMT 2
#define MJ2_CFMT 3
#define PXM_DFMT 0
#define PGX_DFMT 1
#define BMP_DFMT 2
#define YUV_DFMT 3

/*
 * Divide an integer by a power of 2 and round upwards.
 *
 * a divided by 2^b
 */
inline int int_ceildivpow2(int a, int b) {
  return (a + (1 << b) - 1) >> b;
}

class JPEG2000Internals
{
public:
  JPEG2000Internals()
    {
    memset(&coder_param, 0, sizeof(coder_param));
    opj_set_default_encoder_parameters(&coder_param);
    }

  opj_cparameters coder_param;
};

void JPEG2000Codec::SetRate(unsigned int idx, double rate)
{
  Internals->coder_param.tcp_rates[idx] = rate;
  if( Internals->coder_param.tcp_numlayers <= idx )
    {
    Internals->coder_param.tcp_numlayers = idx + 1;
    }
  Internals->coder_param.cp_disto_alloc = 1;
}

double JPEG2000Codec::GetRate(unsigned int idx ) const
{
  return Internals->coder_param.tcp_rates[idx];
}

void JPEG2000Codec::SetQuality(unsigned int idx, double q)
{
  Internals->coder_param.tcp_distoratio[idx] = q;
  if( Internals->coder_param.tcp_numlayers <= idx )
    {
    Internals->coder_param.tcp_numlayers = idx + 1;
    }
  Internals->coder_param.cp_fixed_quality = 1;
}

double JPEG2000Codec::GetQuality(unsigned int idx) const
{
  return Internals->coder_param.tcp_distoratio[idx];
}

void JPEG2000Codec::SetTileSize(unsigned int tx, unsigned int ty)
{
  Internals->coder_param.cp_tdx = tx;
  Internals->coder_param.cp_tdy = ty;
  Internals->coder_param.tile_size_on = true;
}

void JPEG2000Codec::SetNumberOfResolutions(unsigned int nres)
{
  Internals->coder_param.numresolution = nres;
}

void JPEG2000Codec::SetReversible(bool res)
{
  Internals->coder_param.irreversible = !res;
}

JPEG2000Codec::JPEG2000Codec()
{
  Internals = new JPEG2000Internals;
}

JPEG2000Codec::~JPEG2000Codec()
{
  delete Internals;
}

bool JPEG2000Codec::CanDecode(TransferSyntax const &ts) const
{
  return ts == TransferSyntax::JPEG2000Lossless 
      || ts == TransferSyntax::JPEG2000;
}

bool JPEG2000Codec::CanCode(TransferSyntax const &ts) const
{
  return ts == TransferSyntax::JPEG2000Lossless 
      || ts == TransferSyntax::JPEG2000;
}

/*
A.4.4 JPEG 2000 image compression

  If the object allows multi-frame images in the pixel data field, then for these JPEG 2000 Part 1 Transfer
  Syntaxes, each frame shall be encoded separately. Each fragment shall contain encoded data from a
  single frame.
  Note: That is, the processes defined in ISO/IEC 15444-1 shall be applied on a per-frame basis. The proposal
  for encapsulation of multiple frames in a non-DICOM manner in so-called �Motion-JPEG� or �M-JPEG�
  defined in 15444-3 is not used.
*/
bool JPEG2000Codec::Decode(DataElement const &in, DataElement &out)
{
  if( NumberOfDimensions == 2 )
    {
    const SequenceOfFragments *sf = in.GetSequenceOfFragments();
    assert( sf );
    std::stringstream is;
    unsigned long totalLen = sf->ComputeByteLength();
    char *buffer = new char[totalLen];
    sf->GetBuffer(buffer, totalLen);
    is.write(buffer, totalLen);
    delete[] buffer;
    std::stringstream os;
    bool r = Decode(is, os);
    if(!r) return false;
    out = in;
    std::string str = os.str();
    out.SetByteValue( &str[0], str.size() );
    //memcpy(buffer, os.str().c_str(), len);
    return r;
    }
  else if ( NumberOfDimensions == 3 )
    {
    /* I cannot figure out how to use openjpeg to support multiframes
     * as encoded in DICOM
     * MM: Hack. If we are lucky enough the number of encapsulated fragments actually match
     * the number of Z frames.
     * MM: hopefully this is the standard so people are following it ...
     */
    //#ifdef SUPPORT_MULTIFRAMESJ2K_ONLY
    const SequenceOfFragments *sf = in.GetSequenceOfFragments();
    assert( sf );
    std::stringstream os;
    assert( sf->GetNumberOfFragments() == Dimensions[2] );
    for(unsigned int i = 0; i < sf->GetNumberOfFragments(); ++i)
      {
      std::stringstream is;
      const Fragment &frag = sf->GetFragment(i);
      if( frag.IsEmpty() ) return false;
      const ByteValue *bv = frag.GetByteValue();
      assert( bv );
      char *mybuffer = new char[bv->GetLength()];
      bv->GetBuffer(mybuffer, bv->GetLength());
      is.write(mybuffer, bv->GetLength());
      delete[] mybuffer;
      bool r = Decode(is, os);
      if(!r) return false;
      assert( r == true );
      }
    std::string str = os.str();
    assert( str.size() );
    out.SetByteValue( &str[0], str.size() );

    return true;
    }
  // else
  return false;
}

bool JPEG2000Codec::Decode(std::istream &is, std::ostream &os)
{
  opj_dparameters_t parameters;  /* decompression parameters */
  opj_event_mgr_t event_mgr;    /* event manager */
  opj_image_t *image;
  opj_dinfo_t* dinfo;  /* handle to a decompressor */
  opj_cio_t *cio;
  // FIXME: Do some stupid work:
  is.seekg( 0, std::ios::end);
  std::streampos buf_size = is.tellg();
  char *dummy_buffer = new char[buf_size];
  is.seekg(0, std::ios::beg);
  is.read( dummy_buffer, buf_size);
  unsigned char *src = (unsigned char*)dummy_buffer;
  uint32_t file_length = buf_size; // 32bits truncation should be ok since DICOM cannot have larger than 2Gb image

  // WARNING: OpenJPEG is very picky when there is a trailing 00 at the end of the JPC
  // so we need to make sure to remove it:
  // See for example: DX_J2K_0Padding.dcm
  //             and D_CLUNIE_CT1_J2KR.dcm
    //  Marker 0xffd9 EOI End of Image (JPEG 2000 EOC End of codestream)
    // gdcmData/D_CLUNIE_CT1_J2KR.dcm contains a trailing 0xFF which apparently is ok...
  while( file_length > 0 && src[file_length-1] != 0xd9 )
    {
    file_length--;
    }
  // what if 0xd9 is never found ?
  assert( file_length > 0 && src[file_length-1] == 0xd9 );

  /* configure the event callbacks (not required) */
  memset(&event_mgr, 0, sizeof(opj_event_mgr_t));
  event_mgr.error_handler = error_callback;
  event_mgr.warning_handler = warning_callback;
  event_mgr.info_handler = info_callback;

  /* set decoding parameters to default values */
  opj_set_default_decoder_parameters(&parameters);

  // default blindly copied
  parameters.cp_layer=0;
  parameters.cp_reduce=0;
  //   parameters.decod_format=-1;
  //   parameters.cod_format=-1;

  const char jp2magic[] = "\x00\x00\x00\x0C\x6A\x50\x20\x20\x0D\x0A\x87\x0A";
  if( memcmp( src, jp2magic, sizeof(jp2magic) ) == 0 )
    {
    /* JPEG-2000 compressed image data ... sigh */
    // gdcmData/ELSCINT1_JP2vsJ2K.dcm
    // gdcmData/MAROTECH_CT_JP2Lossy.dcm
    gdcmWarningMacro( "J2K start like JPEG-2000 compressed image data instead of codestream" );
    parameters.decod_format = JP2_CFMT;
    assert(parameters.decod_format == JP2_CFMT);
    }
  else
    {
    /* JPEG-2000 codestream */
    parameters.decod_format = J2K_CFMT;
    assert(parameters.decod_format == J2K_CFMT);
    }
  parameters.cod_format = PGX_DFMT;
  assert(parameters.cod_format == PGX_DFMT);

  /* get a decoder handle */
  switch(parameters.decod_format)
    {
  case J2K_CFMT:
    dinfo = opj_create_decompress(CODEC_J2K);
    break;
  case JP2_CFMT:
    dinfo = opj_create_decompress(CODEC_JP2);
    break;
  default:
    gdcmErrorMacro( "Impossible happen" );
    return false;
    }

  /* catch events using our callbacks and give a local context */
  opj_set_event_mgr((opj_common_ptr)dinfo, &event_mgr, NULL);

  /* setup the decoder decoding parameters using user parameters */
  opj_setup_decoder(dinfo, &parameters);

  /* open a byte stream */
  cio = opj_cio_open((opj_common_ptr)dinfo, src, file_length);

  /* decode the stream and fill the image structure */
  image = opj_decode(dinfo, cio);
  if(!image) {
    opj_destroy_decompress(dinfo);
    opj_cio_close(cio);
    gdcmErrorMacro( "opj_decode failed" );
    return false;
  }

#if 0
  if( image->color_space )
    {
    if( image->color_space == CLRSPC_GRAY )
      {
      assert( this->GetPhotometricInterpretation() == PhotometricInterpretation::MONOCHROME2 
        || this->GetPhotometricInterpretation() == PhotometricInterpretation::MONOCHROME1
        || this->GetPhotometricInterpretation() == PhotometricInterpretation::PALETTE_COLOR );
      }
    else if( image->color_space == CLRSPC_SRGB )
      {
      assert( this->GetPhotometricInterpretation() == PhotometricInterpretation::RGB );
      }
    else
      {
      assert(0);
      }
    }
#endif
  
  int reversible;
  opj_j2k_t* j2k = NULL;
  opj_jp2_t* jp2 = NULL;

  switch(parameters.decod_format)
    {
  case J2K_CFMT:
    j2k = (opj_j2k_t*)dinfo->j2k_handle;
    assert( j2k );
    reversible = j2k->cp->tcps->tccps->qmfbid;
    break;
  case JP2_CFMT:
    jp2 = (opj_jp2_t*)dinfo->jp2_handle;
    assert( jp2 );
    reversible = jp2->j2k->cp->tcps->tccps->qmfbid;
    break;
  default:
    gdcmErrorMacro( "Impossible happen" );
    return false;
    }
  LossyFlag = !reversible;
#if 0
#ifndef GDCM_USE_SYSTEM_OPENJPEG
  if( j2k )
    j2k_dump_cp(stdout, image, j2k->cp);
  if( jp2 )
    j2k_dump_cp(stdout, image, jp2->j2k->cp);
#endif
#endif

  assert( image->numcomps == this->GetPixelFormat().GetSamplesPerPixel() );
  assert( image->numcomps == this->GetPhotometricInterpretation().GetSamplesPerPixel() );


  /* close the byte stream */
  opj_cio_close(cio);

  // Copy buffer
  unsigned long len = Dimensions[0]*Dimensions[1] * (PF.GetBitsAllocated() / 8) * image->numcomps;
  char *raw = new char[len];
  for (int compno = 0; compno < image->numcomps; compno++)
    {
    opj_image_comp_t *comp = &image->comps[compno];

    int w = image->comps[compno].w;
    int wr = int_ceildivpow2(image->comps[compno].w, image->comps[compno].factor);

    //int h = image.comps[compno].h;
    int hr = int_ceildivpow2(image->comps[compno].h, image->comps[compno].factor);
    //assert(  wr * hr * 1 * image->numcomps * (comp->prec/8) == len );

    // ELSCINT1_JP2vsJ2K.dcm
    // -> prec = 12, bpp = 0, sgnd = 0
    //assert( wr == Dimensions[0] );
    //assert( hr == Dimensions[1] );
    if( comp->bpp == PF.GetBitsAllocated() )
      {
      gdcmWarningMacro( "BPP = " << comp->bpp << " vs BitsAllocated = " << PF.GetBitsAllocated() );
      }
    assert( comp->sgnd == PF.GetPixelRepresentation() );
    assert( comp->prec == PF.GetBitsStored());
    assert( comp->prec - 1 == PF.GetHighBit());
    assert( comp->prec <= 32 );

    if (comp->prec <= 8)
      {
      uint8_t *data8 = (uint8_t*)raw + compno;
      for (int i = 0; i < wr * hr; i++) 
        {
        int v = image->comps[compno].data[i / wr * w + i % wr];
        *data8 = (uint8_t)v;
        data8 += image->numcomps;
        }
      }
    else if (comp->prec <= 16)
      {
      // ELSCINT1_JP2vsJ2K.dcm is a 12bits image
      uint16_t *data16 = (uint16_t*)raw + compno;
      for (int i = 0; i < wr * hr; i++) 
        {
        int v = image->comps[compno].data[i / wr * w + i % wr];
        *data16 = (uint16_t)v;
        data16 += image->numcomps;
        }
      }
    else
      {
      uint32_t *data32 = (uint32_t*)raw + compno;
      for (int i = 0; i < wr * hr; i++) 
        {
        int v = image->comps[compno].data[i / wr * w + i % wr];
        *data32 = (uint32_t)v;
        data32 += image->numcomps;
        }
      }
    }
  os.write(raw, len );
  delete[] raw;
  /* free the memory containing the code-stream */
  delete[] src;  //FIXME

  /* free remaining structures */
  if(dinfo) {
    opj_destroy_decompress(dinfo);
  }

  /* free image data structure */
  opj_image_destroy(image);

  return true;
}

template<typename T>
void rawtoimage_fill(T *inputbuffer, int w, int h, int numcomps, opj_image_t *image, int pc)
{
  T *p = inputbuffer;
  if( pc )
    {
    for(int compno = 0; compno < numcomps; compno++)
      {
      for (int i = 0; i < w * h; i++)
        {
        /* compno : 0 = GREY, (0, 1, 2) = (R, G, B) */
        image->comps[compno].data[i] = *p;
        ++p;
        }
      }
    }
  else
    {
    for (int i = 0; i < w * h; i++)
      {
      for(int compno = 0; compno < numcomps; compno++)
        {
        /* compno : 0 = GREY, (0, 1, 2) = (R, G, B) */
        image->comps[compno].data[i] = *p;
        ++p;
        }
      }
    }
}

opj_image_t* rawtoimage(char *inputbuffer, opj_cparameters_t *parameters,
  int fragment_size, int image_width, int image_height, int sample_pixel,
  int bitsallocated, int bitsstored, int sign, int quality, int pc)
{
  (void)quality;
  int w, h;
  int numcomps;
  OPJ_COLOR_SPACE color_space;
  opj_image_cmptparm_t cmptparm[3]; /* maximum of 3 components */
  opj_image_t * image = NULL;

  assert( sample_pixel == 1 || sample_pixel == 3 );
  if( sample_pixel == 1 )
    {
    numcomps = 1;
    color_space = CLRSPC_GRAY;
    }
  else // sample_pixel == 3
    {
    numcomps = 3;
    color_space = CLRSPC_SRGB;
    /* Does OpenJPEg support: CLRSPC_SYCC ?? */
    }
  if( bitsallocated % 8 != 0 )
    {
    return 0;
    }
  assert( bitsallocated % 8 == 0 );
  // eg. fragment_size == 63532 and 181 * 117 * 3 * 8 == 63531 ...
  assert( ((fragment_size + 1)/2 ) * 2 == ((image_height * image_width * numcomps * (bitsallocated/8) + 1)/ 2 )* 2 );
  int subsampling_dx = parameters->subsampling_dx;
  int subsampling_dy = parameters->subsampling_dy;

  // FIXME
  w = image_width;
  h = image_height;

  /* initialize image components */
  memset(&cmptparm[0], 0, 3 * sizeof(opj_image_cmptparm_t));
  //assert( bitsallocated == 8 );
  for(int i = 0; i < numcomps; i++) {
    cmptparm[i].prec = bitsstored;
    cmptparm[i].bpp = bitsallocated;
    cmptparm[i].sgnd = sign;
    cmptparm[i].dx = subsampling_dx;
    cmptparm[i].dy = subsampling_dy;
    cmptparm[i].w = w;
    cmptparm[i].h = h;
  }

  /* create the image */
  image = opj_image_create(numcomps, &cmptparm[0], color_space);
  if(!image) {
    return NULL;
  }
  /* set image offset and reference grid */
  image->x0 = parameters->image_offset_x0;
  image->y0 = parameters->image_offset_y0;
  image->x1 = parameters->image_offset_x0 + (w - 1) * subsampling_dx + 1;
  image->y1 = parameters->image_offset_y0 + (h - 1) * subsampling_dy + 1;

  /* set image data */

  //assert( fragment_size == numcomps*w*h*(bitsallocated/8) );
  if (bitsallocated <= 8)
    {
    if( sign )
      {
      rawtoimage_fill<int8_t>((int8_t*)inputbuffer,w,h,numcomps,image,pc);
      }
    else
      {
      rawtoimage_fill<uint8_t>((uint8_t*)inputbuffer,w,h,numcomps,image,pc);
      }
    }
  else if (bitsallocated <= 16)
    {
    if( sign )
      {
      rawtoimage_fill<int16_t>((int16_t*)inputbuffer,w,h,numcomps,image,pc);
      }
    else
      {
      rawtoimage_fill<uint16_t>((uint16_t*)inputbuffer,w,h,numcomps,image,pc);
      }
    }
  else if (bitsallocated <= 32)
    {
    if( sign )
      {
      rawtoimage_fill<int32_t>((int32_t*)inputbuffer,w,h,numcomps,image,pc);
      }
    else
      {
      rawtoimage_fill<uint32_t>((uint32_t*)inputbuffer,w,h,numcomps,image,pc);
      }
    }
  else
    {
    return NULL;
    }

  return image;
}

  // Compress into JPEG
bool JPEG2000Codec::Code(DataElement const &in, DataElement &out)
{
  out = in;
  //
  // Create a Sequence Of Fragments:
  SmartPointer<SequenceOfFragments> sq = new SequenceOfFragments;
  const Tag itemStart(0xfffe, 0xe000);
  //sq->GetTable().SetTag( itemStart );

  const unsigned int *dims = this->GetDimensions();

  const ByteValue *bv = in.GetByteValue();
  const char *input = bv->GetPointer();
  unsigned long len = bv->GetLength();
  unsigned long image_len = len / dims[2];
  size_t inputlength = image_len;

  for(unsigned int dim = 0; dim < dims[2]; ++dim)
    {

    std::ostringstream os;
    std::ostream *fp = &os;
    const char *inputdata = input + dim * image_len; //bv->GetPointer();
    //size_t inputlength = bv->GetLength();
    int image_width = dims[0];
    int image_height = dims[1];
    int numZ = 0; //dims[2];
    const PixelFormat &pf = this->GetPixelFormat();
    int sample_pixel = pf.GetSamplesPerPixel();
    int bitsallocated = pf.GetBitsAllocated();
    int bitsstored = pf.GetBitsStored();
    int sign = pf.GetPixelRepresentation();
    int quality = 100;

    //// input_buffer is ONE image
    //// fragment_size is the size of this image (fragment)
    (void)numZ;
    bool bSuccess;
    //bool delete_comment = true;
    opj_cparameters_t parameters;  /* compression parameters */
    opj_event_mgr_t event_mgr;    /* event manager */
    opj_image_t *image = NULL;
    //quality = 100;

    /*
    configure the event callbacks (not required)
    setting of each callback is optionnal
    */
    memset(&event_mgr, 0, sizeof(opj_event_mgr_t));
    event_mgr.error_handler = error_callback;
    event_mgr.warning_handler = warning_callback;
    event_mgr.info_handler = info_callback;

    /* set encoding parameters to default values */
    //memset(&parameters, 0, sizeof(parameters));
    //opj_set_default_encoder_parameters(&parameters);

    memcpy(&parameters, &(Internals->coder_param), sizeof(parameters));

    if ((parameters.cp_disto_alloc || parameters.cp_fixed_alloc || parameters.cp_fixed_quality)
      && (!(parameters.cp_disto_alloc ^ parameters.cp_fixed_alloc ^ parameters.cp_fixed_quality)))
      {
      gdcmErrorMacro( "Error: options -r -q and -f cannot be used together." );
      return false;
      }				/* mod fixed_quality */

    /* if no rate entered, lossless by default */
    if (parameters.tcp_numlayers == 0) 
      {
      parameters.tcp_rates[0] = 0;
      parameters.tcp_numlayers = 1;
      parameters.cp_disto_alloc = 1;
      }

    if(parameters.cp_comment == NULL) {
      const char comment[] = "Created by GDCM/OpenJPEG version 1.0";
      parameters.cp_comment = (char*)malloc(strlen(comment) + 1);
      strcpy(parameters.cp_comment, comment);
      /* no need to delete parameters.cp_comment on exit */
      //delete_comment = false;
    }


    /* decode the source image */
    /* ----------------------- */

    image = rawtoimage((char*)inputdata, &parameters, 
      static_cast<int>( inputlength ), 
      image_width, image_height,
      sample_pixel, bitsallocated, bitsstored, sign, quality, this->GetPlanarConfiguration() );
    if (!image) {
      return false;
    }

    /* encode the destination image */
    /* ---------------------------- */
    parameters.cod_format = J2K_CFMT; /* J2K format output */
    int codestream_length;
    opj_cio_t *cio = NULL;
    //FILE *f = NULL;

    /* get a J2K compressor handle */
    opj_cinfo_t* cinfo = opj_create_compress(CODEC_J2K);

    /* catch events using our callbacks and give a local context */
    opj_set_event_mgr((opj_common_ptr)cinfo, &event_mgr, stderr);

    /* setup the encoder parameters using the current image and using user parameters */
    opj_setup_encoder(cinfo, &parameters, image);

    /* open a byte stream for writing */
    /* allocate memory for all tiles */
    cio = opj_cio_open((opj_common_ptr)cinfo, NULL, 0);

    /* encode the image */
    bSuccess = opj_encode(cinfo, cio, image, parameters.index);
    if (!bSuccess) {
      opj_cio_close(cio);
      fprintf(stderr, "failed to encode image\n");
      return false;
    }
    codestream_length = cio_tell(cio);

    /* write the buffer to disk */
    //f = fopen(parameters.outfile, "wb");
    //if (!f) {
    //  fprintf(stderr, "failed to open %s for writing\n", parameters.outfile);
    //  return 1;
    //}
    //fwrite(cio->buffer, 1, codestream_length, f);
    //#define MDEBUG
#ifdef MDEBUG
    static int c = 0;
    std::ostringstream os;
    os << "/tmp/debug";
    os << c;
    c++;
    os << ".j2k";
    std::ofstream debug(os.str().c_str());
    debug.write((char*)(cio->buffer), codestream_length);
    debug.close();
#endif
    fp->write((char*)(cio->buffer), codestream_length);
    //fclose(f);

    /* close and free the byte stream */
    opj_cio_close(cio);

    /* free remaining compression structures */
    opj_destroy_compress(cinfo);


    /* free user parameters structure */
    //if(delete_comment) {
    if(parameters.cp_comment) free(parameters.cp_comment);
    //}
    if(parameters.cp_matrice) free(parameters.cp_matrice);

    /* free image data */
    opj_image_destroy(image);



    std::string str = os.str();
    assert( str.size() );
    Fragment frag;
    //frag.SetTag( itemStart );
    frag.SetByteValue( &str[0], str.size() );
    sq->AddFragment( frag );
    }

  //unsigned int nfrags = sq->GetNumberOfFragments();
  assert( sq->GetNumberOfFragments() == dims[2] );
  out.SetValue( *sq );

  return true;
}

bool JPEG2000Codec::GetHeaderInfo(std::istream &is, TransferSyntax &ts)
{
  // FIXME: Do some stupid work:
  is.seekg( 0, std::ios::end);
  std::streampos buf_size = is.tellg();
  char *dummy_buffer = new char[buf_size];
  is.seekg(0, std::ios::beg);
  is.read( dummy_buffer, buf_size);
  bool b = GetHeaderInfo( dummy_buffer, buf_size, ts );
  delete[] dummy_buffer;
  return b;
}

bool JPEG2000Codec::GetHeaderInfo(const char * dummy_buffer, size_t buf_size, TransferSyntax &ts)
{
  opj_dparameters_t parameters;  /* decompression parameters */
  opj_event_mgr_t event_mgr;    /* event manager */
  opj_image_t *image;
  opj_dinfo_t* dinfo;  /* handle to a decompressor */
  opj_cio_t *cio;
  unsigned char *src = (unsigned char*)dummy_buffer;
  int file_length = buf_size;

  /* configure the event callbacks (not required) */
  memset(&event_mgr, 0, sizeof(opj_event_mgr_t));
  event_mgr.error_handler = error_callback;
  event_mgr.warning_handler = warning_callback;
  event_mgr.info_handler = info_callback;

  /* set decoding parameters to default values */
  opj_set_default_decoder_parameters(&parameters);

  // default blindly copied
  parameters.cp_layer=0;
  parameters.cp_reduce=0;
  //   parameters.decod_format=-1;
  //   parameters.cod_format=-1;

  const char jp2magic[] = "\x00\x00\x00\x0C\x6A\x50\x20\x20\x0D\x0A\x87\x0A";
  if( memcmp( src, jp2magic, sizeof(jp2magic) ) == 0 )
    {
    /* JPEG-2000 compressed image data */
    // gdcmData/ELSCINT1_JP2vsJ2K.dcm
    gdcmWarningMacro( "J2K start like JPEG-2000 compressed image data instead of codestream" );
    parameters.decod_format = JP2_CFMT;
    assert(parameters.decod_format == JP2_CFMT);
    }
  else
    {
    /* JPEG-2000 codestream */
    parameters.decod_format = J2K_CFMT;
    assert(parameters.decod_format == J2K_CFMT);
    }
  parameters.cod_format = PGX_DFMT;
  assert(parameters.cod_format == PGX_DFMT);

  /* get a decoder handle */
  switch(parameters.decod_format )
    {
  case J2K_CFMT:
    dinfo = opj_create_decompress(CODEC_J2K);
    break;
  case JP2_CFMT:
    dinfo = opj_create_decompress(CODEC_JP2);
    break;
  default:
    gdcmErrorMacro( "Impossible happen" );
    return false;
    }

  /* catch events using our callbacks and give a local context */
  opj_set_event_mgr((opj_common_ptr)dinfo, &event_mgr, NULL);      

  /* setup the decoder decoding parameters using user parameters */
  opj_setup_decoder(dinfo, &parameters);

  /* open a byte stream */
  cio = opj_cio_open((opj_common_ptr)dinfo, src, file_length);

  /* decode the stream and fill the image structure */
  image = opj_decode(dinfo, cio);
  if(!image) {
    opj_destroy_decompress(dinfo);
    opj_cio_close(cio);
    gdcmErrorMacro( "opj_decode failed" );
    return false;
  }

  int reversible;
  opj_j2k_t* j2k = NULL;
  opj_jp2_t* jp2 = NULL;

  switch(parameters.decod_format)
    {
  case J2K_CFMT:
    j2k = (opj_j2k_t*)dinfo->j2k_handle;
    assert( j2k );
    reversible = j2k->cp->tcps->tccps->qmfbid;
    break;
  case JP2_CFMT:
    jp2 = (opj_jp2_t*)dinfo->jp2_handle;
    assert( jp2 );
    reversible = jp2->j2k->cp->tcps->tccps->qmfbid;
    break;
  default:
    gdcmErrorMacro( "Impossible happen" );
    return false;
    }
#if 0
#ifndef GDCM_USE_SYSTEM_OPENJPEG
  if( j2k )
    j2k_dump_cp(stdout, image, j2k->cp);
  if( jp2 )
    j2k_dump_cp(stdout, image, jp2->j2k->cp);
#endif
#endif

  int compno = 0;
  opj_image_comp_t *comp = &image->comps[compno];

  if( image->numcomps == 3 )
    {
    opj_image_comp_t *comp1 = &image->comps[1];
    opj_image_comp_t *comp2 = &image->comps[2];
    bool invalid = false;
    if( comp->bpp  != comp1->bpp  ) invalid = true;
    if( comp->bpp  != comp2->bpp  ) invalid = true;
    if( comp->prec != comp1->prec ) invalid = true;
    if( comp->prec != comp2->prec ) invalid = true;
    if( comp->sgnd != comp1->sgnd ) invalid = true;
    if( comp->sgnd != comp2->sgnd ) invalid = true;
    if( invalid )
      {
      gdcmErrorMacro( "Invalid test failed" );
      return false;
      }
    }

  this->Dimensions[0] = comp->w;
  this->Dimensions[1] = comp->h;

  if( comp->prec <= 8 )
    {
    if( comp->bpp ) assert( comp->bpp == 8 );
    this->PF = PixelFormat( PixelFormat::UINT8 );
    }
  else if( comp->prec <= 16 )
    {
    if( comp->bpp ) assert( comp->bpp == 16 );
    this->PF = PixelFormat( PixelFormat::UINT16 );
    }
  else if( comp->prec <= 32 )
    {
    if( comp->bpp ) assert( comp->bpp == 32 );
    this->PF = PixelFormat( PixelFormat::UINT32 );
    }
  else
    {
    gdcmErrorMacro( "do not handle precision: " << comp->prec );
    return false;
    }
  this->PF.SetBitsStored( comp->prec );
  this->PF.SetHighBit( comp->prec - 1 );
  this->PF.SetPixelRepresentation( comp->sgnd );

  if( image->numcomps == 1 )
    {
    assert( image->color_space == 0 );
    PI = PhotometricInterpretation::MONOCHROME2;
    this->PF.SetSamplesPerPixel( 1 );
    }
  else if( image->numcomps == 3 )
    {
    assert( image->color_space == 0 );
    //PI = PhotometricInterpretation::RGB;
    /*
    8.2.4 JPEG 2000 IMAGE COMPRESSION
    The JPEG 2000 bit stream specifies whether or not a reversible or irreversible multi-component (color)
    transformation, if any, has been applied. If no multi-component transformation has been applied, then the
    components shall correspond to those specified by the DICOM Attribute Photometric Interpretation
    (0028,0004). If the JPEG 2000 Part 1 reversible multi-component transformation has been applied then
    the DICOM Attribute Photometric Interpretation (0028,0004) shall be YBR_RCT. If the JPEG 2000 Part 1
    irreversible multi-component transformation has been applied then the DICOM Attribute Photometric
    Interpretation (0028,0004) shall be YBR_ICT.
    Notes: 1. For example, single component may be present, and the Photometric Interpretation (0028,0004) may
    be MONOCHROME2.
    2. Though it would be unusual, would not take advantage of correlation between the red, green and blue
    components, and would not achieve effective compression, a Photometric Interpretation of RGB could
    be specified as long as no multi-component transformation was specified by the JPEG 2000 bit stream.
    3. Despite the application of a multi-component color transformation and its reflection in the Photometric
    Interpretation attribute, the �color space� remains undefined. There is currently no means of conveying
    �standard color spaces� either by fixed values (such as sRGB) or by ICC profiles. Note in particular that
    the JP2 file header is not sent in the JPEG 2000 bitstream that is encapsulated in DICOM.
     */
    PI = PhotometricInterpretation::YBR_RCT;
    this->PF.SetSamplesPerPixel( 3 );
    }
  else if( image->numcomps == 4 )
    {
    /* Yes this is legal */
    // http://www.crc.ricoh.com/~gormish/jpeg2000conformance/
    // jpeg2000testimages/Part4TestStreams/codestreams_profile0/p0_06.j2k
    gdcmErrorMacro( "Image is 4 components which is not supported anymore in DICOM (ARGB is retired)" );
    // TODO: How about I get the 3 comps and set the alpha plane in the overlay ?
    return false;
    }
  else
    {
    // jpeg2000testimages/Part4TestStreams/codestreams_profile0/p0_13.j2k
    gdcmErrorMacro( "Image is " << image->numcomps << " components which is not supported in DICOM" );
    return false;
    }

  assert( PI != PhotometricInterpretation::UNKNOW );

  if( reversible )
    {
    ts = TransferSyntax::JPEG2000Lossless;
    }
  else
    {
    ts = TransferSyntax::JPEG2000;
    if( PI == PhotometricInterpretation::YBR_RCT )
      {
      // FIXME ???
      PI = PhotometricInterpretation::YBR_ICT;
      }
    }

  //assert( ts.IsLossy() == this->GetPhotometricInterpretation().IsLossy() );
  //assert( ts.IsLossless() == this->GetPhotometricInterpretation().IsLossless() );
  if( this->GetPhotometricInterpretation().IsLossy() )
    {
    assert( ts.IsLossy() );
    }
  if( ts.IsLossless() && !ts.IsLossy() )
    {
    assert( this->GetPhotometricInterpretation().IsLossless() );
    }

  /* close the byte stream */
  opj_cio_close(cio);

  /* free the memory containing the code-stream */
  //delete[] src;  //FIXME

  /* free remaining structures */
  if(dinfo) {
    opj_destroy_decompress(dinfo);
  }

  /* free image data structure */
  opj_image_destroy(image);


  return true;
}

} // end namespace gdcm
