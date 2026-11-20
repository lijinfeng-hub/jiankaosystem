#include <QCoreApplication>
#include <QDir>
#include <QTextStream>

#include "model/Models.h"
#include "services/DataManager.h"
#include "services/Scheduler.h"

static bool expect(bool condition, const char* message) {
    if (!condition) {
        qCritical("FAILED: %s", message);
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    DataManager data;
    const QDir root(QDir::currentPath());
    data.setTeachersFile(root.filePath("data/teachers.json"));
    data.setExamsFile(root.filePath("data/exams.json"));
    data.setSchedulesFile(root.filePath("data/schedules.json"));

    bool ok = true;
    ok &= expect(data.load(), "test data files load");
    ok &= expect(data.teachers().size() == 12, "twelve teachers loaded");
    ok &= expect(data.exams().size() == 8, "eight exams loaded");

    Scheduler scheduler;
    const auto result = scheduler.generate(data.teachers(), data.exams(), "tester", "fixture");
    if (!result.success) {
        QTextStream(stdout) << "scheduler reason: " << result.reason << Qt::endl;
    }
    ok &= expect(result.success, "fixture produces a balanced schedule");
    if (result.success) {
        for (const auto& exam : result.exams) {
            ok &= expect(exam.assignedTeacherIds.size() == exam.requiredInvigilators,
                         "every exam reaches its required invigilator count");
            if (exam.category == model::ExamCategory::NORMAL) {
                ok &= expect(exam.assignedTeacherIds.contains(exam.mainTeacherId),
                             "normal exam contains its main teacher");
            }
        }
    }

    return ok ? 0 : 1;
}
