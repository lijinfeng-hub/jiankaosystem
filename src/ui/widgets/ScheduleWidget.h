#pragma once

#include <QWidget>

class QComboBox;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTextEdit;

class AppContext;

class ScheduleWidget : public QWidget {
    Q_OBJECT
public:
    explicit ScheduleWidget(AppContext* ctx, QWidget* parent = nullptr);

public slots:
    void refreshPlans();

private slots:
    void onGenerate();
    void onApplySelectedPlan();
    void onUpdateNote();
    void onDeletePlan();
    void onExportCsv();

private:
    AppContext* m_ctx;

    QComboBox* m_planSelect;
    QLineEdit* m_note;

    QPushButton* m_generate;
    QPushButton* m_apply;
    QPushButton* m_updateNote;
    QPushButton* m_delete;
    QPushButton* m_export;

    QTextEdit* m_resultHint;
    QTableWidget* m_table;

    void showPlanInTable(const QString& planId);
    bool applyPlanToRuntime(const QString& planId, QString* err);
};
