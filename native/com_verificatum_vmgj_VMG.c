
/* Copyright 2008-2019 Douglas Wikstrom
 *
 * This file is part of Verificatum Multiplicative Groups library for
 * Java (VMGJ).
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <gmp.h>
#include "gmpmee.h"
#include "convert.h"
#include <stdio.h>
/*
 * We use compiler flags that enforce that unused variables are
 * flagged as errors. Here we are forced to use a given API, so we
 * need to explicitly trick the compiler to not issue an error for
 * those parameters that we do not use.
 */
#define VMGJ_UNUSED(x) ((void)(x))
#define VMGJ_JLONG_FROM_PTR(ptr) ((jlong)(intptr_t)(ptr))
#define VMGJ_PTR_FROM_JLONG(type, value) ((type *)(intptr_t)(value))

static int
vmgj_throw_oom(JNIEnv *env, const char *message)
{
  jclass exceptionClass = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
  if (exceptionClass != NULL)
    {
      (*env)->ThrowNew(env, exceptionClass, message);
      (*env)->DeleteLocalRef(env, exceptionClass);
    }
  return 0;
}

static int
vmgj_throw_illegal_state(JNIEnv *env, const char *message)
{
  jclass exceptionClass = (*env)->FindClass(env, "java/lang/IllegalStateException");
  if (exceptionClass != NULL)
    {
      (*env)->ThrowNew(env, exceptionClass, message);
      (*env)->DeleteLocalRef(env, exceptionClass);
    }
  return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

  /*
   * Class:     com_verificatum_vmgj_VMG
   * Method:    powm
   * Signature: ([B[B[B)[B
   */
  JNIEXPORT jbyteArray JNICALL Java_com_verificatum_vmgj_VMG_powm
  (JNIEnv *env, jclass clazz, jbyteArray javaBasis, jbyteArray javaExponent,
   jbyteArray javaModulus)
  {

    mpz_t basis;
    mpz_t exponent;
    mpz_t modulus;
    mpz_t result;
    int basis_init = 0;
    int exponent_init = 0;
    int modulus_init = 0;
    int result_init = 0;

    jbyteArray javaResult = NULL;

    VMGJ_UNUSED(clazz);

    /* Translate jbyteArray-parameters to their corresponding GMP
       mpz_t-elements. */
    basis_init = jbyteArray_to_mpz_t(env, &basis, javaBasis);
    if (!basis_init)
      {
        goto powm_cleanup;
      }
    exponent_init = jbyteArray_to_mpz_t(env, &exponent, javaExponent);
    if (!exponent_init)
      {
        goto powm_cleanup;
      }
    modulus_init = jbyteArray_to_mpz_t(env, &modulus, javaModulus);
    if (!modulus_init)
      {
        goto powm_cleanup;
      }

    /* Compute modular exponentiation. */
    mpz_init(result);
    result_init = 1;

    mpz_powm(result, basis, exponent, modulus);

    /* Translate result back to jbyteArray (this also allocates the
       result array on the JVM heap). */
    if (!mpz_t_to_jbyteArray(env, &javaResult, result))
      {
        goto powm_cleanup;
      }

    /* Deallocate resources. */
  powm_cleanup:
    if (result_init)
      {
        mpz_clear(result);
      }
    if (modulus_init)
      {
        mpz_clear(modulus);
      }
    if (exponent_init)
      {
        mpz_clear(exponent);
      }
    if (basis_init)
      {
        mpz_clear(basis);
      }

    return javaResult;
  }


  /*
   * Class:     com_verificatum_vmgj_VMG
   * Method:    modmul
   * Signature: ([B[B[B)[B
   */
  JNIEXPORT jbyteArray JNICALL Java_com_verificatum_vmgj_VMG_modmul
  (JNIEnv *env, jclass clazz, jbyteArray javaA, jbyteArray javaB,
   jbyteArray javaModulus)
  {
    mpz_t a;
    mpz_t b;
    mpz_t modulus;
    mpz_t result;
    int a_init = 0;
    int b_init = 0;
    int modulus_init = 0;
    int result_init = 0;
    jbyteArray javaResult = NULL;

    VMGJ_UNUSED(clazz);

    /* Convert inputs to GMP integers. */
    a_init = jbyteArray_to_mpz_t(env, &a, javaA);
    if (!a_init)
      {
        goto modmul_cleanup;
      }
    b_init = jbyteArray_to_mpz_t(env, &b, javaB);
    if (!b_init)
      {
        goto modmul_cleanup;
      }
    modulus_init = jbyteArray_to_mpz_t(env, &modulus, javaModulus);
    if (!modulus_init)
      {
        goto modmul_cleanup;
      }

    /* Compute (a * b) mod modulus. */
    mpz_init(result);
    result_init = 1;
    mpz_mul(result, a, b);
    mpz_mod(result, result, modulus);

    /* Convert result back to Java byte[]. */
    if (!mpz_t_to_jbyteArray(env, &javaResult, result))
      {
        goto modmul_cleanup;
      }

    /* Cleanup. */
  modmul_cleanup:
    if (result_init)
      {
        mpz_clear(result);
      }
    if (modulus_init)
      {
        mpz_clear(modulus);
      }
    if (b_init)
      {
        mpz_clear(b);
      }
    if (a_init)
      {
        mpz_clear(a);
      }

    return javaResult;
  }


  /*
   * Class:     com_verificatum_vmgj_VMG
   * Method:    spowm
   * Signature: ([[B[[B[B)[B
   */
  JNIEXPORT jbyteArray JNICALL Java_com_verificatum_vmgj_VMG_spowm
  (JNIEnv *env, jclass clazz, jobjectArray javaBases,
   jobjectArray javaExponents, jbyteArray javaModulus)
  {

    int i;
    mpz_t *bases = NULL;
    mpz_t *exponents = NULL;
    mpz_t modulus;
    mpz_t result;
    int bases_init = 0;
    int exponents_init = 0;
    int modulus_init = 0;
    int result_init = 0;

    jbyteArray javaResult = NULL;
    jbyteArray javaBase;
    jbyteArray javaExponent;

    /* Extract number of bases/exponents. */
    jsize numberOfBases = (*env)->GetArrayLength(env, javaBases);
    if ((*env)->ExceptionCheck(env))
      {
        return NULL;
      }

    VMGJ_UNUSED(clazz);

    /* Convert exponents represented as array of byte[] to array of
       mpz_t. */
    bases = gmpmee_array_alloc(numberOfBases);
    if (bases == NULL)
      {
        vmgj_throw_oom(env, "gmpmee_array_alloc() failed for bases");
        goto spowm_cleanup;
      }
    for (i = 0; i < numberOfBases; i++)
      {
        javaBase = (jbyteArray)(*env)->GetObjectArrayElement(env, javaBases, i);
        if ((*env)->ExceptionCheck(env))
          {
            goto spowm_cleanup;
          }
        if (!jbyteArray_to_mpz_t(env, &(bases[i]), javaBase))
          {
            if (javaBase != NULL)
              {
                (*env)->DeleteLocalRef(env, javaBase);
              }
            goto spowm_cleanup;
          }
        bases_init++;
        if (javaBase != NULL)
          {
            (*env)->DeleteLocalRef(env, javaBase);
          }
      }

    /* Convert exponents represented as array of byte[] to an array of
       mpz_t. */
    exponents = gmpmee_array_alloc(numberOfBases);
    if (exponents == NULL)
      {
        vmgj_throw_oom(env, "gmpmee_array_alloc() failed for exponents");
        goto spowm_cleanup;
      }
    for (i = 0; i < numberOfBases; i++)
      {
        javaExponent =
          (jbyteArray)(*env)->GetObjectArrayElement(env, javaExponents, i);
        if ((*env)->ExceptionCheck(env))
          {
            goto spowm_cleanup;
          }
        if (!jbyteArray_to_mpz_t(env, &(exponents[i]), javaExponent))
          {
            if (javaExponent != NULL)
              {
                (*env)->DeleteLocalRef(env, javaExponent);
              }
            goto spowm_cleanup;
          }
        exponents_init++;
        if (javaExponent != NULL)
          {
            (*env)->DeleteLocalRef(env, javaExponent);
          }
      }

    /* Convert modulus represented as a byte[] to a mpz_t. */
    modulus_init = jbyteArray_to_mpz_t(env, &modulus, javaModulus);
    if (!modulus_init)
      {
        goto spowm_cleanup;
      }

    /* Call GMP's exponentiated product function. */
    mpz_init(result);
    result_init = 1;
    gmpmee_spowm(result, bases, exponents, numberOfBases, modulus);

    /* Convert result to a jbyteArray. */
    if (!mpz_t_to_jbyteArray(env, &javaResult, result))
      {
        goto spowm_cleanup;
      }

    /* Deallocate resources. */
  spowm_cleanup:
    if (result_init)
      {
        mpz_clear(result);
      }
    if (modulus_init)
      {
        mpz_clear(modulus);
      }
    if (exponents != NULL)
      {
        gmpmee_array_clear_dealloc(exponents, exponents_init);
      }
    if (bases != NULL)
      {
        gmpmee_array_clear_dealloc(bases, bases_init);
      }

    return javaResult;
  }


  /*
   * Class:     com_verificatum_vmgj_VMG
   * Method:    fpowm_precomp
   * Signature: ([B[BII)J
   */
  JNIEXPORT jlong JNICALL Java_com_verificatum_vmgj_VMG_fpowm_1precomp
  (JNIEnv *env, jclass clazz, jbyteArray javaBasis, jbyteArray javaModulus,
   jint javaBlockWidth, jint javaExponentBitlen)
  {
    mpz_t basis;
    mpz_t modulus;
    int basis_init = 0;
    int modulus_init = 0;
    gmpmee_fpowm_tab *tablePtr =
      (gmpmee_fpowm_tab *)malloc(sizeof(gmpmee_fpowm_tab));

    if (tablePtr == NULL)
      {
        vmgj_throw_oom(env, "malloc() failed for fpowm table");
        return 0;
      }

    VMGJ_UNUSED(clazz);

    basis_init = jbyteArray_to_mpz_t(env, &basis, javaBasis);
    if (!basis_init)
      {
        goto fpowm_precomp_cleanup;
      }
    modulus_init = jbyteArray_to_mpz_t(env, &modulus, javaModulus);
    if (!modulus_init)
      {
        goto fpowm_precomp_cleanup;
      }

    gmpmee_fpowm_init_precomp(*tablePtr, basis, modulus,
                              (int)javaBlockWidth, (int)javaExponentBitlen);
    mpz_clear(modulus);
    modulus_init = 0;
    mpz_clear(basis);
    basis_init = 0;

    return VMGJ_JLONG_FROM_PTR(tablePtr);

  fpowm_precomp_cleanup:
    if (modulus_init)
      {
        mpz_clear(modulus);
      }
    if (basis_init)
      {
        mpz_clear(basis);
      }
    free(tablePtr);
    return 0;
  }


  /*
   * Class:     com_verificatum_vmgj_VMG
   * Method:    fpowm
   * Signature: (J[B)[B
   */
  JNIEXPORT jbyteArray JNICALL Java_com_verificatum_vmgj_VMG_fpowm
  (JNIEnv *env, jclass clazz, jlong javaTablePtr, jbyteArray javaExponent)
  {
    mpz_t exponent;
    mpz_t result;
    int exponent_init = 0;
    int result_init = 0;

    jbyteArray javaResult = NULL;

    VMGJ_UNUSED(clazz);
    if (javaTablePtr == 0)
      {
        vmgj_throw_illegal_state(env, "Invalid native handle in fpowm");
        return NULL;
      }

    exponent_init = jbyteArray_to_mpz_t(env, &exponent, javaExponent);
    if (!exponent_init)
      {
        return NULL;
      }
    mpz_init(result);
    result_init = 1;

    gmpmee_fpowm(result,
          *VMGJ_PTR_FROM_JLONG(gmpmee_fpowm_tab, javaTablePtr),
          exponent);

    /* Translate result back to jbyteArray (this also allocates the
       result array on the JVM heap). */
    if (!mpz_t_to_jbyteArray(env, &javaResult, result))
      {
        goto fpowm_cleanup;
      }

  fpowm_cleanup:
    if (result_init)
      {
        mpz_clear(result);
      }
    if (exponent_init)
      {
        mpz_clear(exponent);
      }

    return javaResult;
  }


  /*
   * Class:     com_verificatum_vmgj_VMG
   * Method:    fpowm_clear
   * Signature: (J)V
   */
  JNIEXPORT void JNICALL Java_com_verificatum_vmgj_VMG_fpowm_1clear
  (JNIEnv *env, jclass clazz, jlong javaTablePtr)
  {
    VMGJ_UNUSED(env);
    VMGJ_UNUSED(clazz);
    if (javaTablePtr == 0)
      {
        vmgj_throw_illegal_state(env, "Invalid native handle in fpowm_clear");
        return;
      }
    gmpmee_fpowm_clear(*VMGJ_PTR_FROM_JLONG(gmpmee_fpowm_tab, javaTablePtr));
  }


  /*
   * Class:     com_verificatum_vmgj_VMG
   * Method:    legendre
   * Signature: ([B[B)I
   */
  JNIEXPORT jint JNICALL Java_com_verificatum_vmgj_VMG_legendre
  (JNIEnv *env, jclass clazz, jbyteArray javaOp, jbyteArray javaOddPrime)
  {
    mpz_t op;
    mpz_t oddPrime;
    int symbol;
    int op_init = 0;
    int odd_prime_init = 0;

    VMGJ_UNUSED(clazz);

    op_init = jbyteArray_to_mpz_t(env, &op, javaOp);
    if (!op_init)
      {
        return 0;
      }
    odd_prime_init = jbyteArray_to_mpz_t(env, &oddPrime, javaOddPrime);
    if (!odd_prime_init)
      {
        mpz_clear(op);
        return 0;
      }

    symbol = mpz_legendre(op, oddPrime);

    if (op_init)
      {
        mpz_clear(op);
      }
    if (odd_prime_init)
      {
        mpz_clear(oddPrime);
      }

    return (jint)symbol;
  }

  /*
   * Class:     com_verificatum_vmgj_VMG
   * Method:    millerrabin_init
   * Signature: ([BZ)J
   */
  JNIEXPORT jlong JNICALL Java_com_verificatum_vmgj_VMG_millerrabin_1init
  (JNIEnv *env, jclass clazz, jbyteArray javaN, jboolean search)
  {
    mpz_t n;
    gmpmee_millerrabin_state *statePtr = (void*)0;
    int n_init = 0;

    VMGJ_UNUSED(clazz);

    n_init = jbyteArray_to_mpz_t(env, &n, javaN);
    if (!n_init)
      {
        return 0;
      }

    if (search || gmpmee_millerrabin_trial(n)) {
      statePtr =
        (gmpmee_millerrabin_state *)malloc(sizeof(gmpmee_millerrabin_state));
      if (statePtr == NULL)
        {
          mpz_clear(n);
          vmgj_throw_oom(env, "malloc() failed for Miller-Rabin state");
          return 0;
        }
      gmpmee_millerrabin_init(*statePtr, n);
    }
    if (search) {
      gmpmee_millerrabin_next_cand(*statePtr);
    }

    mpz_clear(n);

    return VMGJ_JLONG_FROM_PTR(statePtr);
  }


  /*
   * Class:     com_verificatum_vmgj_VMG
   * Method:    millerrabin_next_cand
   * Signature: (J)V
   */
  JNIEXPORT void JNICALL Java_com_verificatum_vmgj_VMG_millerrabin_1next_1cand
  (JNIEnv *env, jclass clazz, jlong javaStatePtr)
  {
    VMGJ_UNUSED(env);
    VMGJ_UNUSED(clazz);
    if (javaStatePtr == 0)
      {
        vmgj_throw_illegal_state(env,
                                 "Invalid native handle in millerrabin_next_cand");
        return;
      }
    gmpmee_millerrabin_next_cand(
      *VMGJ_PTR_FROM_JLONG(gmpmee_millerrabin_state, javaStatePtr));
  }


  /*
   * Class:     com_verificatum_vmgj_VMG
   * Method:    millerrabin_once
   * Signature: (J[B)I
   */
  JNIEXPORT jint JNICALL Java_com_verificatum_vmgj_VMG_millerrabin_1once
  (JNIEnv *env, jclass clazz, jlong javaStatePtr, jbyteArray javaBase)
  {
    mpz_t base;
    int res;
    int base_init = 0;

    VMGJ_UNUSED(clazz);
    if (javaStatePtr == 0)
      {
        vmgj_throw_illegal_state(env, "Invalid native handle in millerrabin_once");
        return 0;
      }

    base_init = jbyteArray_to_mpz_t(env, &base, javaBase);
    if (!base_init)
      {
        return 0;
      }
    res = gmpmee_millerrabin_once(
      *VMGJ_PTR_FROM_JLONG(gmpmee_millerrabin_state, javaStatePtr),
      base);

    mpz_clear(base);

    return res;
  }


  /*
   * Class:     com_verificatum_vmgj_VMG
   * Method:    millerrabin_clear
   * Signature: (J)V
   */
  JNIEXPORT void JNICALL Java_com_verificatum_vmgj_VMG_millerrabin_1clear
  (JNIEnv *env, jclass clazz, jlong javaStatePtr)
  {
    VMGJ_UNUSED(env);
    VMGJ_UNUSED(clazz);
    if (javaStatePtr == 0)
      {
        vmgj_throw_illegal_state(env, "Invalid native handle in millerrabin_clear");
        return;
      }
    gmpmee_millerrabin_clear(
      *VMGJ_PTR_FROM_JLONG(gmpmee_millerrabin_state, javaStatePtr));
  }


  /*
   * Class:     com_verificatum_vmgj_VMG
   * Method:    millerrabin_current
   * Signature: (J)[B
   */
  JNIEXPORT jbyteArray JNICALL
  Java_com_verificatum_vmgj_VMG_millerrabin_1current
  (JNIEnv *env, jclass clazz, jlong javaStatePtr)
  {
    jbyteArray javaResult = NULL;

    VMGJ_UNUSED(env);
    VMGJ_UNUSED(clazz);
    if (javaStatePtr == 0)
      {
        vmgj_throw_illegal_state(env, "Invalid native handle in millerrabin_current");
        return NULL;
      }

    if (!mpz_t_to_jbyteArray(env, &javaResult,
                             (*VMGJ_PTR_FROM_JLONG(gmpmee_millerrabin_state,
                                                   javaStatePtr))->n))
      {
        return NULL;
      }
    return javaResult;
  }


  /*
   * Class:     com_verificatum_vmgj_VMG
   * Method:    millerrabin_safe_init
   * Signature: ([BZ)J
   */
  JNIEXPORT jlong JNICALL Java_com_verificatum_vmgj_VMG_millerrabin_1safe_1init
  (JNIEnv *env, jclass clazz, jbyteArray javaN, jboolean search)
  {
    mpz_t n;
    gmpmee_millerrabin_safe_state *statePtr = (void*)0;
    int n_init = 0;

    VMGJ_UNUSED(clazz);

    n_init = jbyteArray_to_mpz_t(env, &n, javaN);
    if (!n_init)
      {
        return 0;
      }

    if (search || gmpmee_millerrabin_safe_trial(n)) {
      statePtr = (gmpmee_millerrabin_safe_state *)
        malloc(sizeof(gmpmee_millerrabin_safe_state));
      if (statePtr == NULL)
        {
          mpz_clear(n);
          vmgj_throw_oom(env, "malloc() failed for safe Miller-Rabin state");
          return 0;
        }
      gmpmee_millerrabin_safe_init(*statePtr, n);
    }
    if (search) {
      gmpmee_millerrabin_safe_next_cand(*statePtr);
    }

    mpz_clear(n);

    return VMGJ_JLONG_FROM_PTR(statePtr);
  }


  /*
   * Class:     com_verificatum_vmgj_VMG
   * Method:    millerrabin_safe_next_cand
   * Signature: (J)V
   */
  JNIEXPORT void JNICALL
  Java_com_verificatum_vmgj_VMG_millerrabin_1safe_1next_1cand
  (JNIEnv *env, jclass clazz, jlong javaStatePtr)
  {
    VMGJ_UNUSED(env);
    VMGJ_UNUSED(clazz);
    if (javaStatePtr == 0)
      {
        vmgj_throw_illegal_state(env,
                                 "Invalid native handle in millerrabin_safe_next_cand");
        return;
      }
    gmpmee_millerrabin_safe_next_cand(
      *VMGJ_PTR_FROM_JLONG(gmpmee_millerrabin_safe_state, javaStatePtr));
  }


  /*
   * Class:     com_verificatum_vmgj_VMG
   * Method:    millerrabin_safe_once
   * Signature: (J[BI)I
   */
  JNIEXPORT jint JNICALL Java_com_verificatum_vmgj_VMG_millerrabin_1safe_1once
  (JNIEnv *env, jclass clazz, jlong javaStatePtr, jbyteArray javaBase,
   jint javaIndex) {

    mpz_t base;
    int res;
    int base_init = 0;

    VMGJ_UNUSED(clazz);
    if (javaStatePtr == 0)
      {
        vmgj_throw_illegal_state(env,
                                 "Invalid native handle in millerrabin_safe_once");
        return 0;
      }

    base_init = jbyteArray_to_mpz_t(env, &base, javaBase);
    if (!base_init)
      {
        return 0;
      }

    if (((int)javaIndex) % 2 == 0)
      {
        res = gmpmee_millerrabin_once((*VMGJ_PTR_FROM_JLONG(
                                        gmpmee_millerrabin_safe_state,
                                        javaStatePtr))->nstate,
                                      base);
      }
    else
      {
        res = gmpmee_millerrabin_once((*VMGJ_PTR_FROM_JLONG(
                                        gmpmee_millerrabin_safe_state,
                                        javaStatePtr))->mstate,
                                      base);
      }

    mpz_clear(base);

    return res;
  }


  /*
   * Class:     com_verificatum_vmgj_VMG
   * Method:    millerrabin_safe_clear
   * Signature: (J)V
   */
  JNIEXPORT void JNICALL Java_com_verificatum_vmgj_VMG_millerrabin_1safe_1clear
  (JNIEnv *env, jclass clazz, jlong javaStatePtr)
  {
    VMGJ_UNUSED(env);
    VMGJ_UNUSED(clazz);
    if (javaStatePtr == 0)
      {
        vmgj_throw_illegal_state(env,
                                 "Invalid native handle in millerrabin_safe_clear");
        return;
      }
    gmpmee_millerrabin_safe_clear(
      *VMGJ_PTR_FROM_JLONG(gmpmee_millerrabin_safe_state, javaStatePtr));
  }


  /*
   * Class:     com_verificatum_vmgj_VMG
   * Method:    millerrabin_current_safe
   * Signature: (J)[B
   */
  JNIEXPORT jbyteArray JNICALL
  Java_com_verificatum_vmgj_VMG_millerrabin_1current_1safe
  (JNIEnv *env, jclass clazz, jlong javaStatePtr)
  {
    jbyteArray javaResult = NULL;

    VMGJ_UNUSED(clazz);
    if (javaStatePtr == 0)
      {
        vmgj_throw_illegal_state(env,
                                 "Invalid native handle in millerrabin_current_safe");
        return NULL;
      }
    if (!mpz_t_to_jbyteArray(env, &javaResult,
                             (*VMGJ_PTR_FROM_JLONG(gmpmee_millerrabin_safe_state,
                                                   javaStatePtr))->nstate->n))
      {
        return NULL;
      }
    return javaResult;
  }

#ifdef __cplusplus
}
#endif
