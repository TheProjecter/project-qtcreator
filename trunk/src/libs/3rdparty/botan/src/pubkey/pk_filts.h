/*
* PK Filters
* (C) 1999-2009 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

#ifndef BOTAN_PK_FILTERS_H__
#define BOTAN_PK_FILTERS_H__

#include <botan/filter.h>
#include <botan/pubkey.h>

namespace Botan {

/*
* PK_Encryptor Filter
*/
class BOTAN_DLL PK_Encryptor_Filter : public Filter
   {
   public:
      void write(const byte[], u32bit);
      void end_msg();
      PK_Encryptor_Filter(PK_Encryptor* c,
                          RandomNumberGenerator& rng_ref) :
         cipher(c), rng(rng_ref) {}
      ~PK_Encryptor_Filter() { delete cipher; }
   private:
      PK_Encryptor* cipher;
      RandomNumberGenerator& rng;
      SecureVector<byte> buffer;
   };

/*
* PK_Decryptor Filter
*/
class BOTAN_DLL PK_Decryptor_Filter : public Filter
   {
   public:
      void write(const byte[], u32bit);
      void end_msg();
      PK_Decryptor_Filter(PK_Decryptor* c) : cipher(c) {}
      ~PK_Decryptor_Filter() { delete cipher; }
   private:
      PK_Decryptor* cipher;
      SecureVector<byte> buffer;
   };

/*
* PK_Signer Filter
*/
class BOTAN_DLL PK_Signer_Filter : public Filter
   {
   public:
      void write(const byte[], u32bit);
      void end_msg();

      PK_Signer_Filter(PK_Signer* s,
                       RandomNumberGenerator& rng_ref) :
         signer(s), rng(rng_ref) {}

      ~PK_Signer_Filter() { delete signer; }
   private:
      PK_Signer* signer;
      RandomNumberGenerator& rng;
   };

/*
* PK_Verifier Filter
*/
class BOTAN_DLL PK_Verifier_Filter : public Filter
   {
   public:
      void write(const byte[], u32bit);
      void end_msg();

      void set_signature(const byte[], u32bit);
      void set_signature(const MemoryRegion<byte>&);

      PK_Verifier_Filter(PK_Verifier* v) : verifier(v) {}
      PK_Verifier_Filter(PK_Verifier*, const byte[], u32bit);
      PK_Verifier_Filter(PK_Verifier*, const MemoryRegion<byte>&);
      ~PK_Verifier_Filter() { delete verifier; }
   private:
      PK_Verifier* verifier;
      SecureVector<byte> signature;
   };

}

#endif
