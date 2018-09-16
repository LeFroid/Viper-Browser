#ifndef CREDENTIALSTOREIMPL_H
#define CREDENTIALSTOREIMPL_H

#ifdef USE_KWALLET

#include "CredentialStoreKWallet.h"
using CredentialStoreImpl = CredentialStoreKWallet;

#else

using CredentialStoreImpl = std::nullptr_t;

#endif

#endif // CREDENTIALSTOREIMPL_H
