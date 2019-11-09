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

#ifndef OPENSSL_COMPAT_H
#define OPENSSL_COMPAT_H

#include <openssl/opensslv.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#if !defined(HAVE_RSA_GET0_KEY) && defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x10100000L
#define HAVE_RSA_GET0_KEY
#define HAVE_RSA_SET0_KEY
#define EVP_MD_CTX_cleanup EVP_MD_CTX_free
#endif

#ifndef HAVE_RSA_GET0_KEY
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
#endif

#ifndef HAVE_RSA_SET0_KEY
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
#endif

#endif /* _OPENSSL_COMPAT_H */

