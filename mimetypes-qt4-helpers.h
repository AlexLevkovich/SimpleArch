#ifndef MIMETYPES_QT4_HELPERS_H
#define MIMETYPES_QT4_HELPERS_H

#include <QtGlobal>

#ifndef Q_DECL_NOEXCEPT
#define Q_DECL_NOEXCEPT
#endif

Q_CORE_EXPORT bool qEnvironmentVariableIsEmpty(const char *varName) Q_DECL_NOEXCEPT;
Q_CORE_EXPORT bool qEnvironmentVariableIsSet(const char *varName) Q_DECL_NOEXCEPT;

#endif // MIMETYPES_QT4_HELPERS_H
