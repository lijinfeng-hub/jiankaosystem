#include "LoginDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFont>
#include<QFileInfo>
#include<QDir>
#include<QCoreApplication>
#include "app/AppContext.h"
#include "app/AppInfo.h"
static QIcon loadAppIcon() {
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QString iconPath = QDir(exeDir).filePath("./assets/app_icon.png");
    if (QFileInfo::exists(iconPath)) {
        const QIcon icon(iconPath);
        if (!icon.isNull()) return icon;
    }
    return {};
}
LoginDialog::LoginDialog(AppContext* ctx, QWidget* parent)
    : QDialog(parent), m_ctx(ctx), m_isRegisterMode(false) {
    setWindowTitle(appinfo::title());
    setObjectName("LoginDialog");
    setModal(true);
    resize(800, 500);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // 去掉问号

    // ============ 主布局：左右分栏 ============
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 左侧：品牌区域
    auto* leftWidget = new QWidget(this);
    leftWidget->setObjectName("LoginLeftPanel");
    leftWidget->setFixedWidth(300);

    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->addStretch();

    auto* logoLabel = new QLabel("排监考系统", leftWidget);
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setStyleSheet("color: white; font-size: 28px; font-weight: bold;");
    leftLayout->addWidget(logoLabel);

    auto* slogan = new QLabel("智能排考 · 高效监考", leftWidget);
    slogan->setAlignment(Qt::AlignCenter);
    slogan->setStyleSheet("color: rgba(255,255,255,0.9); font-size: 14px; margin-top: 10px;");
    leftLayout->addWidget(slogan);

    leftLayout->addStretch();

    // 右侧：登录卡片
    m_cardWidget = new QWidget(this);
    m_cardWidget->setObjectName("cardWidget");

    m_titleLabel = new QLabel("用户登录", m_cardWidget);
    m_titleLabel->setObjectName("titleLabel");

    // === 手动表单布局（替换 QFormLayout）===
    auto* formLayout = new QVBoxLayout();

    // 用户名行
    auto* userRow = new QHBoxLayout();
    auto* userLabel = new QLabel("用户名", m_cardWidget);
    userLabel->setStyleSheet("font-size: 14px; color: #333;");
    userLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    userLabel->setMinimumWidth(80);
    m_userEdit = new QLineEdit(m_cardWidget);
    userRow->addWidget(userLabel);
    userRow->addWidget(m_userEdit);
    formLayout->addLayout(userRow);

    // 密码行
    auto* passRow = new QHBoxLayout();
    auto* passLabel = new QLabel("密码", m_cardWidget);
    passLabel->setStyleSheet("font-size: 14px; color: #333;");
    passLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    passLabel->setMinimumWidth(80);
    m_passEdit = new QLineEdit(m_cardWidget);
    m_passEdit->setEchoMode(QLineEdit::Password);
    passRow->addWidget(passLabel);
    passRow->addWidget(m_passEdit);
    formLayout->addLayout(passRow);

    // 确认密码行（初始隐藏）
    auto* pass2Row = new QHBoxLayout();
    auto* pass2Label = new QLabel("确认密码", m_cardWidget);
    pass2Label->setStyleSheet("font-size: 14px; color: #333;");
    pass2Label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    pass2Label->setMinimumWidth(80);
    m_pass2Edit = new QLineEdit(m_cardWidget);
    m_pass2Edit->setEchoMode(QLineEdit::Password);
    m_pass2Edit->hide(); // 初始隐藏
    pass2Row->addWidget(pass2Label);
    pass2Row->addWidget(m_pass2Edit);
    formLayout->addLayout(pass2Row);

    m_hintLabel = new QLabel("默认管理员：admin / admin", m_cardWidget);
    m_hintLabel->setObjectName("hintLabel");

    m_loginBtn = new QPushButton("登录", m_cardWidget);
    m_loginBtn->setObjectName("loginBtn");
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginOrRegisterClicked);

    m_switchBtn = new QPushButton("没有账号？立即注册", m_cardWidget);
    m_switchBtn->setStyleSheet(R"(
    color: #ff7b87;
    font-size: 12px;
    border: none;
    background: transparent;
    text-decoration: underline;
    padding: 5px;
)");

    m_switchBtn->setCursor(Qt::PointingHandCursor);
    connect(m_switchBtn, &QPushButton::clicked, this, &LoginDialog::toggleMode);

    auto* cardLayout = new QVBoxLayout(m_cardWidget);
    cardLayout->addWidget(m_titleLabel);
    cardLayout->addLayout(formLayout);
    cardLayout->addWidget(m_hintLabel);
    cardLayout->addWidget(m_loginBtn);
    cardLayout->addWidget(m_switchBtn);
    cardLayout->addStretch();

    mainLayout->addWidget(leftWidget);
    mainLayout->addWidget(m_cardWidget);
    setLayout(mainLayout);

    // 回车快捷登录
    connect(m_passEdit, &QLineEdit::returnPressed, m_loginBtn, &QPushButton::click);
    connect(m_pass2Edit, &QLineEdit::returnPressed, m_loginBtn, &QPushButton::click);

    m_userEdit->setFocus();
    setStyleSheet(R"(
/* ================= 登录窗口整体 ================= */
QDialog#LoginDialog {
    background-color: #fffafc;
}

