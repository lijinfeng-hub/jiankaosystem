#include "AppContext.h"

#include <QCoreApplication>
#include <QDir>

bool AppContext::initialize() {
    const QString base = QCoreApplication::applicationDirPath();

    // 数据与日志放在可写目录：优先使用当前工作目录下的 data/logs
    //（教学场景下更直观，方便查看 JSON）
    const QString cwd = QDir::currentPath();
    m_dataDir = QDir(cwd).filePath("data");
    m_logsDir = QDir(cwd).filePath("logs");

    QDir().mkpath(m_dataDir);
    QDir().mkpath(m_logsDir);

    m_log.setLogFile(QDir(m_logsDir).filePath("operations.log"));
    m_auth.setUsersFile(QDir(m_dataDir).filePath("users.json"));
    m_data.setTeachersFile(QDir(m_dataDir).filePath("teachers.json"));
    m_data.setExamsFile(QDir(m_dataDir).filePath("exams.json"));
    m_data.setSchedulesFile(QDir(m_dataDir).filePath("schedules.json"));
    m_data.setLogManager(&m_log);

    // 初始化顺序：日志 -> 用户 -> 数据
    if (!m_log.ensureReady()) {
        return false;
    }
    if (!m_auth.load()) {
        return false;
    }
    if (!m_data.load()) {
        return false;
    }

    return true;
}

AuthManager& AppContext::auth() { return m_auth; }
DataManager& AppContext::data() { return m_data; }
LogManager& AppContext::log() { return m_log; }

QString AppContext::dataDir() const { return m_dataDir; }
QString AppContext::logsDir() const { return m_logsDir; }
