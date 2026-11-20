#include "DataManager.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>
#include <QSet>

#include "services/LogManager.h"

static bool writeAll(const QString& path, const QByteArray& data) {
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    if (f.write(data) != data.size()) return false;
    return f.commit();
}

static bool readJsonArray(const QString& path, const QString& key, QJsonArray* result) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning().noquote() << "无法读取数据文件：" << path;
        return false;
    }

    const QByteArray bytes = f.readAll();
    if (bytes.trimmed().isEmpty()) {
        qWarning().noquote() << "数据文件为空：" << path;
        return false;
    }

    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning().noquote() << "数据文件解析失败：" << path << parseError.errorString();
        return false;
    }

    const auto value = doc.object().value(key);
    if (!value.isArray()) {
        qWarning().noquote() << "数据文件缺少数组节点：" << path << key;
        return false;
    }

    const auto array = value.toArray();
    for (const auto& item : array) {
        if (!item.isObject()) {
            qWarning().noquote() << "数据文件包含非法条目：" << path;
            return false;
        }
    }
    *result = array;
    return true;
}

void DataManager::setTeachersFile(const QString& path) { m_teachersFile = path; }
void DataManager::setExamsFile(const QString& path) { m_examsFile = path; }
void DataManager::setSchedulesFile(const QString& path) { m_schedulesFile = path; }
void DataManager::setLogManager(LogManager* log) { m_log = log; }

QList<model::Teacher>& DataManager::teachers() { return m_teachers; }
QList<model::ExamSession>& DataManager::exams() { return m_exams; }
QList<model::SchedulePlan>& DataManager::plans() { return m_plans; }

const QList<model::Teacher>& DataManager::teachers() const { return m_teachers; }
const QList<model::ExamSession>& DataManager::exams() const { return m_exams; }
const QList<model::SchedulePlan>& DataManager::plans() const { return m_plans; }

void DataManager::logOp(const QString& operatorId, const QString& action, const QString& details) const {
    if (m_log) m_log->writeOp(operatorId, action, details);
}

bool DataManager::load() {
    m_teachers.clear();
    m_exams.clear();
    m_plans.clear();

    if (!m_teachersFile.isEmpty() && QFile::exists(m_teachersFile)) {
        QJsonArray arr;
        if (!readJsonArray(m_teachersFile, "teachers", &arr)) {
            return false;
        }
        for (const auto& v : arr) m_teachers.push_back(model::Teacher::fromJson(v.toObject()));
    }

    if (!m_examsFile.isEmpty() && QFile::exists(m_examsFile)) {
        QJsonArray arr;
        if (!readJsonArray(m_examsFile, "exams", &arr)) {
            m_teachers.clear();
            return false;
        }
        for (const auto& v : arr) m_exams.push_back(model::ExamSession::fromJson(v.toObject()));
    }

    if (!m_schedulesFile.isEmpty() && QFile::exists(m_schedulesFile)) {
        QJsonArray arr;
        if (!readJsonArray(m_schedulesFile, "plans", &arr)) {
            m_teachers.clear();
            m_exams.clear();
            return false;
        }
        for (const auto& v : arr) m_plans.push_back(model::SchedulePlan::fromJson(v.toObject()));
    }

    return true;
}

bool DataManager::saveTeachers() const {
    if (m_teachersFile.isEmpty()) return false;
    QJsonArray arr;
    for (const auto& t : m_teachers) arr.append(t.toJson());
    QJsonObject root;
    root["teachers"] = arr;
    return writeAll(m_teachersFile, QJsonDocument(root).toJson(QJsonDocument::Indented));
}

bool DataManager::saveExams() const {
    if (m_examsFile.isEmpty()) return false;
    QJsonArray arr;
    for (const auto& e : m_exams) arr.append(e.toJson());
    QJsonObject root;
    root["exams"] = arr;
    return writeAll(m_examsFile, QJsonDocument(root).toJson(QJsonDocument::Indented));
}

bool DataManager::savePlans() const {
    if (m_schedulesFile.isEmpty()) return false;
    QJsonArray arr;
    for (const auto& p : m_plans) arr.append(p.toJson());
    QJsonObject root;
    root["plans"] = arr;
    return writeAll(m_schedulesFile, QJsonDocument(root).toJson(QJsonDocument::Indented));
}

model::Teacher* DataManager::findTeacherById(const QString& id) {
    for (auto& t : m_teachers) if (t.id == id) return &t;
    return nullptr;
}

