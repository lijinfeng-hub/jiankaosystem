#pragma once

#include <QList>
#include <QMap>
#include <QString>

#include "model/Models.h"

struct SchedulerResult {
    bool success = false;
    QString reason;

    QList<model::Teacher> teachers;
    QList<model::ExamSession> exams;
    model::SchedulePlan plan;
};

class Scheduler {
public:
    SchedulerResult generate(const QList<model::Teacher>& teachers,
                            const QList<model::ExamSession>& exams,
                            const QString& operatorId,
                            const QString& note);

private:
    struct TState {
        model::Teacher t;
        QSet<QString> assignedSlots; // slotKey 集
        QMap<QString, bool> satisfiedCourse; // courseName -> 已满足“任课老师至少监考一个班”
        QMap<QString, int> dateCount; // date -> 已安排次数
    };

    static int compareExamTime(const model::ExamSession& a, const model::ExamSession& b);
    static int parseStartMinutes(const QString& timeSlot);

    static bool isBusy(const model::Teacher& t, const QString& slotKey);
    static bool hasTimeConflict(const TState& s, const QString& slotKey);

    static int scoreCandidate(const TState& s,
                              const model::ExamSession& e,
                              int avgTarget,
                              int maxAllowed,
                              const QMap<QString, int>& globalDateLoad);

    static bool tryBalance(QList<TState>& states,
                           QList<model::ExamSession>& exams,
                           int maxDiffAllowed,
                           QString* reason);
};
