#ifndef ECCP_CURVE_H
#define ECCP_CURVE_H

#include <aic_common.h>
#include "pke_common.h"

#ifdef __cplusplus
extern "C" {
#endif

//sample ecc GF(p) curve
/*
#define SUPPORT_BRAINPOOLP160R1
#define SUPPORT_BRAINPOOLP192R1
#define SUPPORT_BRAINPOOLP224R1
#define SUPPORT_BRAINPOOLP256R1
#define SUPPORT_BRAINPOOLP320R1
#define SUPPORT_BRAINPOOLP384R1
#define SUPPORT_BRAINPOOLP512R1
#define SUPPORT_SECP160R1
#define SUPPORT_SECP160R2
#define SUPPORT_SECP192R1
#define SUPPORT_SECP224R1
*/
#define SUPPORT_SECP256R1
/*
#define SUPPORT_SECP384R1
#if (AIC_PKE_ECCP_MAX_BIT_LEN >= 521)
#define SUPPORT_SECP521R1
#endif
#define SUPPORT_SECP160K1
#define SUPPORT_SECP192K1
#define SUPPORT_SECP224K1
#define SUPPORT_SECP256K1
#define SUPPORT_BN256
#if (AIC_PKE_ECCP_MAX_BIT_LEN >= 638)
#define SUPPORT_BN638
#endif
*/

// eccp curve struct
#ifdef PKE_HP
typedef struct
{
    u32 eccp_p_bitLen;        //bit length of prime p
    u32 eccp_n_bitLen;        //bit length of order n
    u32 *eccp_p;
    u32 *eccp_p_h;
    u32 *eccp_a;
    u32 *eccp_b;
    u32 *eccp_Gx;
    u32 *eccp_Gy;
    u32 *eccp_n;
    u32 *eccp_n_h;
    u32 *eccp_half_Gx;
    u32 *eccp_half_Gy;
} eccp_curve_t;
#else
typedef struct
{
    u32 eccp_p_bitLen;        //bit length of prime p
    u32 eccp_n_bitLen;        //bit length of order n
    u32 *eccp_p;              //prime p
    u32 *eccp_p_h;
    u32 *eccp_p_n0;
    u32 *eccp_a;
    u32 *eccp_b;
    u32 *eccp_Gx;
    u32 *eccp_Gy;
    u32 *eccp_n;              //order of curve or point(Gx,Gy)
    u32 *eccp_n_h;
    u32 *eccp_n_n0;
} eccp_curve_t;
#endif



#ifdef SUPPORT_BRAINPOOLP160R1
extern const eccp_curve_t brainpoolp160r1[1];
#endif

#ifdef SUPPORT_BRAINPOOLP192R1
extern const eccp_curve_t brainpoolp192r1[1];
#endif

#ifdef SUPPORT_BRAINPOOLP224R1
extern const eccp_curve_t brainpoolp224r1[1];
#endif

#ifdef SUPPORT_BRAINPOOLP256R1
extern const eccp_curve_t brainpoolp256r1[1];
#endif

#ifdef SUPPORT_BRAINPOOLP320R1
extern const eccp_curve_t brainpoolp320r1[1];
#endif

#ifdef SUPPORT_BRAINPOOLP384R1
extern const eccp_curve_t brainpoolp384r1[1];
#endif

#ifdef SUPPORT_BRAINPOOLP512R1
extern const eccp_curve_t brainpoolp512r1[1];
#endif

#ifdef SUPPORT_SECP160R1
extern const eccp_curve_t secp160r1[1];
#endif

#ifdef SUPPORT_SECP160R2
extern const eccp_curve_t secp160r2[1];
#endif

#ifdef SUPPORT_SECP192R1
extern const eccp_curve_t secp192r1[1];
#endif

#ifdef SUPPORT_SECP224R1
extern const eccp_curve_t secp224r1[1];
#endif

#ifdef SUPPORT_SECP256R1
extern const eccp_curve_t secp256r1[1];
#endif

#ifdef SUPPORT_SECP384R1
extern const eccp_curve_t secp384r1[1];
#endif

#ifdef SUPPORT_SECP521R1
extern const eccp_curve_t secp521r1[1];
#endif

#ifdef SUPPORT_SECP160K1
extern const eccp_curve_t secp160k1[1];
#endif

#ifdef SUPPORT_SECP192K1
extern const eccp_curve_t secp192k1[1];
#endif

#ifdef SUPPORT_SECP224K1
extern const eccp_curve_t secp224k1[1];
#endif

#ifdef SUPPORT_SECP256K1
extern const eccp_curve_t secp256k1[1];
#endif

#ifdef SUPPORT_BN256
extern const eccp_curve_t bn256[1];
#endif

#ifdef SUPPORT_BN638
extern const eccp_curve_t bn638[1];
#endif


/********* Curve25519 struct *********/
typedef struct
{
    u32 p_bitLen;        //bit length of prime p
    u32 n_bitLen;        //bit length of order n
    u32 *p;
    u32 *p_h;
#ifndef PKE_HP
    u32 *p_n0;
#endif
    u32 *a24;            //(A-2)/4
    u32 *u;
    u32 *v;
    u32 *n;              //order of curve or point(Gx,Gy)
    u32 *n_h;
#ifndef PKE_HP
    u32 *n_n0;
#endif
    u32 *h;
} mont_curve_t;


/********* Edward Curve 25519 struct *********/
typedef struct
{
    u32 p_bitLen;        //bit length of prime p
    u32 n_bitLen;        //bit length of order n
    u32 *p;
    u32 *p_h;
#ifndef PKE_HP
    u32 *p_n0;
#endif
    u32 *d;
    u32 *Gx;
    u32 *Gy;
    u32 *n;              //order of curve or point(Gx,Gy)
    u32 *n_h;
#ifndef PKE_HP
    u32 *n_n0;
#endif
    u32 *h;
} edward_curve_t;

#ifdef __cplusplus
}
#endif

#endif

