/*

Copyright 2018 Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include <libff/algebra/fields/fp.hpp>
#include <dkg/dkg.h>



#include <jsonrpccpp/server/connectors/httpserver.h>


#include <libff/algebra/curves/alt_bn128/alt_bn128_pp.hpp>
#include <libff/algebra/exponentiation/exponentiation.hpp>

#include <libff/algebra/fields/fp.hpp>

#include <dkg/dkg.h>

#include "sgxwallet_common.h"
#include "create_enclave.h"
#include "secure_enclave_u.h"
#include "sgx_detect.h"
#include <gmp.h>
#include <sgx_urts.h>
#include <stdio.h>

#include "BLSCrypto.h"
#include "ServerInit.h"

#include "DKGCrypto.h"

#include "RPCException.h"
#include "LevelDB.h"

#include "SGXWalletServer.hpp"

#include <sgx_tcrypto.h>

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include "stubclient.h"

#include "BLSSigShare.h"
#include "BLSSigShareSet.h"
#include "BLSPublicKeyShare.h"
#include "BLSPublicKey.h"

std::string stringFromFr(libff::alt_bn128_Fr& el) {

    mpz_t t;
    mpz_init(t);

    el.as_bigint().to_mpz(t);

    char arr[mpz_sizeinbase(t, 10) + 2];

    char *tmp = mpz_get_str(arr, 10, t);
    mpz_clear(t);

    return std::string(tmp);
}


void usage() {
    fprintf(stderr, "usage: sgxwallet\n");
    exit(1);
}

sgx_launch_token_t token = {0};
sgx_enclave_id_t eid;
sgx_status_t status;
int updated;

#define  TEST_BLS_KEY_SHARE "4160780231445160889237664391382223604184857153814275770598791864649971919844"
#define TEST_BLS_KEY_NAME "SCHAIN:17:INDEX:5:KEY:1"

void reset_db() {
    REQUIRE(system("rm -rf " WALLETDB_NAME) == 0);
}

char* encryptTestKey() {

    const char *key = TEST_BLS_KEY_SHARE;


    int errStatus = -1;

    char *errMsg = (char *) calloc(BUF_LEN, 1);

    char *encryptedKeyHex = encryptBLSKeyShare2Hex(&errStatus, errMsg, key);

    REQUIRE(encryptedKeyHex != nullptr);
    REQUIRE(errStatus == 0);

    printf("Encrypt key completed with status: %d %s \n", errStatus, errMsg);
    printf("Encrypted key len %d\n", (int) strlen(encryptedKeyHex));
    printf("Encrypted key %s \n", encryptedKeyHex);

    return encryptedKeyHex;
}


TEST_CASE("BLS key encrypt", "[bls-key-encrypt]") {


    init_all();
    char* key = encryptTestKey();
    REQUIRE(key != nullptr);

}


TEST_CASE("BLS key encrypt/decrypt", "[bls-key-encrypt-decrypt]") {
    {


        init_all();

        int errStatus =  -1;
        char* errMsg = (char*) calloc(BUF_LEN, 1);



        char* encryptedKey = encryptTestKey();
        REQUIRE(encryptedKey != nullptr);

        char* plaintextKey = decryptBLSKeyShareFromHex(&errStatus, errMsg, encryptedKey);

        REQUIRE(errStatus == 0);

        REQUIRE(strcmp(plaintextKey, TEST_BLS_KEY_SHARE) == 0);

        printf("Decrypt key completed with status: %d %s \n", errStatus, errMsg);
        printf("Decrypted key len %d\n", (int) strlen(plaintextKey));
        printf("Decrypted key: %s\n", plaintextKey);


    }
}

TEST_CASE("BLS key import", "[bls-key-import]") {
    reset_db();
    init_all();



    auto result = importBLSKeyShareImpl(TEST_BLS_KEY_SHARE, TEST_BLS_KEY_NAME, 2, 2, 1);

    REQUIRE(result["status"] == 0);

    REQUIRE(result["encryptedKeyShare"] != "");

}


TEST_CASE("BLS sign test", "[bls-sign]") {

    //init_all();
    init_enclave();

    char* encryptedKeyHex ="04000200000000000406ffffff02000000000000000000000b000000000000ff0000000000000000813f8390f6228a568e181a4dadb6508e3e66f5247175d65dbd0d8c7fbfa4df45000000f000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000800000000000000000000000000000008000000000000000000000000000000000dc044ae0cd79faaf41e8a7abb412790476738a98b5b6ce95fa1a32db5551b0a0d867305f4de558c64fee730a1f62394633c7d4ca65e3a40b7883e89c2801c61918b01c5de8624a52963df6f4de8581bcbdd2f9b69720d4cc764e03a04c7a99314bfdb5d2d55deda2ca40cd691f093fb2ecbae24cdacdd4d5de93189c6dfd6792d7b95bd5e330aec3538e7a85d15793"; //encryptTestKey();

    REQUIRE(encryptedKeyHex != nullptr);


   // const char *hexHash = "001122334455667788" "001122334455667788" "001122334455667788" "001122334455667788";
    const char *hexHash = "3F891FDA3704F0368DAB65FA81EBE616F4AA2A0854995DA4DC0B59D2CADBD64F";

    char* hexHashBuf = (char*) calloc(BUF_LEN, 1);

    strncpy(hexHashBuf,  hexHash, BUF_LEN);

    char sig[BUF_LEN];
    auto result = sign(encryptedKeyHex, hexHashBuf, 2, 2, 1, sig);

    REQUIRE(result == true);
    printf("Signature is: %s \n",  sig );

}

TEST_CASE("Server BLS sign test", "[bls-server-sign]") {

    reset_db();

    init_all();

    auto result = importBLSKeyShareImpl( TEST_BLS_KEY_SHARE, TEST_BLS_KEY_NAME, 2, 2, 1);

    REQUIRE(result["status"] == 0);

    REQUIRE(result["encryptedKeyShare"] != "");

    const char *hexHash = "001122334455667788" "001122334455667788" "001122334455667788" "001122334455667788";

    REQUIRE_NOTHROW(result = blsSignMessageHashImpl(TEST_BLS_KEY_NAME, hexHash,2,2,1));

    if (result["status"] != 0) {
        printf("Error message: %s", result["errorMessage"].asString().c_str());
    }


    REQUIRE(result["status"] == 0);
    REQUIRE(result["signatureShare"] != "");

    printf("Signature is: %s \n",  result["signatureShare"].asString().c_str());

}

TEST_CASE("KeysDB test", "[keys-db]") {



    reset_db();
    init_all();


    string key = TEST_BLS_KEY_SHARE;
    string value = TEST_BLS_KEY_SHARE;



    REQUIRE_THROWS(readKeyShare(key));


    writeKeyShare(key, value, 1, 2, 1);

    REQUIRE(readKeyShare(key) != nullptr);


// put your test here
}




TEST_CASE( "DKG gen test", "[dkg-gen]" ) {

  //init_all();
  init_enclave();
  uint8_t* encrypted_dkg_secret = (uint8_t*) calloc(DKG_MAX_SEALED_LEN, 1);

  char* errMsg = (char*) calloc(1024,1);
  int err_status = 0;
  uint32_t enc_len = 0;

  status = gen_dkg_secret (eid, &err_status, errMsg, encrypted_dkg_secret, &enc_len, 32);
  REQUIRE(status == SGX_SUCCESS);
  printf("gen_dkg_secret completed with status: %d %s \n", err_status, errMsg);
  printf("\n Length: %d \n", enc_len);

  char* secret = (char*)calloc(DKG_BUFER_LENGTH, sizeof(char));

  char* errMsg1 = (char*) calloc(1024,1);

  uint32_t dec_len;
  status = decrypt_dkg_secret(eid, &err_status, errMsg1, encrypted_dkg_secret, (uint8_t*)secret, &dec_len);
  REQUIRE(status == SGX_SUCCESS);

  printf("\ndecrypt_dkg_secret completed with status: %d %s \n", err_status, errMsg1);
  printf("decrypted secret %s \n\n", secret);
  printf ("secret length %d \n", strlen(secret));
  printf ("decr length %d \n", dec_len);

  free(errMsg);
  free(errMsg1);
  free(encrypted_dkg_secret);
  free(secret);

  sgx_destroy_enclave(eid);
}

std::vector<libff::alt_bn128_Fr> SplitStringToFr(const char* koefs, const char symbol){
  std::string str(koefs);
  std::string delim;
  delim.push_back(symbol);
  std::vector<libff::alt_bn128_Fr> tokens;
  size_t prev = 0, pos = 0;
  do
  {
    pos = str.find(delim, prev);
    if (pos == std::string::npos) pos = str.length();
    std::string token = str.substr(prev, pos-prev);
    if (!token.empty()) {
      libff::alt_bn128_Fr koef(token.c_str());
      tokens.push_back(koef);
    }
    prev = pos + delim.length();
  }
  while (pos < str.length() && prev < str.length());

  return tokens;
}

std::vector<std::string> SplitStringTest(const char* koefs, const char symbol){
  libff::init_alt_bn128_params();
  std::string str(koefs);
  std::string delim;
  delim.push_back(symbol);
  std::vector<std::string> G2_strings;
  size_t prev = 0, pos = 0;
  do
  {
    pos = str.find(delim, prev);
    if (pos == std::string::npos) pos = str.length();
    std::string token = str.substr(prev, pos-prev);
    if (!token.empty()) {
      std::string koef(token.c_str());
      G2_strings.push_back(koef);
    }
    prev = pos + delim.length();
  }
  while (pos < str.length() && prev < str.length());

  return G2_strings;
}

libff::alt_bn128_G2 VectStringToG2(const std::vector<std::string>& G2_str_vect){
  libff::init_alt_bn128_params();
  libff::alt_bn128_G2 koef = libff::alt_bn128_G2::zero();
  koef.X.c0 = libff::alt_bn128_Fq(G2_str_vect.at(0).c_str());
  koef.X.c1 = libff::alt_bn128_Fq(G2_str_vect.at(1).c_str());
  koef.Y.c0 = libff::alt_bn128_Fq(G2_str_vect.at(2).c_str());
  koef.Y.c1 = libff::alt_bn128_Fq(G2_str_vect.at(3).c_str());
  koef.Z.c0 = libff::alt_bn128_Fq::one();
  koef.Z.c1 = libff::alt_bn128_Fq::zero();

  return koef;
}



 TEST_CASE( "DKG secret shares test", "[dkg-s_shares]" ) {

  //init_all();
  init_enclave();
  libff::init_alt_bn128_params();

  uint8_t* encrypted_dkg_secret = (uint8_t*) calloc(DKG_MAX_SEALED_LEN, 1);

  char* errMsg = (char*) calloc(1024,1);
  int err_status = 0;
  uint32_t enc_len = 0;

  unsigned t = 32, n = 32;

  status = gen_dkg_secret (eid, &err_status, errMsg, encrypted_dkg_secret, &enc_len, n);
  REQUIRE(status == SGX_SUCCESS);
  printf("gen_dkg_secret completed with status: %d %s \n", err_status, errMsg);
  printf("\n Length: %d \n", enc_len);
 /* printf("encr_dkg_secret: \n");
  for ( int i = 0 ; i < enc_len; i++)
    printf(" %d ", encrypted_dkg_secret[i]);*/

  char* errMsg1 = (char*) calloc(1024,1);

  char colon = ':';
  char* secret_shares = (char*)calloc(DKG_BUFER_LENGTH, sizeof(char));
  uint32_t dec_len = enc_len;
 // status = decrypt_dkg_secret(eid, &err_status, errMsg1, encrypted_dkg_secret, (uint8_t*)secret_shares, &dec_len);
  status = get_secret_shares(eid, &err_status, errMsg1, encrypted_dkg_secret, &dec_len, secret_shares, t, n);
  REQUIRE(status == SGX_SUCCESS);
  printf("\nget_secret_shares status: %d %s \n", err_status, errMsg1);
  printf("secret shares %s \n\n", secret_shares);

  std::vector <libff::alt_bn128_Fr> s_shares = SplitStringToFr( secret_shares, colon);

 char* secret = (char*)calloc(DKG_BUFER_LENGTH, sizeof(char));
 status = decrypt_dkg_secret(eid, &err_status, errMsg1, encrypted_dkg_secret, (uint8_t*)secret, &dec_len);
 REQUIRE(status == SGX_SUCCESS);
 //printf("\ndecrypt_dkg_secret completed with status: %d %s \n", err_status, errMsg1);

 signatures::Dkg dkg_obj(t,n);

 std::vector < libff::alt_bn128_Fr> poly = SplitStringToFr((char*)secret, colon);
 std::vector < libff::alt_bn128_Fr> s_shares_dkg = dkg_obj.SecretKeyContribution(SplitStringToFr((char*)secret, colon));
 printf("calculated secret length %d : \n", s_shares_dkg.size());
 for ( int  i = 0; i < s_shares_dkg.size(); i++){
   libff::alt_bn128_Fr cur_share = s_shares_dkg.at(i);
   mpz_t(sshare);
   mpz_init(sshare);
   cur_share.as_bigint().to_mpz(sshare);
   char arr[mpz_sizeinbase (sshare, 10) + 2];
   char* share_str = mpz_get_str(arr, 10, sshare);
   printf(" %s \n", share_str);
   mpz_clear(sshare);
 }

 REQUIRE(s_shares == s_shares_dkg);

  free(errMsg);
  free(errMsg1);
  free(encrypted_dkg_secret);
  free(secret_shares);

  sgx_destroy_enclave(eid);
}

