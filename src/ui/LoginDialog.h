#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QFormLayout>

class AppContext;

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(AppContext* ctx, QWidget* parent = nullptr);

private slots:
    void toggleMode();
    void onLoginOrRegisterClicked();

private:
    void setHint(const QString& text, bool ok);

    AppContext* m_ctx;
    bool m_isRegisterMode = false;
    QPushButton* m_switchBtn = nullptr;
    QWidget* m_cardWidget = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLineEdit* m_userEdit = nullptr;
    QLineEdit* m_passEdit = nullptr;
    QLineEdit* m_pass2Edit = nullptr;
    QLabel* m_hintLabel = nullptr;
    QPushButton* m_loginBtn = nullptr;
    QFormLayout* m_formLayout = nullptr;
};

#endif // LOGINDIALOG_H
