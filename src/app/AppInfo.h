#pragma once

#include <QString>

namespace appinfo {

inline QString name() {
    return QStringLiteral("智能排监考管理系统");
}

inline QString version() {
    return QStringLiteral("V1.0");
}

inline QString title() {
    return name() + QStringLiteral(" ") + version();
}

} // namespace appinfo