TEST_CASE( "DKG public shares test", "[dkg-pub_shares]" ) {

  //init_all();
  libff::init_alt_bn128_params();
  init_enclave();
  uint8_t* encrypted_dkg_secret = (uint8_t*) calloc(DKG_MAX_SEALED_LEN, 1);

  char* errMsg = (char*) calloc(1024,1);
  int err_status = 0;
  uint32_t enc_len = 0;

  unsigned t = 32, n = 32;

  status = gen_dkg_secret (eid, &err_status, errMsg, encrypted_dkg_secret, &enc_len, n);
  REQUIRE(status == SGX_SUCCESS);
  //printf("gen_dkg_public completed with status: %d %s \n", err_status, errMsg);


  char* errMsg1 = (char*) calloc(1024,1);

  char colon = ':';
  char* public_shares = (char*)calloc(10000, 1);
  status = get_public_shares(eid, &err_status, errMsg1, encrypted_dkg_secret, enc_len, public_shares, t, n);
  REQUIRE(status == SGX_SUCCESS);
  printf("\nget_public_shares status: %d error %s \n\n", err_status, errMsg1);
  printf(" LEN: %d \n", strlen(public_shares));
  printf(" result: %s \n", public_shares);

  std::vector <std::string> G2_strings = SplitString( public_shares, ',');
  std::vector <libff::alt_bn128_G2> pub_shares_G2;
  for ( int i = 0; i < G2_strings.size(); i++){
    std::vector <std::string> koef_str = SplitString(G2_strings.at(i).c_str(), ':');
    libff::alt_bn128_G2 el = VectStringToG2(koef_str);
    //std::cerr << "pub_share G2 " << i+1 << " : " << std::endl;
    //el.print_coordinates();
    pub_shares_G2.push_back(VectStringToG2(koef_str));
  }

  char* secret = (char*)calloc(DKG_MAX_SEALED_LEN, sizeof(char));
  status = decrypt_dkg_secret(eid, &err_status, errMsg1, encrypted_dkg_secret, (uint8_t*)secret, &enc_len);
  REQUIRE(status == SGX_SUCCESS);
  printf("\ndecrypt_dkg_secret completed with status: %d %s \n", err_status, errMsg1);

  signatures::Dkg dkg_obj(t,n);

  std::vector < libff::alt_bn128_Fr> poly = SplitStringToFr((char*)secret, colon);
  std::vector < libff::alt_bn128_G2> pub_shares_dkg = dkg_obj.VerificationVector(poly);
  printf("calculated public shares (X.c0): \n");
  for ( int  i = 0; i < pub_shares_dkg.size(); i++){
    libff::alt_bn128_G2 el = pub_shares_dkg.at(i);
    el.to_affine_coordinates();
    libff::alt_bn128_Fq x_c0_el = el.X.c0;
    mpz_t x_c0;
    mpz_init(x_c0);
    x_c0_el.as_bigint().to_mpz(x_c0);
    char arr[mpz_sizeinbase (x_c0, 10) + 2];
    char* share_str = mpz_get_str(arr, 10, x_c0);
    printf(" %s \n", share_str);
    mpz_clear(x_c0);
  }

  bool res = (pub_shares_G2 == pub_shares_dkg);
  REQUIRE( res == true);

  free(errMsg);
  free(errMsg1);
  free(encrypted_dkg_secret);
  free(public_shares);

  sgx_destroy_enclave(eid);
}

