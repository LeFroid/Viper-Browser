#ifndef _SQLITE_BADGE_H_
#define _SQLITE_BADGE_H_

namespace sqlite
{

/// Skeleton class, used to restrict the callers of a public
/// class method
template<typename T>
class Badge
{
    friend T;
    Badge() {}
};

}

#endif // _SQLITE_BADGE_H_

