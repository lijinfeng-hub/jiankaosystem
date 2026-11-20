#pragma once

#include <QMainWindow>

class QTabWidget;
class QLabel;

class AppContext;

class TeachersWidget;
class ExamsWidget;
class ScheduleWidget;
class QueryWidget;
class LogsWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(AppContext* ctx, QWidget* parent = nullptr);

private slots:
    void onLogout();

private:
    AppContext* m_ctx;

    QTabWidget* m_tabs;
    QLabel* m_userLabel;

    TeachersWidget* m_teachers;
    ExamsWidget* m_exams;
    ScheduleWidget* m_schedule;
    QueryWidget* m_query;
    LogsWidget* m_logs;

    void buildMenu();
    void refreshUserLabel();
};
