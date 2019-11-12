#ifndef _SQLITE_BLOB_H_
#define _SQLITE_BLOB_H_

#include <string>

namespace sqlite
{
    /// Wrapper for BLOB types. Used to differentiate itself from strings in the template-based API
    struct Blob
    {
        std::string data;
    };
}

#endif // _SQLITE_BLOB_H_