TEST_CASE( "DKG encrypted secret shares test", "[dkg-encr_sshares]" ) {

  // init_all();
  init_enclave();
  uint8_t *encrypted_key = (uint8_t *) calloc(BUF_LEN, 1);

  char *errMsg = (char *)calloc(1024, 1);
  char *result = (char *)calloc(130, 1);

  int err_status = 0;
  uint32_t enc_len = 0;


  uint8_t* encrypted_dkg_secret = (uint8_t*) calloc(DKG_MAX_SEALED_LEN, 1);


  status = gen_dkg_secret (eid, &err_status, errMsg, encrypted_dkg_secret, &enc_len, 2);
  REQUIRE(status == SGX_SUCCESS);
  std::cerr << " poly generated" << std::endl;

  status = set_encrypted_dkg_poly(eid, &err_status, errMsg, encrypted_dkg_secret);
  REQUIRE(status == SGX_SUCCESS);
  std::cerr << " poly set" << std::endl;

  uint8_t *encr_pr_DHkey = (uint8_t *)calloc(1024, 1);
  char *pub_key_x = (char *)calloc(1024, 1);
  char *pub_key_y = (char *)calloc(1024, 1);

  char *pub_keyB = "c0152c48bf640449236036075d65898fded1e242c00acb45519ad5f788ea7cbf9a5df1559e7fc87932eee5478b1b9023de19df654395574a690843988c3ff475";
  char s_shareG2[320];
  status = get_encr_sshare(eid, &err_status, errMsg, encr_pr_DHkey, &enc_len, result, s_shareG2,
                     pub_keyB, 2, 2, 1);
  REQUIRE(status == SGX_SUCCESS);
  printf(" get_encr_sshare completed with status: %d %s \n", err_status, errMsg);

  std::cerr << "secret share is " << result << std::endl;
}

