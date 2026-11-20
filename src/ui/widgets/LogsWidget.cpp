#include "LogsWidget.h"

#include <QFile>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include "app/AppContext.h"

LogsWidget::LogsWidget(AppContext* ctx, QWidget* parent)
    : QWidget(parent), m_ctx(ctx) {
    m_refresh = new QPushButton("刷新", this);
    connect(m_refresh, &QPushButton::clicked, this, &LogsWidget::refresh);

    m_text = new QTextEdit(this);
    m_text->setReadOnly(true);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_refresh);
    layout->addWidget(m_text);
    setLayout(layout);

    refresh();
}

void LogsWidget::refresh() {
    const QString path = m_ctx->log().logFilePath();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_text->setText("无法读取日志文件：" + path);
        return;
    }
    m_text->setText(QString::fromUtf8(f.readAll()));
    f.close();
}
