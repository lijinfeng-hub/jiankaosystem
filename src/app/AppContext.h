#pragma once

#include <QString>

#include "services/AuthManager.h"
#include "services/DataManager.h"
#include "services/LogManager.h"

class AppContext {
public:
    bool initialize();

    AuthManager& auth();
    DataManager& data();
    LogManager& log();

    QString dataDir() const;
    QString logsDir() const;

private:
    QString m_dataDir;
    QString m_logsDir;

    LogManager m_log;
    AuthManager m_auth;
    DataManager m_data;
};