model::Teacher* DataManager::findTeacherByName(const QString& name) {
    const QString n = name.trimmed();
    for (auto& t : m_teachers) if (t.name == n) return &t;
    return nullptr;
}

model::ExamSession* DataManager::findExamById(const QString& id) {
    for (auto& e : m_exams) if (e.id == id) return &e;
    return nullptr;
}

model::SchedulePlan* DataManager::findPlanById(const QString& planId) {
    for (auto& p : m_plans) if (p.planId == planId) return &p;
    return nullptr;
}

bool DataManager::validateTeacher(const model::Teacher& t, QString* err) {
    if (t.id.trimmed().isEmpty()) {
        if (err) *err = "工号不能为空";
        return false;
    }
    if (t.name.trimmed().isEmpty()) {
        if (err) *err = "姓名不能为空";
        return false;
    }
    QSet<QString> busySlotKeys;
    for (const auto& b : t.busySlots) {
        const QString key = b.slotKey.trimmed();
        const int separator = key.indexOf('|');
        if (separator <= 0 || separator >= key.size() - 1
            || !model::isValidDate(key.left(separator))
            || !model::isValidTimeSlot(key.mid(separator + 1))) {
            if (err) *err = "忙碌时段必须使用 YYYY-MM-DD|HH:MM-HH:MM 格式";
            return false;
        }
        if (busySlotKeys.contains(key)) {
            if (err) *err = "忙碌时段不能重复";
            return false;
        }
        busySlotKeys.insert(key);
    }
    return true;
}

bool DataManager::validateExam(const model::ExamSession& e, QString* err) {
    if (e.id.trimmed().isEmpty()) {
        if (err) *err = "考场编号不能为空";
        return false;
    }
    if (!model::isValidDate(e.date)) {
        if (err) *err = "日期必须使用 YYYY-MM-DD 格式";
        return false;
    }
    if (!model::isValidTimeSlot(e.timeSlot)) {
        if (err) *err = "时间段必须使用 HH:MM-HH:MM 格式且开始时间早于结束时间";
        return false;
    }
    if (e.roomName.trimmed().isEmpty()) {
        if (err) *err = "教室不能为空";
        return false;
    }
    if (e.category == model::ExamCategory::NORMAL) {
        if (e.courseName.trimmed().isEmpty()) {
            if (err) *err = "考试科目不能为空";
            return false;
        }
        if (e.mainTeacherId.trimmed().isEmpty()) {
            if (err) *err = "任课老师不能为空";
            return false;
        }
    }
    if (e.requiredInvigilators <= 0) {
        if (err) *err = "需求监考人数必须>0";
        return false;
    }
    return true;
}

bool DataManager::addTeacher(const model::Teacher& t, const QString& operatorId, QString* err) {
    if (!validateTeacher(t, err)) return false;
    if (findTeacherById(t.id)) {
        if (err) *err = "工号已存在";
        return false;
    }
    m_teachers.push_back(t);
    if (!saveTeachers()) {
        if (err) *err = "保存 teachers.json 失败";
        return false;
    }
    logOp(operatorId, "ADD_TEACHER", QString("id=%1 name=%2").arg(t.id, t.name));
    return true;
}

bool DataManager::updateTeacher(const model::Teacher& t, const QString& operatorId, QString* err) {
    if (!validateTeacher(t, err)) return false;
    auto* ptr = findTeacherById(t.id);
    if (!ptr) {
        if (err) *err = "未找到该工号";
        return false;
    }
    *ptr = t;
    if (!saveTeachers()) {
        if (err) *err = "保存 teachers.json 失败";
        return false;
    }
    logOp(operatorId, "UPDATE_TEACHER", QString("id=%1 name=%2").arg(t.id, t.name));
    return true;
}

bool DataManager::deleteTeacher(const QString& teacherId, const QString& operatorId, const QString& reason, QString* err) {
    for (const auto& e : m_exams) {
        if (e.mainTeacherId == teacherId) {
            if (err) *err = "该教师仍被考试场次引用，请先修改相关考试";
            return false;
        }
    }
    for (const auto& p : m_plans) {
        for (auto it = p.assignments.cbegin(); it != p.assignments.cend(); ++it) {
            if (it.value().contains(teacherId)) {
                if (err) *err = "该教师仍出现在排班方案中，请先处理相关方案";
                return false;
            }
        }
    }

    const int before = m_teachers.size();
    for (int i = 0; i < m_teachers.size(); ++i) {
        if (m_teachers[i].id == teacherId) {
            m_teachers.removeAt(i);
            break;
        }
    }
    if (m_teachers.size() == before) {
        if (err) *err = "未找到要删除的老师";
        return false;
    }
    if (!saveTeachers()) {
        if (err) *err = "保存 teachers.json 失败";
        return false;
    }
    logOp(operatorId, "DELETE_TEACHER", QString("id=%1 reason=%2").arg(teacherId, reason));
    return true;
}

