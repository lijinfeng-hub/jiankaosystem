#include "LogManager.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>

void LogManager::setLogFile(const QString& path) {
    m_logFile = path;
}

QString LogManager::logFilePath() const {
    return m_logFile;
}

bool LogManager::ensureReady() {
    if (m_logFile.isEmpty()) return false;

    const QFileInfo fi(m_logFile);
    QDir().mkpath(fi.dir().absolutePath());

    if (!QFile::exists(m_logFile)) {
        QFile f(m_logFile);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
        QTextStream out(&f);
        out << "# operations.log\n";
        out << "# format: [time] operator action details\n";
        f.close();
    }
    return true;
}

void LogManager::write(const model::OperationLog& log) {
    if (m_logFile.isEmpty()) return;
    QFile f(m_logFile);
    if (!f.open(QIODevice::Append | QIODevice::Text)) return;
    QTextStream out(&f);
    out << log.toLine() << "\n";
    f.close();
}

void LogManager::writeOp(const QString& operatorId, const QString& action, const QString& details) {
    model::OperationLog l;
    l.time = QDateTime::currentDateTime().toString(Qt::ISODate);
    l.operatorId = operatorId;
    l.action = action;
    l.details = details;
    write(l);
}
