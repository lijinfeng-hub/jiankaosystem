#include "AuthManager.h"

#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>

void AuthManager::setUsersFile(const QString& path) {
    m_usersFile = path;
}

static QByteArray readAll(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    return f.readAll();
}

static bool writeAll(const QString& path, const QByteArray& data) {
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    if (f.write(data) != data.size()) return false;
    return f.commit();
}

bool AuthManager::load() {
    m_users.clear();
    m_loggedIn = false;

    if (m_usersFile.isEmpty()) return false;

    if (!QFile::exists(m_usersFile)) {
        // 初始创建一个默认管理员：admin/admin
        model::User admin;
        admin.username = "admin";
        admin.passwordHash = hashPassword("admin");
        admin.isAdmin = true;
        m_users.push_back(admin);
        return save();
    }

    const auto bytes = readAll(m_usersFile);
    if (bytes.trimmed().isEmpty()) {
        qWarning().noquote() << "用户文件为空：" << m_usersFile;
        return false;
    }

    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning().noquote() << "用户文件解析失败：" << m_usersFile << parseError.errorString();
        return false;
    }

    const auto usersValue = doc.object().value("users");
    if (!usersValue.isArray()) {
        qWarning().noquote() << "用户文件缺少 users 数组：" << m_usersFile;
        return false;
    }

    const auto arr = usersValue.toArray();
    for (const auto& v : arr) {
        if (!v.isObject()) {
            qWarning().noquote() << "用户文件包含非法条目：" << m_usersFile;
            m_users.clear();
            return false;
        }
        m_users.push_back(model::User::fromJson(v.toObject()));
    }
    return true;
}

bool AuthManager::save() const {
    if (m_usersFile.isEmpty()) return false;

    QJsonArray arr;
    for (const auto& u : m_users) arr.append(u.toJson());

    QJsonObject root;
    root["users"] = arr;

    const auto doc = QJsonDocument(root);
    return writeAll(m_usersFile, doc.toJson(QJsonDocument::Indented));
}

model::User* AuthManager::findUser(const QString& username) {
    for (auto& u : m_users) {
        if (u.username == username) return &u;
    }
    return nullptr;
}

QString AuthManager::hashPassword(const QString& password) {
    const auto bytes = password.toUtf8();
    return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
}

bool AuthManager::registerUser(const QString& username, const QString& password, bool isAdmin, QString* err) {
    const QString u = username.trimmed();
    if (u.isEmpty()) {
        if (err) *err = "用户名不能为空";
        return false;
    }
    if (password.size() < 4) {
        if (err) *err = "密码至少4位";
        return false;
    }
    if (findUser(u)) {
        if (err) *err = "用户名已存在";
        return false;
    }

    model::User nu;
    nu.username = u;
    nu.passwordHash = hashPassword(password);
    nu.isAdmin = isAdmin;

    m_users.push_back(nu);
    if (!save()) {
        if (err) *err = "保存用户文件失败";
        return false;
    }
    return true;
}

bool AuthManager::login(const QString& username, const QString& password, QString* err) {
    const QString u = username.trimmed();
    auto* user = findUser(u);
    if (!user) {
        if (err) *err = "用户不存在";
        return false;
    }
    if (user->passwordHash != hashPassword(password)) {
        if (err) *err = "密码错误";
        return false;
    }
    m_loggedIn = true;
    m_current = *user;
    return true;
}

void AuthManager::logout() {
    m_loggedIn = false;
    m_current = model::User{};
}

bool AuthManager::isLoggedIn() const { return m_loggedIn; }

model::User AuthManager::currentUser() const { return m_current; }

QList<model::User> AuthManager::users() const { return m_users; }
