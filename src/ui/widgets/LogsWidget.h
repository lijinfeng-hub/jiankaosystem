#pragma once

#include <QWidget>

class QTextEdit;
class QPushButton;

class AppContext;

class LogsWidget : public QWidget {
    Q_OBJECT
public:
    explicit LogsWidget(AppContext* ctx, QWidget* parent = nullptr);

public slots:
    void refresh();

private:
    AppContext* m_ctx;
    QTextEdit* m_text;
    QPushButton* m_refresh;
};