TEST_CASE( "DKG verification test", "[dkg-verify]" ) {

  // init_all();
  init_enclave();
  uint8_t *encrypted_key = (uint8_t *) calloc(BUF_LEN, 1);

  char *errMsg = (char *)calloc(1024, 1);
  char *result = (char *)calloc(130, 1);

  int err_status = 0;
  uint32_t enc_len = 0;


  uint8_t* encrypted_dkg_secret = (uint8_t*) calloc(DKG_MAX_SEALED_LEN, 1);


  status = gen_dkg_secret (eid, &err_status, errMsg, encrypted_dkg_secret, &enc_len, 2);
  REQUIRE(status == SGX_SUCCESS);
  std::cerr << " poly generated" << std::endl;

  status = set_encrypted_dkg_poly(eid, &err_status, errMsg, encrypted_dkg_secret);
  REQUIRE(status == SGX_SUCCESS);
  std::cerr << " poly set" << std::endl;

  uint8_t *encr_pr_DHkey = (uint8_t *)calloc(1024, 1);
  char *pub_key_x = (char *)calloc(1024, 1);
  char *pub_key_y = (char *)calloc(1024, 1);

  char *pub_keyB = "c0152c48bf640449236036075d65898fded1e242c00acb45519ad5f788ea7cbf9a5df1559e7fc87932eee5478b1b9023de19df654395574a690843988c3ff475";

  char s_shareG2[320];
  status = get_encr_sshare(eid, &err_status, errMsg, encr_pr_DHkey, &enc_len, result, s_shareG2,
                           pub_keyB, 2, 2, 1);
  REQUIRE(status == SGX_SUCCESS);
  printf(" get_encr_sshare completed with status: %d %s \n", err_status, errMsg);

  std::cerr << "secret share is " << result << std::endl;
}


TEST_CASE("ECDSA keygen and signature test", "[ecdsa_test]") {

  init_enclave();

  char *errMsg = (char *)calloc(1024, 1);
  int err_status = 0;
  uint8_t *encr_pr_key = (uint8_t *)calloc(1024, 1);

  char *pub_key_x = (char *)calloc(1024, 1);
  char *pub_key_y = (char *)calloc(1024, 1);
  uint32_t enc_len = 0;

  //printf("before %p\n", pub_key_x);

  status = generate_ecdsa_key(eid, &err_status, errMsg, encr_pr_key, &enc_len, pub_key_x, pub_key_y );
  printf("\nerrMsg %s\n", errMsg );
  REQUIRE(status == SGX_SUCCESS);

  printf("\nwas pub_key_x %s: \n", pub_key_x);
  printf("\nwas pub_key_y %s: \n", pub_key_y);
  /*printf("\nencr priv_key : \n");
  for ( int i = 0; i < 1024 ; i++)
    printf("%u ", encr_pr_key[i]);*/

  char* hex = "3F891FDA3704F0368DAB65FA81EBE616F4AA2A0854995DA4DC0B59D2CADBD64F";
 // char* hex = "0x09c6137b97cdf159b9950f1492ee059d1e2b10eaf7d51f3a97d61f2eee2e81db";
  printf("hash length %d ", strlen(hex));
  char* signature_r = (char *)calloc(1024, 1);
  char* signature_s = (char *)calloc(1024, 1);
  uint8_t signature_v = 0;

  status = ecdsa_sign1(eid, &err_status, errMsg, encr_pr_key, enc_len, (unsigned char*)hex, signature_r, signature_s, &signature_v, 16);
  REQUIRE(status == SGX_SUCCESS);
  printf("\nsignature r : %s  ", signature_r);
  printf("\nsignature s: %s  ", signature_s);
  printf("\nsignature v: %u  ", signature_v);
  printf("\n %s  \n", errMsg);

  free(errMsg);
  sgx_destroy_enclave(eid);
  printf("the end of ecdsa test\n");
}

TEST_CASE("Test test", "[test_test]") {

  init_enclave();

  char *errMsg = (char *)calloc(1024, 1);
  int err_status = 0;
  uint8_t *encr_pr_key = (uint8_t *)calloc(1024, 1);

  char *pub_key_x = (char *)calloc(1024, 1);
  char *pub_key_y = (char *)calloc(1024, 1);
  uint32_t enc_len = 0;

  status = generate_ecdsa_key(eid, &err_status, errMsg, encr_pr_key, &enc_len, pub_key_x, pub_key_y );
  //printf("\nerrMsg %s\n", errMsg );
  REQUIRE(status == SGX_SUCCESS);


  //printf("\nwas pub_key_x %s: \n", pub_key_x);
  //printf("\nwas pub_key_y %s: \n", pub_key_y);
  //printf("\nencr priv_key %s: \n");

  //for ( int i = 0; i < 1024 ; i++)
   // printf("%u ", encr_pr_key[i]);

  //printf( "haha");

  //free(errMsg);
  sgx_destroy_enclave(eid);


}