/* ================= 左侧面板 ================= */
#LoginLeftPanel {
    background: qlineargradient(
        x1:0, y1:0, x2:0, y2:1,
        stop:0 #a8e6cf,
        stop:1 #7fd1b9
    );
}

/* 左侧标题 */
#LoginLeftPanel QLabel {
    color: white;
}

/* ================= 登录卡片 ================= */
#LoginCard {
    background: white;
    border-radius: 14px;
    padding: 24px;
}

/* 标题 */
QLabel#titleLabel {
    font-size: 24px;
    font-weight: bold;
    color: #ff7b87;
    margin-bottom: 20px;
}

/* ================= 输入框 ================= */
QLineEdit {
    background: #ffffff;
    border: 1px solid #f3c1c6;
    color: #333333;
    border-radius: 10px;
    padding: 8px;
    font-size: 14px;
}
QLineEdit::placeholder {
    color: #bbbbbb;
}

QLineEdit:focus {
    border: 2px solid #ff9aa2;
}

/* ================= 主按钮 ================= */
QPushButton#loginBtn {
    background-color: #ff9aa2;
    color: white;
    border: none;
    border-radius: 12px;
    padding: 10px;
    font-size: 16px;
    font-weight: bold;
}

QPushButton#loginBtn:hover {
    background-color: #ff7b87;
}

/* ================= 提示文字 ================= */
QLabel#hintLabel {
    color: #ff7b87;
    font-size: 12px;
    min-height: 20px;
}
)");
    const QIcon icon = loadAppIcon();
    if (!icon.isNull()) {
        LoginDialog::setWindowIcon(icon);
    }
}

void LoginDialog::toggleMode() {
    m_isRegisterMode = !m_isRegisterMode;

    if (m_isRegisterMode) {
        m_titleLabel->setText("用户注册");
        m_loginBtn->setText("注册并登录");
        m_pass2Edit->show();
        m_hintLabel->setText("密码至少4位");
        m_switchBtn->setText("已有账号？立即登录");
    } else {
        m_titleLabel->setText("用户登录");
        m_loginBtn->setText("登录");
        m_pass2Edit->hide();
        m_hintLabel->setText("默认管理员：admin / admin");
        m_switchBtn->setText("没有账号？立即注册");
    }

    m_userEdit->clear();
    m_passEdit->clear();
    m_pass2Edit->clear();
    m_userEdit->setFocus();
}

void LoginDialog::setHint(const QString& text, bool ok) {
    m_hintLabel->setText(text);
    m_hintLabel->setStyleSheet(ok ? "color: #1b5e20; font-style: italic; min-height: 20px;"
                                  : "color: #ff5722; font-style: italic; min-height: 20px;");
}

void LoginDialog::onLoginOrRegisterClicked() {
    if (m_isRegisterMode) {
        if (m_passEdit->text() != m_pass2Edit->text()) {
            setHint("两次密码不一致", false);
            return;
        }
        QString err;
        if (!m_ctx->auth().registerUser(m_userEdit->text(), m_passEdit->text(), false, &err)) {
            setHint("注册失败：" + err, false);
            return;
        }
        if (!m_ctx->auth().login(m_userEdit->text(), m_passEdit->text(), &err)) {
            setHint("注册成功，但登录失败：" + err, false);
            return;
        }
        setHint("注册并登录成功！", true);
        accept();
    } else {
        QString err;
        if (!m_ctx->auth().login(m_userEdit->text(), m_passEdit->text(), &err)) {
            setHint("登录失败：" + err, false);
            return;
        }
        setHint("登录成功！", true);
        accept();
    }
}
