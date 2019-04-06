/*
 * Copyright (c) 2005 Darren Tucker <dtucker@zip.com.au>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _OPENSSL_COMPAT_H
#define _OPENSSL_COMPAT_H

#include <openssl/opensslv.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>

int ssh_compatible_openssl(long, long);

#if OPENSSL_VERSION_NUMBER < 0x10000001L
# define LIBCRYPTO_EVP_INL_TYPE unsigned int
#else
# define LIBCRYPTO_EVP_INL_TYPE size_t
#endif

#ifndef OPENSSL_RSA_MAX_MODULUS_BITS
# define OPENSSL_RSA_MAX_MODULUS_BITS	16384
#endif
#ifndef OPENSSL_DSA_MAX_MODULUS_BITS
# define OPENSSL_DSA_MAX_MODULUS_BITS	10000
#endif

#ifndef OPENSSL_HAVE_EVPCTR
# define EVP_aes_128_ctr evp_aes_128_ctr
# define EVP_aes_192_ctr evp_aes_128_ctr
# define EVP_aes_256_ctr evp_aes_128_ctr
const EVP_CIPHER *evp_aes_128_ctr(void);
void ssh_aes_ctr_iv(EVP_CIPHER_CTX *, int, u_char *, size_t);
#endif

/* Avoid some #ifdef. Code that uses these is unreachable without GCM */
#if !defined(OPENSSL_HAVE_EVPGCM) && !defined(EVP_CTRL_GCM_SET_IV_FIXED)
# define EVP_CTRL_GCM_SET_IV_FIXED -1
# define EVP_CTRL_GCM_IV_GEN -1
# define EVP_CTRL_GCM_SET_TAG -1
# define EVP_CTRL_GCM_GET_TAG -1
#endif

/* Replace missing EVP_CIPHER_CTX_ctrl() with something that returns failure */
#ifndef HAVE_EVP_CIPHER_CTX_CTRL
# ifdef OPENSSL_HAVE_EVPGCM
#  error AES-GCM enabled without EVP_CIPHER_CTX_ctrl /* shouldn't happen */
# else
# define EVP_CIPHER_CTX_ctrl(a,b,c,d) (0)
# endif
#endif

#if defined(HAVE_EVP_RIPEMD160)
# if defined(OPENSSL_NO_RIPEMD) || defined(OPENSSL_NO_RMD160)
#  undef HAVE_EVP_RIPEMD160
# endif
#endif

/*
 * We overload some of the OpenSSL crypto functions with ssh_* equivalents
 * to automatically handle OpenSSL engine initialisation.
 *
 * In order for the compat library to call the real functions, it must
 * define SSH_DONT_OVERLOAD_OPENSSL_FUNCS before including this file and
 * implement the ssh_* equivalents.
 */
#ifndef SSH_DONT_OVERLOAD_OPENSSL_FUNCS

# ifdef USE_OPENSSL_ENGINE
#  ifdef OpenSSL_add_all_algorithms
#   undef OpenSSL_add_all_algorithms
#  endif
#  define OpenSSL_add_all_algorithms()  ssh_OpenSSL_add_all_algorithms()
# endif

void ssh_OpenSSL_add_all_algorithms(void);

#endif	/* SSH_DONT_OVERLOAD_OPENSSL_FUNCS */

/**
 * Get the RSA parameters
 *
 * @param rsa                 The RSA object
 * @param n                   The @c n parameter
 * @param e                   The @c e parameter
 * @param d                   The @c d parameter
 */
static inline void
RSA_get0_key(const RSA *rsa, const BIGNUM **n,
             const BIGNUM **e, const BIGNUM **d)
{
    if (n != NULL)
    {
        *n = rsa ? rsa->n : NULL;
    }
    if (e != NULL)
    {
        *e = rsa ? rsa->e : NULL;
    }
    if (d != NULL)
    {
        *d = rsa ? rsa->d : NULL;
    }
}

/**
 * Set the RSA parameters
 *
 * @param rsa                 The RSA object
 * @param n                   The @c n parameter
 * @param e                   The @c e parameter
 * @param d                   The @c d parameter
 * @return                    1 on success, 0 on error
 */
static inline int
RSA_set0_key(RSA *rsa, BIGNUM *n, BIGNUM *e, BIGNUM *d)
{
    if ((rsa->n == NULL && n == NULL)
        || (rsa->e == NULL && e == NULL))
    {
        return 0;
    }

    if (n != NULL)
    {
        BN_free(rsa->n);
        rsa->n = n;
    }
    if (e != NULL)
    {
        BN_free(rsa->e);
        rsa->e = e;
    }
    if (d != NULL)
    {
        BN_free(rsa->d);
        rsa->d = d;
    }

    return 1;
}

#endif /* _OPENSSL_COMPAT_H */

