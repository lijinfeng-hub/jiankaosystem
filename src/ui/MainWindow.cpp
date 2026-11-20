#include "MainWindow.h"

#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabWidget>

#include "app/AppContext.h"
#include "app/AppInfo.h"
#include "ui/widgets/TeachersWidget.h"
#include "ui/widgets/ExamsWidget.h"
#include "ui/widgets/ScheduleWidget.h"
#include "ui/widgets/QueryWidget.h"
#include "ui/widgets/LogsWidget.h"

MainWindow::MainWindow(AppContext* ctx, QWidget* parent)
    : QMainWindow(parent), m_ctx(ctx) {
    setWindowTitle(appinfo::title());
    resize(2200, 1440);
    m_tabs = new QTabWidget(this);
    m_teachers = new TeachersWidget(m_ctx, this);
    m_exams = new ExamsWidget(m_ctx, this);
    m_schedule = new ScheduleWidget(m_ctx, this);
    m_query = new QueryWidget(m_ctx, this);
    m_logs = new LogsWidget(m_ctx, this);
    m_tabs->addTab(m_teachers, "教师管理");
    m_tabs->addTab(m_exams, "考场设置");
    m_tabs->addTab(m_schedule, "智能排班");
    m_tabs->addTab(m_query, "按姓名查询");
    m_tabs->addTab(m_logs, "系统日志");
    QFont font = this->font();
    font.setFamily("Microsoft YaHei UI"); // Windows 常见中文字体
    font.setPointSize(10);
    m_tabs->setFont(font);
    setCentralWidget(m_tabs);
    m_userLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_userLabel);
    buildMenu();
    refreshUserLabel();
}

void MainWindow::buildMenu() {
    auto* fileMenu = menuBar()->addMenu("文件");
    auto* quit = new QAction("退出", this);
    connect(quit, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(quit);

    auto* userMenu = menuBar()->addMenu("用户");
    auto* logout = new QAction("退出登录", this);
    connect(logout, &QAction::triggered, this, &MainWindow::onLogout);
    userMenu->addAction(logout);
}

void MainWindow::refreshUserLabel() {
    if (m_ctx->auth().isLoggedIn()) {
        const auto u = m_ctx->auth().currentUser();
        m_userLabel->setText(QString("当前用户：%1%2")
                                 .arg(u.username)
                                 .arg(u.isAdmin ? "（管理员）" : ""));
    } else {
        m_userLabel->setText("未登录");
    }
}

void MainWindow::onLogout() {
    m_ctx->auth().logout();
    refreshUserLabel();
    close();
}
