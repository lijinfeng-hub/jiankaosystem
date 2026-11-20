#pragma once

#include <QString>

#include "model/Models.h"

class LogManager {
public:
    void setLogFile(const QString& path);
    bool ensureReady();

    void write(const model::OperationLog& log);
    void writeOp(const QString& operatorId, const QString& action, const QString& details);

    QString logFilePath() const;

private:
    QString m_logFile;
};