TEST_CASE("get public ECDSA key", "[get_pub_ecdsa_key_test]") {

  //init_all();
  init_enclave();

  char *errMsg = (char *)calloc(1024, 1);
  int err_status = 0;
  uint8_t *encr_pr_key = (uint8_t *)calloc(1024, 1);

  char *pub_key_x = (char *)calloc(1024, 1);
  char *pub_key_y = (char *)calloc(1024, 1);
  uint32_t enc_len = 0;

  //printf("before %p\n", pub_key_x);

  status = generate_ecdsa_key(eid, &err_status, errMsg, encr_pr_key, &enc_len, pub_key_x, pub_key_y );
  printf("\nerrMsg %s\n", errMsg );
  REQUIRE(status == SGX_SUCCESS);

  printf("\nwas pub_key_x %s length %d: \n", pub_key_x, strlen(pub_key_x));
  printf("\nwas pub_key_y %s length %d: \n", pub_key_y, strlen(pub_key_y));

  /*printf("\nencr priv_key %s: \n");
  for ( int i = 0; i < 1024 ; i++)
   printf("%u ", encr_pr_key[i]);*/

  char *got_pub_key_x = (char *)calloc(1024, 1);
  char *got_pub_key_y = (char *)calloc(1024, 1);

  status = get_public_ecdsa_key(eid, &err_status, errMsg, encr_pr_key, enc_len, got_pub_key_x,  got_pub_key_y);
  REQUIRE(status == SGX_SUCCESS);
  printf("\nnow pub_key_x %s: \n", got_pub_key_x);
  printf("\nnow pub_key_y %s: \n", got_pub_key_y);
  printf("\n pr key  %s  \n", errMsg);

  free(errMsg);
  sgx_destroy_enclave(eid);
}

/*TEST_CASE( "verification test", "[verify]" ) {


    char*  pubshares = "0d72c21fc5a43452ad5f36699822309149ce6ce2cdce50dafa896e873f1b8ddd12f65a2e9c39c617a1f695f076b33b236b47ed773901fc2762f8b6f63277f5e30d7080be8e98c97f913d1920357f345dc0916c1fcb002b7beb060aa8b6b473a011bfafe9f8a5d8ea4c643ca4101e5119adbef5ae64f8dfb39cd10f1e69e31c591858d7eaca25b4c412fe909ca87ca7aadbf6d97d32d9b984e93d436f13d43ec31f40432cc750a64ac239cad6b8f78c1f1dd37427e4ff8c1cc4fe1c950fcbcec10ebfd79e0c19d0587adafe6db4f3c63ea9a329724a8804b63a9422e6898c0923209e828facf3a073254ec31af4231d999ba04eb5b7d1e0056d742a65b766f2f3";
    char *sec_share = "11592366544581417165283270001305852351194685098958224535357729125789505948557";
    mpz_t sshare;
    mpz_init(sshare);
    mpz_set_str(sshare, "11592366544581417165283270001305852351194685098958224535357729125789505948557", 10);
    int result = Verification(pubshares, sshare, 2, 0);
    REQUIRE(result == 1);


}*/

TEST_CASE( "pub_bls_key", "[pub_bls]" ) {
  init_daemon();
  init_enclave();
  char *encryptedKeyHex =
      "04000200000000000406ffffff02000000000000000000000b000000000000ff0000000000000000813f8390f6228a568e181a4dadb6508e3e66f5247175d65dbd0d8c7fbfa4df45000000f000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000800000000000000000000000000000008000000000000000000000000000000000dc044ae0cd79faaf41e8a7abb412790476738a98b5b6ce95fa1a32db5551b0a0d867305f4de558c64fee730a1f62394633c7d4ca65e3a40b7883e89c2801c61918b01c5de8624a52963df6f4de8581bcbdd2f9b69720d4cc764e03a04c7a99314bfdb5d2d55deda2ca40cd691f093fb2ecbae24cdacdd4d5de93189c6dfd6792d7b95bd5e330aec3538e7a85d15793"; // encryptTestKey();
  //writeDataToDB("test_bls_key0", encryptedKeyHex);
  std::vector<std::string> result = GetBLSPubKey(encryptedKeyHex);
  //std::cerr << "pub key " << result << std::endl;
  sgx_destroy_enclave(eid);
}

#include "stubclient.h"
#include <jsonrpccpp/client/connectors/httpclient.h>

using namespace jsonrpc;
using namespace std;

std::string ConvertDecToHex(std::string dec, int numBytes = 32){
  mpz_t num;
  mpz_init(num);
  mpz_set_str(num, dec.c_str(), 10);

  char tmp[mpz_sizeinbase (num, 16) + 2];
  char * hex = mpz_get_str(tmp, 16, num);

  std::string result = hex;
  int n_zeroes = numBytes * 2 - result.length();
  result.insert(0, n_zeroes, '0');

  return result;
}


