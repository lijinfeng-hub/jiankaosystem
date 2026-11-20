#pragma once

#include <QList>
#include <QString>

#include "model/Models.h"

class AuthManager {
public:
    void setUsersFile(const QString& path);

    bool load();
    bool save() const;

    bool registerUser(const QString& username, const QString& password, bool isAdmin, QString* err);
    bool login(const QString& username, const QString& password, QString* err);

    void logout();
    bool isLoggedIn() const;
    model::User currentUser() const;

    QList<model::User> users() const;

private:
    QString m_usersFile;
    QList<model::User> m_users;
    bool m_loggedIn = false;
    model::User m_current;

    static QString hashPassword(const QString& password);
    model::User* findUser(const QString& username);
};
