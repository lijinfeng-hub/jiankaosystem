#pragma once

#include <QList>
#include <QString>

#include "model/Models.h"

class LogManager;

class DataManager {
public:
    void setTeachersFile(const QString& path);
    void setExamsFile(const QString& path);
    void setSchedulesFile(const QString& path);
    void setLogManager(LogManager* log);

    bool load();
    bool saveTeachers() const;
    bool saveExams() const;
    bool savePlans() const;

    QList<model::Teacher>& teachers();
    QList<model::ExamSession>& exams();
    QList<model::SchedulePlan>& plans();

    const QList<model::Teacher>& teachers() const;
    const QList<model::ExamSession>& exams() const;
    const QList<model::SchedulePlan>& plans() const;

    model::Teacher* findTeacherById(const QString& id);
    model::Teacher* findTeacherByName(const QString& name);
    model::ExamSession* findExamById(const QString& id);

    // CRUD：都会自动记录日志并保存到文件
    bool addTeacher(const model::Teacher& t, const QString& operatorId, QString* err);
    bool updateTeacher(const model::Teacher& t, const QString& operatorId, QString* err);
    bool deleteTeacher(const QString& teacherId, const QString& operatorId, const QString& reason, QString* err);

    bool addExam(const model::ExamSession& e, const QString& operatorId, QString* err);
    bool updateExam(const model::ExamSession& e, const QString& operatorId, QString* err);
    bool deleteExam(const QString& examId, const QString& operatorId, const QString& reason, QString* err);

    bool addPlan(const model::SchedulePlan& p, const QString& operatorId, QString* err);
    bool updatePlanNote(const QString& planId, const QString& newNote, const QString& operatorId, QString* err);
    bool deletePlan(const QString& planId, const QString& operatorId, const QString& reason, QString* err);
    model::SchedulePlan* findPlanById(const QString& planId);

private:
    QString m_teachersFile;
    QString m_examsFile;
    QString m_schedulesFile;

    QList<model::Teacher> m_teachers;
    QList<model::ExamSession> m_exams;
    QList<model::SchedulePlan> m_plans;

    LogManager* m_log = nullptr;

    static bool validateTeacher(const model::Teacher& t, QString* err);
    static bool validateExam(const model::ExamSession& e, QString* err);

    void logOp(const QString& operatorId, const QString& action, const QString& details) const;
};
