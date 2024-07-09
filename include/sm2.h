#ifndef ____SM_2__H__45AE147B_76D4_4DB9_8FBB_00FE54E5EA79__
#define ____SM_2__H__45AE147B_76D4_4DB9_8FBB_00FE54E5EA79__

/*
 * sm2 implimentation based upon libTomMath library and goldbar's sm3 project
 *
 * Author : Simon Pang of catt2009 / steven.psm@gmail.com 
 * 2012-6-22
 *
 * reference : 
 * 1. Public Key Cryptographic Algorithm SM2 Based on Elliptic Curves 
 * [Part 1: General] page 18/93 (or page 12 of part one)  and 28/93(page 22 of part one); 57/93(page 7 of part two)
 * 2. bn.pdf of LibTomMath User Manual
 * 3. Guide to Elliptic Curve Cryptography
 *
 */
// #ifdef WIN32
// #ifdef SM2_DLL_EXPORTS
// #define SM2_DLL_API __declspec(dllexport)
// #else
// #define SM2_DLL_API __declspec(dllimport)
// #endif
// #else //linux or android
#ifndef SM2_DLL_API
#define SM2_DLL_API
#endif
// #endif //WIn32 -- linux -- define -- end

//#define _DEBUG //打开后进行测试，测试使用的是http://www.oscca.gov.cn/News/201012/News_1197.htm文档里面的例子，使用里面的曲线，实际中不需要打开
//err
#define ERR_PARAM  -2
#define ERR_MEM_ALLOC -3
#define ERR_NEED_RAND_REGEN -4
#define ERR_MEM_LOW    -5
#define ERR_DECRYPTION_FAILED -6
#define ERR_UNKNOWN     -7
#define ERR_GENKEY_FAILED  -8

#define ERR_INFINITE_POINT -10
#define ERR_POINT_NOT_ON_CURVE  -11

#define ERR_SIG_VER_R_OR_S_LARGER_THAN_N	10
#define ERR_SIG_VER_T_EQUL_ZERO				11
#define ERR_SIG_VER_R_NOT_EQUL				12
#define ERR_HEX2BYTE_PARAM_ERROR			13
#define ERR_HEX2BYTE_INVALID_DATA			14
#define ERR_HEX2BYTE_BEYOND_RANGE			15

//#define KEY_LONG  256
#define MAX_STRLEN  256
#define MAX_TRY_TIMES 100


