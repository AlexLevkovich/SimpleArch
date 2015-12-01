#include "mimetypes-qt4-helpers.h"

/*!
    \relates <QtGlobal>
    \since 5.1

    Returns whether the environment variable \a varName is empty.

    Equivalent to
    \code
    qgetenv(varName).isEmpty()
    \endcode
    except that it's potentially much faster, and can't throw exceptions.

    \sa qgetenv(), qEnvironmentVariableIsSet()
*/    
bool qEnvironmentVariableIsEmpty(const char *varName) Q_DECL_NOEXCEPT
{
#if defined(_MSC_VER) && _MSC_VER >= 1400
    // we provide a buffer that can only hold the empty string, so
    // when the env.var isn't empty, we'll get an ERANGE error (buffer
    // too small):
    size_t dummy;
    char buffer = '\0';
    return getenv_s(&dummy, &buffer, 1, varName) != ERANGE;
#else
    const char * const value = ::getenv(varName);
    return !value || !*value;
#endif
}

/*!    
    \relates <QtGlobal>
    \since 5.1

    Returns whether the environment variable \a varName is set.

    Equivalent to
    \code
    !qgetenv(varName).isNull()
    \endcode
    except that it's potentially much faster, and can't throw exceptions.

    \sa qgetenv(), qEnvironmentVariableIsEmpty()
*/
bool qEnvironmentVariableIsSet(const char *varName) Q_DECL_NOEXCEPT
{
#if defined(_MSC_VER) && _MSC_VER >= 1400
    size_t requiredSize = 0;
    (void)getenv_s(&requiredSize, 0, 0, varName);
    return requiredSize != 0;
#else
    return ::getenv(varName) != 0;
#endif
}