TEST_CASE("BLS_DKG test", "[bls_dkg]") {
  std::cerr<< "test started" << std::endl;
  init_all();
  cerr << "Server inited" << endl;
  HttpClient client("http://localhost:1027");
  StubClient c(client, JSONRPC_CLIENT_V2);
  cerr << "Client inited" << endl;

  reset_db();





  int n = 2, t = 2;
  Json::Value EthKeys[n];
  Json::Value VerifVects[n];
  Json::Value pubEthKeys;
  Json::Value secretShares[n];
  Json::Value pubBLSKeys[n];
  Json::Value BLSSigShares[n];
  std::vector<std::string> pubShares(n);
  std::vector<std::string> poly_names(n);

  for ( uint8_t i = 0; i < n; i++){
    EthKeys[i] = c.generateECDSAKey();
    std::string polyName = "POLY:SCHAIN_ID:1:NODE_ID:" + std::to_string(i) + ":DKG_ID:0";
    c.generateDKGPoly(polyName, t);
    poly_names[i] = polyName;
    VerifVects[i] = c.getVerificationVector(polyName, t, n);
    cout << "VV " << i <<  " " << VerifVects[i] << std::endl;
    pubEthKeys.append(EthKeys[i]["PublicKey"]);
  }


  for ( uint8_t i = 0; i < n; i++){
    secretShares[i] = c.getSecretShare(poly_names[i], pubEthKeys, t, n);
    for ( uint8_t k = 0; k < t; k++ ) {
      for (uint8_t j = 0; j < 4; j++) {
        std::string pubShare = VerifVects[i]["Verification Vector"][k][j].asString();
        pubShares[i] += ConvertDecToHex(pubShare);
      }
    }
//    std::cerr << "i is " << i << " pubShares[i] = " << pubShares[i] << std::endl;
//    std::cerr << "length is" << pubShares[i].length() << std::endl;
  }

  Json::Value ComplaintResponse = c.ComplaintResponse(poly_names[1], 0);
  std::cerr << "share * G2 is " << ComplaintResponse["share*G2"].asString();
  std::cerr << "DHKey is " << ComplaintResponse["DHKey"].asString();

  int k = 0;

  std::vector < std::string> secShares_vect(n);

  for ( int i = 0; i < n; i++)
    for ( int j = 0; j < n; j++){
     // if ( i != j ){
       std::cerr << "SecretShare length is " << secretShares[i]["SecretShare"].asString().length() << std::endl;
       std::string secretShare = secretShares[i]["SecretShare"].asString().substr(192*j, 192 * (j+1));
       secShares_vect[i] +=  secretShares[j]["SecretShare"].asString().substr(192*i, 192 * (i+1));
       bool res = c.DKGVerification(pubShares[i], EthKeys[j]["KeyName"].asString(), secretShare, t, n, j)["result"].asBool();
       k++;
       std::cerr << "NOW K IS " << k << " i is " << i << " j is " << j << std::endl;
       REQUIRE( res );
    //  }
    }

  BLSSigShareSet sigShareSet(t, n);

  std::string hash = "09c6137b97cdf159b9950f1492ee059d1e2b10eaf7d51f3a97d61f2eee2e81db";

  auto hash_arr = std::make_shared<std::array<uint8_t, 32>>();
  uint64_t binLen;
  if (!hex2carray(hash.c_str(), &binLen, hash_arr->data())){
        throw RPCException(INVALID_HEX, "Invalid hash");
  }


  std::map<size_t, std::shared_ptr<BLSPublicKeyShare>> koefs_pkeys_map;

  for ( int i = 0; i < t; i++){
    std::string endName = poly_names[i].substr(4);
    std::string blsName = "BLS_KEY" + poly_names[i].substr(4);
    std::string secretShare = secretShares[i]["SecretShare"].asString();
    //cout << c.CreateBLSPrivateKey(blsName, EthKeys[i]["KeyName"].asString(), poly_names[i], secretShare, t, n);
    cout << c.CreateBLSPrivateKey(blsName, EthKeys[i]["KeyName"].asString(), poly_names[i], secShares_vect[i], t, n);
    pubBLSKeys[i] = c.GetBLSPublicKeyShare(blsName);
    std::cerr << "BLS KEY SHARE NAME IS " << blsName << std::endl;
    //std::string hash = "09c6137b97cdf159b9950f1492ee059d1e2b10eaf7d51f3a97d61f2eee2e81db";
    BLSSigShares[i] = c.blsSignMessageHash(blsName, hash, t, n, i + 1);
    std::cerr << i << " sig share is created " << std::endl;
    std::shared_ptr<std::string> sig_share_ptr = std::make_shared<std::string>(BLSSigShares[i]["signatureShare"].asString());
    BLSSigShare sig(sig_share_ptr, i + 1, t, n);
    sigShareSet.addSigShare(std::make_shared<BLSSigShare>(sig));

    std::vector<std::string> pubKey_vect;
    for ( uint8_t j = 0; j < 4; j++){
        pubKey_vect.push_back(pubBLSKeys[i]["BLSPublicKeyShare"][j].asString());
    }
    BLSPublicKeyShare pubKey(std::make_shared<std::vector<std::string>>(pubKey_vect), t, n);
    REQUIRE( pubKey.VerifySigWithHelper(hash_arr, std::make_shared<BLSSigShare>(sig) , t, n));

    koefs_pkeys_map[i+1] = std::make_shared<BLSPublicKeyShare>(pubKey);

  }

  std::shared_ptr<BLSSignature> commonSig = sigShareSet.merge();
  BLSPublicKey common_public(std::make_shared<std::map<size_t, std::shared_ptr<BLSPublicKeyShare>>>(koefs_pkeys_map), t, n);
  REQUIRE( common_public.VerifySigWithHelper(hash_arr, commonSig, t, n) );

  std::cout << "try to get bls public key" << std::endl;
  std::cout << c.GetBLSPublicKeyShare("BLS_KEY:SCHAIN_ID:1:NODE_ID:1:DKG_ID:0");

  exit(0);

}

