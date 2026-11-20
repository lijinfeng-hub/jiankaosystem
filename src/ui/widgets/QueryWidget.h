#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QTableWidget;

class AppContext;

class QueryWidget : public QWidget {
    Q_OBJECT
public:
    explicit QueryWidget(AppContext* ctx, QWidget* parent = nullptr);

private slots:
    void onQuery();

private:
    AppContext* m_ctx;

    QLineEdit* m_name;
    QPushButton* m_query;
    QTableWidget* m_table;

    void showTeacher(const QString& name);
};
