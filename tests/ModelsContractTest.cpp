#include <QCoreApplication>
#include <QDebug>
#include <QTemporaryDir>

#include "app/AppInfo.h"
#include "model/Models.h"
#include "services/DataManager.h"

static bool expect(bool condition, const char* message) {
    if (!condition) {
        qCritical("FAILED: %s", message);
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    bool ok = true;
    ok &= expect(appinfo::name() == QStringLiteral("智能排监考管理系统"), "application name");
    ok &= expect(appinfo::version() == QStringLiteral("V1.0"), "application version");
    ok &= expect(appinfo::title() == QStringLiteral("智能排监考管理系统 V1.0"), "application title");
    ok &= expect(model::isValidTimeSlot("09:00-11:00"), "valid time slot");
    ok &= expect(!model::isValidTimeSlot("11:00-09:00"), "reverse time slot rejected");
    ok &= expect(!model::isValidTimeSlot("25:00-26:00"), "invalid clock rejected");
    ok &= expect(model::csvEscape("课程,期中") == "\"课程,期中\"", "comma escaped");
    ok &= expect(model::csvEscape("老师\"A") == "\"老师\"\"A\"", "quote escaped");
    ok &= expect(model::csvEscape("普通文本") == "普通文本", "plain text unchanged");

    QTemporaryDir temp;
    ok &= expect(temp.isValid(), "temporary data directory created");
    if (temp.isValid()) {
        DataManager data;
        data.setTeachersFile(temp.filePath("teachers.json"));
        data.setExamsFile(temp.filePath("exams.json"));
        data.setSchedulesFile(temp.filePath("schedules.json"));
        ok &= expect(data.load(), "empty data store loads");

        model::Teacher invalidTeacher;
        invalidTeacher.id = "T-invalid";
        invalidTeacher.name = "非法教师";
        invalidTeacher.busySlots.push_back({"2025-01-01|25:00-26:00", "测试"});
        QString err;
        ok &= expect(!data.addTeacher(invalidTeacher, "tester", &err), "invalid busy slot rejected");

        model::Teacher teacher;
        teacher.id = "T-001";
        teacher.name = "张老师";
        ok &= expect(data.addTeacher(teacher, "tester", &err), "valid teacher saved");

        model::ExamSession missingTeacherExam;
        missingTeacherExam.date = "2025-01-01";
        missingTeacherExam.timeSlot = "09:00-11:00";
        missingTeacherExam.roomName = "A101";
        missingTeacherExam.id = model::buildExamId(missingTeacherExam.date,
                                                    missingTeacherExam.timeSlot,
                                                    missingTeacherExam.roomName);
        missingTeacherExam.courseName = "数据结构";
        missingTeacherExam.mainTeacherId = "T-missing";
        missingTeacherExam.requiredInvigilators = 3;
        ok &= expect(!data.addExam(missingTeacherExam, "tester", &err), "missing main teacher rejected");

        model::ExamSession exam = missingTeacherExam;
        exam.mainTeacherId = teacher.id;
        ok &= expect(data.addExam(exam, "tester", &err), "valid exam saved");
        ok &= expect(!data.deleteTeacher(teacher.id, "tester", "仍被考试引用", &err),
                     "referenced teacher cannot be deleted");
    }

    return ok ? 0 : 1;
}