bool DataManager::addExam(const model::ExamSession& e, const QString& operatorId, QString* err) {
    if (!validateExam(e, err)) return false;
    if (e.category == model::ExamCategory::NORMAL && !findTeacherById(e.mainTeacherId)) {
        if (err) *err = "任课老师不存在，请先添加教师信息";
        return false;
    }
    if (findExamById(e.id)) {
        if (err) *err = "考场编号已存在";
        return false;
    }
    m_exams.push_back(e);
    if (!saveExams()) {
        if (err) *err = "保存 exams.json 失败";
        return false;
    }
    logOp(operatorId, "ADD_EXAM", QString("id=%1 course=%2 room=%3").arg(e.id, e.courseName, e.roomName));
    return true;
}

bool DataManager::updateExam(const model::ExamSession& e, const QString& operatorId, QString* err) {
    if (!validateExam(e, err)) return false;
    if (e.category == model::ExamCategory::NORMAL && !findTeacherById(e.mainTeacherId)) {
        if (err) *err = "任课老师不存在，请先添加教师信息";
        return false;
    }
    auto* ptr = findExamById(e.id);
    if (!ptr) {
        if (err) *err = "未找到该考场";
        return false;
    }
    *ptr = e;
    if (!saveExams()) {
        if (err) *err = "保存 exams.json 失败";
        return false;
    }
    logOp(operatorId, "UPDATE_EXAM", QString("id=%1 course=%2 room=%3").arg(e.id, e.courseName, e.roomName));
    return true;
}

bool DataManager::deleteExam(const QString& examId, const QString& operatorId, const QString& reason, QString* err) {
    const int before = m_exams.size();
    for (int i = 0; i < m_exams.size(); ++i) {
        if (m_exams[i].id == examId) {
            m_exams.removeAt(i);
            break;
        }
    }
    if (m_exams.size() == before) {
        if (err) *err = "未找到要删除的考场";
        return false;
    }
    if (!saveExams()) {
        if (err) *err = "保存 exams.json 失败";
        return false;
    }
    logOp(operatorId, "DELETE_EXAM", QString("id=%1 reason=%2").arg(examId, reason));
    return true;
}

bool DataManager::addPlan(const model::SchedulePlan& p, const QString& operatorId, QString* err) {
    if (p.planId.trimmed().isEmpty()) {
        if (err) *err = "方案ID不能为空";
        return false;
    }
    m_plans.push_back(p);
    if (!savePlans()) {
        if (err) *err = "保存 schedules.json 失败";
        return false;
    }
    logOp(operatorId, "ADD_PLAN", QString("planId=%1 note=%2").arg(p.planId, p.note));
    return true;
}

bool DataManager::updatePlanNote(const QString& planId, const QString& newNote, const QString& operatorId, QString* err) {
    auto* p = findPlanById(planId);
    if (!p) {
        if (err) *err = "未找到要修改的方案";
        return false;
    }
    p->note = newNote;
    if (!savePlans()) {
        if (err) *err = "保存 schedules.json 失败";
        return false;
    }
    logOp(operatorId, "UPDATE_PLAN", QString("planId=%1 note=%2").arg(planId, newNote));
    return true;
}

bool DataManager::deletePlan(const QString& planId, const QString& operatorId, const QString& reason, QString* err) {
    const int before = m_plans.size();
    for (int i = 0; i < m_plans.size(); ++i) {
        if (m_plans[i].planId == planId) {
            m_plans.removeAt(i);
            break;
        }
    }
    if (m_plans.size() == before) {
        if (err) *err = "未找到要删除的方案";
        return false;
    }
    if (!savePlans()) {
        if (err) *err = "保存 schedules.json 失败";
        return false;
    }
    logOp(operatorId, "DELETE_PLAN", QString("planId=%1 reason=%2").arg(planId, reason));
    return true;
}
