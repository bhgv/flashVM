/* OpenSSL verified TLS, using a custom BIO to preserve deadlines and local SIGPIPE handling. */
#include "tls.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>
#include <limits.h>
#include <stdlib.h>


/* No public/native socket representation leaks into the TLS SDK. */
struct FVM_NetTLS
{
    FVM_NetSocket *socket; /* Borrowed; the HTTP provider closes it after TLS cleanup. */
    SSL_CTX *context;      /* Owned per-request verification policy. */
    SSL *session;          /* Owns the attached custom BIO after SSL_set_bio. */
    BIO_METHOD *method;    /* Owned method, retained until all associated BIOs are freed. */
};


/* BIO has no independent socket ownership and no descriptor close side effects. */
static int tls_bio_create(BIO *bio)
{
    BIO_set_init(bio, 1);
    BIO_set_data(bio, 0);
    BIO_set_shutdown(bio, 0);
    return 1;
}


/* Socket ownership is deliberately outside the BIO lifecycle. */
static int tls_bio_destroy(BIO *bio)
{
    if (!bio)
    {
        return 0;
    }
    BIO_set_data(bio, 0);
    BIO_set_init(bio, 0);
    return 1;
}


/* Native adapter waits internally; do not advertise retryable BIO state after a deadline. */
static int tls_bio_read(BIO *bio, char *bytes, int size)
{
    size_t received;
    FVM_NetSocket *socket = (FVM_NetSocket *)BIO_get_data(bio);
    BIO_clear_retry_flags(bio);
    if (!bytes || size <= 0 || !fvm_net_read(socket, bytes, (size_t)size, &received))
    {
        return -1;
    }
    return (int)received;
}


/* All TLS records pass through the same MSG_NOSIGNAL/SO_NOSIGPIPE-aware native writer. */
static int tls_bio_write(BIO *bio, const char *bytes, int size)
{
    size_t sent;
    FVM_NetSocket *socket = (FVM_NetSocket *)BIO_get_data(bio);
    BIO_clear_retry_flags(bio);
    if (!bytes || size <= 0 || !fvm_net_write(socket, bytes, (size_t)size, &sent))
    {
        return -1;
    }
    return (int)sent;
}


/* No extra buffering or seek/pending semantics; only flush succeeds. */
static long tls_bio_ctrl(BIO *bio, int command, long number, void *pointer)
{
    (void)bio;
    (void)number;
    (void)pointer;
    return command == BIO_CTRL_FLUSH ? 1 : 0;
}


/* Establish verified TLS without disabling trust/name checks or changing global SDK ownership. */
FVM_NetTLS *fvm_net_tls_open(FVM_NetSocket *socket, const char *host, int numeric, const char *ca_file)
{
    FVM_NetTLS *tls;
    BIO *bio = 0;
    int method_index;
    if (!socket || !host || !*host || (ca_file && !*ca_file) || !fvm_net_active(socket))
    {
        return 0;
    }
    tls = (FVM_NetTLS *)calloc(1, sizeof(*tls));
    if (!tls)
    {
        return 0;
    }
    tls->socket = socket;
    ERR_clear_error();
    tls->context = SSL_CTX_new(TLS_client_method());
    if (!tls->context || !SSL_CTX_set_min_proto_version(tls->context, TLS1_2_VERSION))
    {
        goto failed;
    }
    SSL_CTX_set_verify(tls->context, SSL_VERIFY_PEER, 0);
    if (ca_file)
    {
        if (!SSL_CTX_load_verify_locations(tls->context, ca_file, 0))
        {
            goto failed;
        }
    }
    else if (!SSL_CTX_set_default_verify_paths(tls->context))
    {
        goto failed;
    }
    tls->session = SSL_new(tls->context);
    if (!tls->session)
    {
        goto failed;
    }
    SSL_set_hostflags(tls->session, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
    if (numeric)
    {
        if (!X509_VERIFY_PARAM_set1_ip_asc(SSL_get0_param(tls->session), host))
        {
            goto failed;
        }
    }
    else
    {
        if (!SSL_set1_host(tls->session, host) || !SSL_set_tlsext_host_name(tls->session, host))
        {
            goto failed;
        }
    }
    method_index = BIO_get_new_index();
    if (method_index < 0)
    {
        goto failed;
    }
    tls->method = BIO_meth_new(method_index | BIO_TYPE_SOURCE_SINK, "FlashVM deadline TCP");
    if (!tls->method || !BIO_meth_set_create(tls->method, tls_bio_create) ||
        !BIO_meth_set_destroy(tls->method, tls_bio_destroy) || !BIO_meth_set_read(tls->method, tls_bio_read) ||
        !BIO_meth_set_write(tls->method, tls_bio_write) || !BIO_meth_set_ctrl(tls->method, tls_bio_ctrl))
    {
        goto failed;
    }
    bio = BIO_new(tls->method);
    if (!bio)
    {
        goto failed;
    }
    BIO_set_data(bio, socket);
    SSL_set_bio(tls->session, bio, bio);
    bio = 0;
    ERR_clear_error();
    if (SSL_connect(tls->session) != 1 || SSL_get_verify_result(tls->session) != X509_V_OK || !fvm_net_active(socket))
    {
        goto failed;
    }
    return tls;
failed:
    BIO_free(bio);
    fvm_net_tls_close(tls);
    return 0;
}


/* Reject unauthenticated EOF; framed HTTP may finish before needing any EOF read. */
int fvm_net_tls_read(FVM_NetTLS *tls, void *bytes, size_t size, size_t *received)
{
    int count, error;
    if (!tls || !bytes || !size || !received || !fvm_net_active(tls->socket))
    {
        return 0;
    }
    if (size > INT_MAX)
    {
        size = INT_MAX;
    }
    ERR_clear_error();
    count = SSL_read(tls->session, bytes, (int)size);
    if (count > 0)
    {
        *received = (size_t)count;
        return 1;
    }
    error = SSL_get_error(tls->session, count);
    if (error == SSL_ERROR_ZERO_RETURN)
    {
        *received = 0;
        return 1;
    }
    return 0;
}


/* A partial TLS write advances only the reported bytes, under the original native deadline. */
int fvm_net_tls_write(FVM_NetTLS *tls, const void *bytes, size_t size, size_t *sent)
{
    int count;
    if (!tls || !bytes || !size || !sent || !fvm_net_active(tls->socket))
    {
        return 0;
    }
    if (size > INT_MAX)
    {
        size = INT_MAX;
    }
    ERR_clear_error();
    count = SSL_write(tls->session, bytes, (int)size);
    if (count <= 0)
    {
        return 0;
    }
    *sent = (size_t)count;
    return 1;
}


/* No SSL_shutdown network wait: Connection: close is one request, never a reused session. */
void fvm_net_tls_close(FVM_NetTLS *tls)
{
    if (!tls)
    {
        return;
    }
    SSL_free(tls->session);
    SSL_CTX_free(tls->context);
    BIO_meth_free(tls->method);
    free(tls);
}
