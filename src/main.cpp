#include <QApplication>
#include <QIcon>
#include <QFont>
#include <QDebug>
#include<QFileInfo>
#include<QDir>
#include "app/AppContext.h"
#include "app/AppInfo.h"
#include "ui/LoginDialog.h"
#include "ui/MainWindow.h"
#include<QAbstractItemView>
static QIcon loadAppIcon() {
    // Load from: <executable_dir>/assets/app_icon.png
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QString iconPath = QDir(exeDir).filePath("assets/app_icon_2.png");
    if (QFileInfo::exists(iconPath)) {
        const QIcon icon(iconPath);
        if (!icon.isNull()) return icon;
    }
    return {};
}
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    const QIcon icon = loadAppIcon();
    if (!icon.isNull()) {
        QApplication::setWindowIcon(icon);
    }
    QApplication::setApplicationName(appinfo::name());
    QApplication::setApplicationVersion(appinfo::version());
    QFont font = QApplication::font();
    font.setFamily("Microsoft YaHei UI");
    font.setPointSize(10);
    QApplication::setFont(font);
    AppContext ctx;
    if (!ctx.initialize()) {
        return 1;
    }
    // 创建并显示登录对话框
    LoginDialog login(&ctx);
    if (login.exec() != QDialog::Accepted) {
        return 0;
    }

    // 创建主窗口
    MainWindow w(&ctx);
    w.setWindowIcon(icon);
    w.show();
    QString style = R"(
QWidget {
    background-color: #fffafc;       /* 奶白粉背景 */
    color: #444444;
    font-family: "Microsoft YaHei UI", "Segoe UI", sans-serif;
    font-size: 10pt;
}
QPushButton {
    background-color: #ff9aa2;       /* 马卡龙粉 */
    color: white;
    border: none;
    padding: 6px 14px;
    border-radius: 10px;              /* 圆润 */
    min-width: 90px;
}

QPushButton:hover {
    background-color: #ff7b87;
}

QPushButton:pressed {
    background-color: #e56770;
}

QPushButton:disabled {
    background-color: #ffd6da;
    color: #999999;
}

/* 次要按钮（如果你有用 objectName） */
QPushButton#secondary {
    background-color: #8ecae6;       /* 浅蓝 */
}
QPushButton#secondary:hover {
    background-color: #6fb6d8;
}

/* ================= 输入框 ================= */
QLineEdit, QComboBox, QSpinBox {
    background-color: #ffffff;
    border: 1px solid #f3c1c6;
    border-radius: 8px;
    padding: 6px;
    selection-background-color: #ffb7c5;
}

QLineEdit:focus, QComboBox:focus, QSpinBox:focus {
    border: 2px solid #ff9aa2;
}

/* ================= 下拉框 ================= */
QComboBox::drop-down {
    border: none;
}
QComboBox::down-arrow {
    image: none;
    border: 5px solid transparent;
    border-top-color: #ff9aa2;
    margin-right: 6px;
}

/* ================= 对话框 ================= */
QDialog {
    background-color: #ffffff;
    border-radius: 12px;
}

/* ================= 表格 ================= */
QTableView {
    gridline-color: #f3d5d8;
    border: 1px solid #f3c1c6;
    border-radius: 10px;
    selection-background-color: #ffb7c5;
    selection-color: #ffffff;
}
/* ===== 表格“后面空白区域”的背景图 ===== */
QTableView::viewport {
    background-repeat: no-repeat;
    background-position: center;
    background-color: #fffafc;
}
QTableView::item {
    background-color: rgba(255, 255, 255, 200);
}
QHeaderView::section {
    background-color: #ff9aa2;
    color: white;
    padding: 6px;
    border: 1px solid #e56770;
    border-bottom: 1px solid #f3c1c6;
    font-weight: bold;
}
QTableView::item:selected {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ffb7c5, stop:1 #ff9aa2);
    color: white;
}
QTableView::item:hover {
    background-color: #fff0f4; /* 淡淡的粉色提示当前鼠标位置 */
}

QTableView::item:selected:hover {
    background-color: #ff7b87; /* 选中且悬浮时颜色最深 */
}
QTableView::item:selected:!active {
    background-color: #ffd6da;
    color: #444444;
}
/* ================= 滚动条（超可爱） ================= */
QScrollBar:vertical {
    background: #fff0f4;
    width: 10px;
    margin: 0;
    border-radius: 5px;
}

QScrollBar::handle:vertical {
    background: #ff9aa2;
    border-radius: 5px;
    min-height: 20px;
}

QScrollBar::handle:vertical:hover {
    background: #ff7b87;
}

QScrollBar::add-line, QScrollBar::sub-line {
    height: 0;
}

/* ================= 提示框 ================= */
QToolTip {
    background-color: #ff9aa2;
    color: white;
    border-radius: 6px;
    padding: 4px 8px;
}
/* ================= Tab 页整体 ================= */
QTabWidget::pane {
    border: none;
    background: #ffffff;
    border-radius: 12px;
    margin-top: 6px;
}

/* 未选中 Tab（淡绿色） */
QTabBar::tab {
    background: #e6f7f1;
    color: #2f6f5f;
    padding: 8px 20px;
    margin-right: 6px;
    border-radius: 10px;
    min-width: 110px;
    font-weight: bold;
}

/* 悬浮 */
QTabBar::tab:hover {
    background: #c9efe3;
}

/* 选中 Tab */
QTabBar::tab:selected {
    background: #7fd1b9;
    color: white;
}
/* ================= 菜单栏（文件 / 用户） ================= */

/* 整个菜单栏背景 */
QMenuBar {
    background-color: #f3fbf8;
    padding: 6px;
}

/* 菜单项（文件 / 用户） */
QMenuBar::item {
    background: transparent;
    padding: 6px 14px;
    margin: 0 4px;
    border-radius: 8px;
    color: #2f6f5f;
    font-weight: bold;
}

/* 悬浮 */
QMenuBar::item:selected {
    background-color: #c9efe3;
}

/* 按下 */
QMenuBar::item:pressed {
    background-color: #7fd1b9;
    color: white;
}

/* ================= 下拉菜单 ================= */
QMenu {
    background-color: #ffffff;
    border: 1px solid #c9efe3;
    border-radius: 10px;
    padding: 6px;
}

/* 下拉菜单里的每一项 */
QMenu::item {
    padding: 8px 20px;
    border-radius: 8px;
    color: #2f6f5f;
}

/* 悬浮项 */
QMenu::item:selected {
    background-color: #e6f7f1;
}

/* 分隔线 */
QMenu::separator {
    height: 1px;
    background: #c9efe3;
    margin: 6px 10px;
}
)";
    app.setStyleSheet(style);
    return app.exec();
}