TEST_CASE("API test", "[api_test]") {

  //std::cerr << __GNUC__ << std::endl;
    cerr << "API test started" << endl;
    init_all();
    //HttpServer httpserver(1025);
    //SGXWalletServer s(httpserver,
    //                JSONRPC_SERVER_V2); // hybrid server (json-rpc 1.0 & 2.0)
   // s.StartListening();
    cerr << "Server inited" << endl;
    HttpClient client("http://localhost:1027");
    StubClient c(client, JSONRPC_CLIENT_V2);

    cerr << "Client inited" << endl;

    try {
        //levelDb->deleteOlegKey("0");
        //levelDb->deleteOlegKey("1");
       // levelDb->deleteDHDKGKey("p2_0:");
        //levelDb->deleteDHDKGKey("p2_1:");
//        for ( uint8_t i = 0; i < 2; i++) {
//        levelDb->deleteKey("POLY:SCHAIN_ID:1:NODE_ID:" + std::to_string(i) +
//                             ":DKG_ID:0");
//
//          levelDb->deleteKey(" DKG_DH_KEY_POLY:SCHAIN_ID:0:NODE_ID:" + std::to_string(i)+ ":DKG_ID:0_0");
//          levelDb->deleteKey(" DKG_DH_KEY_POLY:SCHAIN_ID:0:NODE_ID:" + std::to_string(i)+ ":DKG_ID:0_1");
//        }

       //cout << c.importBLSKeyShare("4160780231445160889237664391382223604184857153814275770598791864649971919844","BLS_KEY:SCHAIN_ID:2660016693368503500803087136248943520694587309641817:NODE_ID:33909:DKG_ID:3522960548719023733985054069487289468077787284706573", 4, 3,1);

      //  cout << c.generateECDSAKey() << endl;
       // cout << c.renameESDSAKey("NODE_1CHAIN_1","tmp_NEK:bcacde0d26c0ea2c7e649992e7f791e1fba2492f5b7ae63dadb799075167c7fc");
      //  cout<<c.getPublicECDSAKey("NEK:7ca98cf32fd1edba26ea685820719fd2201b068a10c1264d382abbde13802a0e");
      //cout << c.ecdsaSignMessageHash(16, "NEK:697fadfc597bdbfae9ffb7412b80939e848c9c2fec2657bb2122b6d0d4a0dca8","0x09c6137b97cdf159b9950f1492ee059d1e2b10eaf7d51f3a97d61f2eee2e81db" );
        //cout << c.ecdsaSignMessageHash(16, "known_key1","0x09c6137b97cdf159b9950f1492ee059d1e2b10eaf7d51f3a97d61f2eee2e81db" );
        //  cout << c.blsSignMessageHash(TEST_BLS_KEY_NAME, "0x09c6137b97cdf159b9950f1492ee059d1e2b10eaf7d51f3a97d61f2eee2e81db", 2,2,1 );
         // cout << c.generateDKGPoly("pp2", 2);
       //  cout << c.generateDKGPoly("POLY:SCHAIN_ID:1:NODE_ID:1:DKG_ID:1", 2);
      //cout << c.generateDKGPoly("POLY:SCHAIN_ID:14225439306783892379384764908040542049263455631509697460847850632966314337557:NODE_ID:1:DKG_ID:71951190446274221430521459675625214118086594348715", 1);
       //cout << c.getVerificationVector("POLY:SCHAIN_ID:1:NODE_ID:1:DKG_ID:2", 2, 2);

      //  cout << c.getVerificationVector("polyy", 5,  5);

//      cout << c.getSecretShare("p2",
//          "505f55a38f9c064da744f217d1cb993a17705e9839801958cda7c884e08ab4dad7fd8d22953d3ac7f0913de24fd67d7ed36741141b8a3da152d7ba954b0f14e232d69c361f0bc9e05f1cf8ef387122dc1d2f7cee7b6cda3537fc9427c02328b01f02fd94ec933134dc795a642864f8cb41ae263e11abaf992e21fcf9be732deb",
//         2,2);

//        cout << c.getSecretShare("p2",
//              "669aa790e1c5f5199af82ab0b6f1965c382d23a2ebdda581454adba3fd082a30edab62b545f78f1e402ceef7340a0364a7046633d6151fe7e657d8b8a6352378b3e6fdfe2633256ae1662fcd23466d02ead907b5d4366136341cea5e46f5a7bb67d897d6e35f619810238aa143c416f61c640ed214eb9c67a34c4a31b7d25e6e",
//              2,2);

      Json::Value publicKeys;
      publicKeys.append("505f55a38f9c064da744f217d1cb993a17705e9839801958cda7c884e08ab4dad7fd8d22953d3ac7f0913de24fd67d7ed36741141b8a3da152d7ba954b0f14e2");
      publicKeys.append("378b3e6fdfe2633256ae1662fcd23466d02ead907b5d4366136341cea5e46f5a7bb67d897d6e35f619810238aa143c416f61c640ed214eb9c67a34c4a31b7d25");
     // cout << c.getSecretShare("POLY:SCHAIN_ID:1:NODE_ID:1:DKG_ID:1", publicKeys, 2, 2);
      // cout << c.generateDKGPoly("p3", 3);
     // cout << c.getSecretShare("p3",
       //                        "669aa790e1c5f5199af82ab0b6f1965c382d23a2ebdda581454adba3fd082a30edab62b545f78f1e402ceef7340a0364a7046633d6151fe7e657d8b8a6352378b3e6fdfe2633256ae1662fcd23466d02ead907b5d4366136341cea5e46f5a7bb67d897d6e35f619810238aa143c416f61c640ed214eb9c67a34c4a31b7d25e6e9d43f1c88581f53af993da1654c9f91829c1fe5344c4452ef8d2d8675c6a051c19029f6e4f82b035fb3552058cf22c5bbafd9e6456d579634987281765d130b0",
         //                      3,3);



      std::string share_big0 = "501e364a6ea516f4812b013bcc150cbb435a2c465c9fd525951264969d8441a986798fd3317c1c3e60f868bb26c4cff837d9185f4be6015d8326437cb5b69480495859cd5a385430ece51252acdc234d8dbde75708b600ac50b2974e813ee26bd87140d88647fcc44df7262bbba24328e8ce622cd627a15b508ffa0db9ae81e0e110fab42cfe40da66b524218ca3c8e5aa3363fbcadef748dc3523a7ffb95b8f5d8141a5163db9f69d1ab223494ed71487c9bb032a74c08a222d897a5e49a617";
      std::string share_big = "03f749e2fcc28021895d757ec16d1636784446f5effcd3096b045136d8ab02657b32adc577f421330b81f5b7063df3b08a0621a897df2584b9046ca416e50ecc27e8c3277e981f7e650f8640289be128eecf0105f89a20e5ffb164744c45cf191d627ce9ab6c44e2ef96f230f2a4de742ea43b6f74b56849138026610b2d965605ececba527048a0f29f46334b1cec1d23df036248b24eccca99057d24764acee66c1a3f2f44771d0d237bf9d18c4177277e3ce3dc4e83686a2647fce1565ee0";
      std::string share = share_big.substr(0, 192);

      std::string publicShares = "1fc8154abcbf0c2ebf559571d7b57a8995c0e293a73d4676a8f76051a0d0ace30e00a87c9f087254c9c860c3215c4f11e8f85a3e8fae19358f06a0cbddf3df1924b1347b9b58f5bcb20958a19bdbdd832181cfa9f9e9fd698f6a485051cb47b829d10f75b6e227a7d7366dd02825b5718072cd42c39f0352071808622b7db6421b1069f519527e49052a8da6e3720cbda9212fc656eef945f5e56a4159c3b9622d883400460a9eff07fe1873f9b1ec50f6cf70098b9da0b90625b176f12329fa2ecc65082c626dc702d9cfb23a06770d4a2c7867e269efe84e3709b11001fb380a32d609855d1d46bc60f21140c636618b8ff55ed06d7788b6f81b498f96d3f9";
    //  cout << c.DKGVerification(publicShares, "test_key1", "37092c06c423b627c38ff86d1e66608bdc1496ef855b86e9f773441ac0b285d92aa466376a6008de4aab9858aa34848775282c4c3b56370bf25827321619c6e47701c8a32e3f4bb28f5a3b12a09800f318c550cedff6150e9a673ea56ece8b76", 2, 2, 0);

     // cout << c.DKGVerification("oleh1", "key0", "37092c06c423b627c38ff86d1e66608bdc1496ef855b86e9f773441ac0b285d92aa466376a6008de4aab9858aa34848775282c4c3b56370bf25827321619c6e47701c8a32e3f4bb28f5a3b12a09800f318c550cedff6150e9a673ea56ece8b76", 2, 2, 0);

      Json::Value SecretShare;
      SecretShare.append(share_big0);
      SecretShare.append(share_big);

      //cout << c.CreateBLSPrivateKey( "test_bls_key1","test_key1", "p2", share_big0, 2, 2 );

     // std::string shares = "252122c309ed1f32faa897ede140c5b9c1bc07d5d9c94b7a22d4eeb13da7b7142aa466376a6008de4aab9858aa34848775282c4c3b56370bf25827321619c6e47701c8a32e3f4bb28f5a3b12a09800f318c550cedff6150e9a673ea56ece8b76df831dbef474cfc38be1c980130a8d273ff410fbf87deece9d7756a1b08ba9e954c1676cc7f2cac16e16cff0c877d8cf967381321fb4cc78e3638245a1dc85419766d281aff4935cc6eac25c9842032c8f7fae567c57622969599a72c42d2e1e";
     std::string shares = "252122c309ed1f32faa897ede140c5b9c1bc07d5d9c94b7a22d4eeb13da7b7142aa466376a6008de4aab9858aa34848775282c4c3b56370bf25827321619c6e47701c8a32e3f4bb28f5a3b12a09800f318c550cedff6150e9a673ea56ece8b7637092c06c423b627c38ff86d1e66608bdc1496ef855b86e9f773441ac0b285d92aa466376a6008de4aab9858aa34848775282c4c3b56370bf25827321619c6e47701c8a32e3f4bb28f5a3b12a09800f318c550cedff6150e9a673ea56ece8b76";
     //cout << c.CreateBLSPrivateKey( "test_bls1","key0", "oleh1", shares, 2, 2 );

     //cout << c.GetBLSPublicKeyShare("test_bls_key0");

      std::string s_share = "13b871ad5025fed10a41388265b19886e78f449f758fe8642ade51440fcf850bb2083f87227d8fb53fdfb2854e2d0abec4f47e2197b821b564413af96124cd84a8700f8eb9ed03161888c9ef58d6e5896403de3608e634e23e92fba041aa283484427d0e6de20922216c65865cfe26edd2cf9cbfc3116d007710e8d82feafd9135c497bef0c800ca310ba6044763572681510dad5e043ebd87ffaa1a4cd45a899222207f3d05dec8110d132ad34c62d6a3b40bf8e9f40f875125c3035062d2ca";
      std::string EthKeyName = "tmp_NEK:8abc8e8280fb060988b65da4b8cb00779a1e816ec42f8a40ae2daa520e484a01";
      //cout << c.CreateBLSPrivateKey( "test_blskey", EthKeyName, "JCGMt", s_share, 2, 2 );
      //cout << c.GetBLSPublicKeyShare("test_blskey");

     // cout << c.blsSignMessageHash("dOsRY","38433e5ce087dcc1be82fcc834eae83c256b3db87d34f84440d0b708daa0c6f7", 2, 2, 1);

    // cout << c.ComplaintResponse("POLY:SCHAIN_ID:1:NODE_ID:1:DKG_ID:1", 0);
     // cout << c.GetBLSPublicKeyShare("BLS_KEY:SCHAIN_ID:1:NODE_ID:1:DKG_ID:0");

    //  cout << c.getPublicECDSAKey("NEK:91573248d6b0ebd5b1bd313ab35163361b423c0f9f01bad085d166650b8b2c1f");
    cout << c.MultG2("4160780231445160889237664391382223604184857153814275770598791864649971919844");

    } catch (JsonRpcException &e) {
        cerr << e.what() << endl;
    }
  sgx_destroy_enclave(eid);
}

//decr sshare is 0570d18552dc248c5f806cbfeb96cdc40234d51233b3ba80a9c7b790ae4eed13
//common_key is e6d91ec58664d25dd80071520793ab307bf408158543a9710445bd663041a760decr
//sshare is d56909d4b29a0f1d306be98c019bed02e9c6b9b56bfe9e933314815983401b40
//common_key is 0e4506de4faa7a241fccbcc9339cce03737415ba38349ccfa7aec916d37cee07
//
//
//decr sshare is 1f63caaf684e632338cd7c17569fb65d820004266acd36f0b6d2cbd05648b071
//common_key is e969840c044a3e7252e4677225f2513722d545ff1612f35d0cd66cda65185356decr
//sshare is 20d254d489bb31fc5b470c641ba280ea35e7da3a86c11ccdb74d3f9898daaa93
//common_key is 18dffe11f73f6d53e8f53ce1fe0ab8192f7a9180d50fb7e34a01776652e73471
