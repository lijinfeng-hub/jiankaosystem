#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QTableWidget;

class AppContext;

class ExamsWidget : public QWidget {
    Q_OBJECT
public:
    explicit ExamsWidget(AppContext* ctx, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void onSearchChanged(const QString& text);

private:
    AppContext* m_ctx;

    QLineEdit* m_search;
    QPushButton* m_add;
    QPushButton* m_edit;
    QPushButton* m_del;
    QPushButton* m_refresh;

    QTableWidget* m_table;

    void rebuildTable(const QString& filter);
    QString selectedExamId() const;
};