#ifdef __cplusplus
extern "C"{
#endif
/*
 * instruction : GM sm2 generate key pair
 * param:
 * @prikey, @pulPriLen : [out] : output private key
 * @pubkey_XY : [out] : output public key [---32 bytes of X coordinate---][---32bytes of Y coordinate ---]
 * return :
 * 0 : success
 * other errcode : operation failed
 */
SM2_DLL_API int GM_GenSM2keypair(unsigned char * prikey, unsigned long * pulPriLen,
					 unsigned char pubkey_XY[64]);




/*
 * instruction : sm2 signature with GM predefined curve params 
 * param:
 * @signedData ,@pulSigLen : [out] : output sig data *LenDgst will always return 64, 
 *      ###signedData = [r,s] = [---32 bytes of r ---][---32 bytes of s ---]
 * @Src ,@SrcLen: [in] : source data to digest and signature
 * @UserID, @lenUID :  [in] : user id
 * @prikey, @ulPrikeyLen : [in] : private key (Byte Stream) of a random number
 * return :
 * 0 : success
 * other errcode : operation failed
 *
 * #define SM2_P     "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF"
 * #define SM2_A     "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFC"
 * #define SM2_B     "28E9FA9E9D9F5E344D5A9E4BCF6509A7F39789F515AB8F92DDBCBD414D940E93"
 * #define SM2_N     "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFF7203DF6B21C6052B53BBF40939D54123"
 * #define SM2_G_X   "32C4AE2C1F1981195F9904466A39C9948FE30BBFF2660BE1715A4589334C74C7"
 * #define SM2_G_Y   "BC3736A2F4F6779C59BDCEE36B692153D0A9877CC62A474002DF32E52139F0A0"
 */
SM2_DLL_API int GM_SM2Sign(unsigned char * signedData, unsigned long * pulSigLen,
					unsigned char * Src, unsigned long SrcLen, 
					unsigned char * UserID, unsigned long lenUID,
					unsigned char * prikey, unsigned long ulPrikeyLen);



/*
 * instruction : sm2 signature verify with GM predefined curve params 
 * param:
 * @signedData ,@ulSigLen : [in] : sig data to verify , ulSigLen should be 64
 *    ###signedData's structure: [r,s] = [---32 bytes of r ---][---32 bytes of s ---]
 * @Src ,@SrcLen: [in] : source data to digest and signature
 * @UserID, @lenUID :  [in] : user id
 * @szPubkey_XY, @ulPubkey_XYLen : [in] : public key of a (XA,YA),ulPubkey_XYLen should be 64
 *    ###structure of pubkey should be : [---32 byte of X coordinate---][---32 byte of Y coordinate---]
 * return :
 * 0 : success
 * other errcode : operation failed
 *
 * #define SM2_P     "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF"
 * #define SM2_A     "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFC"
 * #define SM2_B     "28E9FA9E9D9F5E344D5A9E4BCF6509A7F39789F515AB8F92DDBCBD414D940E93"
 * #define SM2_N     "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFF7203DF6B21C6052B53BBF40939D54123"
 * #define SM2_G_X   "32C4AE2C1F1981195F9904466A39C9948FE30BBFF2660BE1715A4589334C74C7"
 * #define SM2_G_Y   "BC3736A2F4F6779C59BDCEE36B692153D0A9877CC62A474002DF32E52139F0A0"
 */
SM2_DLL_API int GM_SM2VerifySig(unsigned char * signedData, unsigned long ulSigLen,
				 unsigned char * Src, unsigned long SrcLen, 
				 unsigned char * UserID, unsigned long lenUID,
				 unsigned char * szPubkey_XY, unsigned long ulPubkey_XYLen);

/*
 * instruction : sm3 kdf
 * param:
 * @kdfOutBuff : [out] : output
 * @Z_in, @ulZlen : [in] : input data to proceed
 * @klen : [in] : output kdf data length 
 * return :
 * 0 : success
 * other errcode : operation failed
 */
SM2_DLL_API int KDFwithSm3(unsigned char * kdfOutBuff, unsigned char * Z_in, unsigned long ulZlen, unsigned long klen );

/*
 * instruction : GM sm2 encryption
 * param:
 * @encData, @ulEncDataLen: [out] : encrypted data to output
 * @plain, @plainLen : [in] : input data to proceed
 * @szPubkey_XY, @ul_PubkXY_len : [in] : pubkey point (XA,YA) -> [X||Y] --total 64 byte
 * return :
 * 0 : success
 * other errcode : operation failed
 */
SM2_DLL_API int GM_SM2Encrypt(unsigned char * encData, unsigned long * ulEncDataLen, unsigned char * plain, unsigned long plainLen,
				  unsigned char * szPubkey_XY, unsigned long ul_PubkXY_len);
/*
 * instruction : GM sm2 decryption
 * param:
 * @DecData, @ulDecDataLen: [out] : decrypted data to output
 * @input, @inlen : [in] : input data to proceed
 * @pri_dA, @ulPri_dALen : [in] : private key data
 * return :
 * 0 : success
 * other errcode : operation failed
 */
SM2_DLL_API int GM_SM2Decrypt(unsigned char * DecData, unsigned long * ulDecDataLen, unsigned char * input, unsigned long inlen, 
				  unsigned char * pri_dA, unsigned long ulPri_dALen);

#ifdef __cplusplus
};
#endif
#endif   //____SM_2__H__45AE147B_76D4_4DB9_8FBB_00FE54E5EA79__
