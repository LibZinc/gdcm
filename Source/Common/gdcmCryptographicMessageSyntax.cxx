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
#include "gdcmCryptographicMessageSyntax.h"

#include <stdio.h> // stderr
#include <string.h> // strcmp
#include <assert.h>
#include <time.h> // time()

#ifdef GDCM_USE_SYSTEM_OPENSSL
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#endif

/*
 */
namespace gdcm
{

/*
 * openssl genrsa -out CA_key.pem 2048 
 *
 * openssl req -new -key CA_key.pem -x509 -days 365 -out CA_cert.cer
 */
class CryptographicMessageSyntaxInternals
{
#ifdef GDCM_USE_SYSTEM_OPENSSL
public:
  CryptographicMessageSyntaxInternals():recips(NULL),pkey(NULL),CipherType( CryptographicMessageSyntax::AES256_CIPHER ) {
    recips = sk_X509_new_null();
  }
  ~CryptographicMessageSyntaxInternals() {
    sk_X509_pop_free(recips, X509_free);
    EVP_PKEY_free(pkey);
  }
  unsigned int GetNumberOfRecipients() const {
    //::STACK_OF(X509) *recips = recips;
    if(!recips) {
      return 0;
    }
    return ::sk_X509_num(recips);
    }
  ::STACK_OF(X509)* GetRecipients( ) const {
    return recips;
  }
  ::X509* GetRecipient( unsigned int i ) const {
    //::STACK_OF(X509) *recips = Internals->recips;
    ::X509 *ret = sk_X509_value(recips, i);
    return ret;
  }
  void SetPrivateKey(::EVP_PKEY* pkey) {
    this->pkey = pkey;
  }
  ::EVP_PKEY* GetPrivateKey() const {
    return pkey;
  }
  void SetCipherType(CryptographicMessageSyntax::CipherTypes ciphertype) {
    CipherType = ciphertype;
  }
  CryptographicMessageSyntax::CipherTypes GetCipherType() {
    return CipherType;
  }
private:
  ::STACK_OF(X509) *recips;
  ::EVP_PKEY *pkey;
  CryptographicMessageSyntax::CipherTypes CipherType;
#endif
};

CryptographicMessageSyntax::CryptographicMessageSyntax()
{
  Internals = new CryptographicMessageSyntaxInternals;
}

CryptographicMessageSyntax::~CryptographicMessageSyntax()
{
  delete Internals;
}

void CryptographicMessageSyntax::SetCipherType( CipherTypes type)
{
#ifdef GDCM_USE_SYSTEM_OPENSSL
  Internals->SetCipherType( type );
#endif
}

CryptographicMessageSyntax::CipherTypes CryptographicMessageSyntax::GetCipherType() const
{
#ifdef GDCM_USE_SYSTEM_OPENSSL
  return Internals->GetCipherType();
#else
  return AES256_CIPHER; // why not :)
#endif
}

//void CryptographicMessageSyntax::SetCertificate( X509 *cert )
//{
//  Internals->x509 = cert;
//}
//
//const X509 *CryptographicMessageSyntax::GetCertificate( ) const
//{
//  return Internals->x509;
//}

/*
openssl smime -encrypt -aes256 -in inputfile.txt -out outputfile.txt -outform DER /tmp/server.pem 
*/
#ifdef GDCM_USE_SYSTEM_OPENSSL
const EVP_CIPHER *CreateCipher( CryptographicMessageSyntax::CipherTypes ciphertype)
{
  const EVP_CIPHER *cipher = 0;
  switch( ciphertype )
    {
  case CryptographicMessageSyntax::DES_CIPHER:    // DES
    cipher = EVP_des_cbc();
    break;
  case CryptographicMessageSyntax::DES3_CIPHER:   // Triple DES
    cipher = EVP_des_ede3_cbc();
    break;
  case CryptographicMessageSyntax::AES128_CIPHER: // CBC AES
    cipher = EVP_aes_128_cbc();
    break;
  case CryptographicMessageSyntax::AES192_CIPHER: // '   '
    cipher = EVP_aes_192_cbc();
    break;
  case CryptographicMessageSyntax::AES256_CIPHER: // '   '
    cipher = EVP_aes_256_cbc();
    break;
    }
  return cipher;
}
#endif

bool CryptographicMessageSyntax::Encrypt(char *output, size_t &outlen, const char *array, size_t len) const
{
#ifdef GDCM_USE_SYSTEM_OPENSSL
	::PKCS7 *p7;
  BIO *data,*p7bio;
  char buf[1024*4];
  int i;
  int nodetach=1;
  const EVP_CIPHER *cipher=NULL;
  BIO*  wbio = NULL;
  time_t t;

  t = time(NULL);
  RAND_seed(&t,sizeof(t));
#ifdef _WIN32
  RAND_screen(); /* Loading video display memory into random state */
#endif
  CryptographicMessageSyntaxInternals *x509 = Internals;

  //OpenSSL_add_all_algorithms();

  //if (!BIO_read_filename(data,argv[1])) goto err;
  data = BIO_new_mem_buf((void*)array, len);
  if(!data) goto err;

  if(!cipher)  {
    cipher = CreateCipher( Internals->GetCipherType() );
  }
  if(!cipher)
    {
    return false;
    }

  // The following is inspired by PKCS7_encrypt
  // and openssl/crypto/pkcs7/enc.c
  p7=PKCS7_new();
  PKCS7_set_type(p7,NID_pkcs7_enveloped);
  //PKCS7_set_type(p7,NID_pkcs7_encrypted);


  if (!PKCS7_set_cipher(p7,cipher)) goto err;
  for(i = 0; i < x509->GetNumberOfRecipients(); i++) {
	  ::X509* recip = x509->GetRecipient(i);
    if (!PKCS7_add_recipient(p7,recip)) goto err;
  }

  if ((p7bio=PKCS7_dataInit(p7,NULL)) == NULL) goto err;

  for (;;)
    {
    i=BIO_read(data,buf,sizeof(buf));
    if (i <= 0) break;
    BIO_write(p7bio,buf,i);
    }
  (void)BIO_flush(p7bio);
  BIO_free(data);

  if (!PKCS7_dataFinal(p7,p7bio)) goto err;
  BIO_free_all(p7bio);

  if (!(wbio = BIO_new(BIO_s_mem()))) goto err;
  i2d_PKCS7_bio(wbio,p7);
  (void)BIO_flush(wbio);
  PKCS7_free(p7);
  p7 = NULL;

  char *binary;
  outlen = BIO_get_mem_data(wbio,&binary);
  memcpy( output, binary, outlen );

  BIO_free_all(wbio);  /* also frees b64 */

  t = time(NULL);
  RAND_seed(&t,sizeof(t));

  return true;
err:
  ERR_load_crypto_strings();
  ERR_print_errors_fp(stderr);
  return false;
#else
  outlen = 0;
  return false;
#endif /* GDCM_USE_SYSTEM_OPENSSL */
}

/*
 $ openssl smime -decrypt -in /tmp/debug.der -inform DER -recip /tmp/server.pem -inkey CA_key.pem   
*/
bool CryptographicMessageSyntax::Decrypt(char *output, size_t &outlen, const char *array, size_t len) const
{
#ifdef GDCM_USE_SYSTEM_OPENSSL
  CryptographicMessageSyntaxInternals *x509 = Internals;
  ::PKCS7 *p7;
#undef PKCS7_SIGNER_INFO
  ::PKCS7_SIGNER_INFO *si;
  X509_STORE_CTX cert_ctx;
  X509_STORE *cert_store=NULL;
  BIO *data,*detached=NULL,*p7bio=NULL;
  char buf[1024*4];
  unsigned char *pp;
  int i,printit=0;
  STACK_OF(PKCS7_SIGNER_INFO) *sk;
  char * ptr = output;

  OpenSSL_add_all_algorithms();
  //bio_err=BIO_new_fp(stderr,BIO_NOCLOSE);

  //data=BIO_new(BIO_s_file());
  pp=NULL;

  EVP_PKEY *pkey = x509->GetPrivateKey();

   //if (!BIO_read_filename(data,argv[0])) goto err; 
  data = BIO_new_mem_buf((void*)array, len);
  if(!data) goto err;


  if (pp == NULL)
    BIO_set_fp(data,stdin,BIO_NOCLOSE);


  /* Load the PKCS7 object from a file */
  //if ((p7=PEM_read_bio_PKCS7(data,NULL,NULL,NULL)) == NULL) goto err;
  if ((p7=d2i_PKCS7_bio(data,NULL)) == NULL) goto err;

  if(!PKCS7_type_is_enveloped(p7)) {
    goto err;
  }

//  if(cert && !X509_check_private_key(cert, pkey)) {
//    PKCS7err(PKCS7_F_PKCS7_DECRYPT,
//        PKCS7_R_PRIVATE_KEY_DOES_NOT_MATCH_CERTIFICATE);
//    return 0;
//  }


  /* This stuff is being setup for certificate verification.
   * When using SSL, it could be replaced with a 
   * cert_stre=SSL_CTX_get_cert_store(ssl_ctx); */
  cert_store=X509_STORE_new();
  X509_STORE_set_default_paths(cert_store);
  X509_STORE_load_locations(cert_store,NULL,"../../certs");
  //X509_STORE_set_verify_cb_func(cert_store,verify_callback);

  ERR_clear_error();

  /* We need to process the data */
  /* We cannot support detached encryption */
  p7bio=PKCS7_dataDecode(p7,pkey,detached,NULL);

  if (p7bio == NULL)
    {
    printf("problems decoding\n");
    goto err;
    }

  /* We now have to 'read' from p7bio to calculate digests etc. */
  for (;;)
    {
    i=BIO_read(p7bio,buf,sizeof(buf));
    /* print it? */
    if (i <= 0) break;
    //fwrite(buf,1, i, stdout);
    memcpy(ptr, buf, i);
    ptr += i;
    }

  /* We can now verify signatures */
  sk=PKCS7_get_signer_info(p7);
  if (sk == NULL)
    {
    //fprintf(stderr, "there are no signatures on this data\n");
    }
  else
    {
    /* Ok, first we need to, for each subject entry,
     * see if we can verify */
    ERR_clear_error();
    for (i=0; i<sk_PKCS7_SIGNER_INFO_num(sk); i++)
      {
      //si=my_sk_PKCS7_SIGNER_INFO_value(sk,i);
	        si=sk_PKCS7_SIGNER_INFO_value(sk,i);
      i=PKCS7_dataVerify(cert_store,&cert_ctx,p7bio,p7,si);
      if (i <= 0)
        goto err;
      else
        fprintf(stderr,"Signature verified\n");
      }
    }
  X509_STORE_free(cert_store);

  BIO_free_all(p7bio);
  PKCS7_free(p7); p7 = NULL;
  BIO_free(data);

  return true;
err:
  ERR_load_crypto_strings();
  ERR_print_errors_fp(stderr);
  return false;
#else
  outlen = 0;
  return false;
#endif /* GDCM_USE_SYSTEM_OPENSSL */
}


bool CryptographicMessageSyntax::ParseKeyFile( const char *keyfile)
{
#ifdef GDCM_USE_SYSTEM_OPENSSL
  ::BIO *in;
  ::EVP_PKEY *pkey;
  if ((in=::BIO_new_file(keyfile,"r")) == NULL)
    {
    return false;
    }
  //if ((x509=openssl::PEM_read_bio_X509(in,NULL,NULL,NULL)) == NULL) goto err;
  (void)BIO_reset(in);
  if ((pkey=PEM_read_bio_PrivateKey(in,NULL,NULL,NULL)) == NULL)
    {
    return false;
    }
  BIO_free(in);
  Internals->SetPrivateKey( pkey );
  return true;
#else
  return false;
#endif
}

bool CryptographicMessageSyntax::ParseCertificateFile( const char *keyfile)
{
#ifdef GDCM_USE_SYSTEM_OPENSSL
  ::STACK_OF(X509) *recips = Internals->GetRecipients();
  assert( recips );
  ::X509 *x509 = NULL;

  ::BIO *in;
  if (!(in=::BIO_new_file(keyfile,"r")))
    {
    return false;
    }
  // -> LEAK reported by valgrind...
  if (!(x509=::PEM_read_bio_X509(in,NULL,NULL,NULL)))
    {
    return false;
    }
  ::BIO_free(in); in = NULL;
  ::sk_X509_push(recips, x509);
  return true;
#else
  return false;
#endif
}


} // end namespace gdcm
