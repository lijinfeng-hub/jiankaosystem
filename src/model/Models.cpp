#include "Models.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QTime>
#include <QUuid>

namespace model {

static QString trimmed(const QString& s) {
    return s.trimmed();
}

static QString normalizePunctuation(const QString& s) {
    // 兼容用户输入的全角/不同破折号
    QString out = s;
    out.replace(QChar(0xFF1A), QChar(':')); // ： -> :
    out.replace(QChar(0x2013), QChar('-')); // –
    out.replace(QChar(0x2014), QChar('-')); // —
    out.replace(QChar(0x2212), QChar('-')); // −
    out.replace(QChar(0xFF0D), QChar('-')); // －
    return out;
}

QJsonObject BusySlot::toJson() const {
    QJsonObject o;
    o["slotKey"] = slotKey;
    o["reason"] = reason;
    return o;
}

BusySlot BusySlot::fromJson(const QJsonObject& obj) {
    BusySlot b;
    b.slotKey = normalizePunctuation(obj.value("slotKey").toString()).trimmed();
    b.reason = normalizePunctuation(obj.value("reason").toString()).trimmed();
    if (!b.slotKey.contains('|') && b.slotKey.size() == 10 && b.slotKey[4] == '-' && b.slotKey[7] == '-') {
        if (b.reason.contains('|')) {
            const QStringList parts = b.reason.split('|');
            if (!parts.isEmpty()) {
                const QString ts = normalizeTimeSlot(parts[0]);
                QString rr;
                if (parts.size() >= 2) {
                    QStringList rest = parts;
                    rest.removeFirst();
                    rr = rest.join("|").trimmed();
                }
                if (!ts.isEmpty()) {
                    b.slotKey = QString("%1|%2").arg(b.slotKey, ts);
                    b.reason = rr;
                }
            }
        }
    }
    return b;
}

QJsonObject Teacher::toJson() const {
    QJsonObject o;
    o["id"] = id;
    o["name"] = name;
    o["title"] = titleToString(title);

    QJsonArray cs;
    for (const auto& c : courses) cs.append(c);
    o["courses"] = cs;

    QJsonArray bs;
    for (const auto& b : busySlots) bs.append(b.toJson());
    o["busySlots"] = bs;

    // 运行时字段不强制持久化，但这里仍保存，方便查看与调试
    o["assignedCount"] = assignedCount;
    QJsonArray ae;
    for (const auto& e : assignedExamIds) ae.append(e);
    o["assignedExamIds"] = ae;

    return o;
}

Teacher Teacher::fromJson(const QJsonObject& obj) {
    Teacher t;
    t.id = obj.value("id").toString();
    t.name = obj.value("name").toString();
    t.title = titleFromString(obj.value("title").toString());

    const auto cs = obj.value("courses").toArray();
    for (const auto& v : cs) t.courses.push_back(v.toString());

    const auto bs = obj.value("busySlots").toArray();
    for (const auto& v : bs) t.busySlots.push_back(BusySlot::fromJson(v.toObject()));

    t.assignedCount = obj.value("assignedCount").toInt(0);
    const auto ae = obj.value("assignedExamIds").toArray();
    for (const auto& v : ae) t.assignedExamIds.push_back(v.toString());

    return t;
}

QJsonObject ExamSession::toJson() const {
    QJsonObject o;
    o["id"] = id;
    o["date"] = date;
    o["timeSlot"] = timeSlot;
    o["roomName"] = roomName;
    o["roomType"] = roomTypeToString(type);
    o["category"] = categoryToString(category);
    o["courseName"] = courseName;
    o["mainTeacherId"] = mainTeacherId;
    o["requiredInvigilators"] = requiredInvigilators;

    QJsonArray as;
    for (const auto& tid : assignedTeacherIds) as.append(tid);
    o["assignedTeacherIds"] = as;

    return o;
}

ExamSession ExamSession::fromJson(const QJsonObject& obj) {
    ExamSession e;
    e.id = obj.value("id").toString();
    e.date = obj.value("date").toString();
    e.timeSlot = obj.value("timeSlot").toString();
    e.roomName = obj.value("roomName").toString();
    e.type = roomTypeFromString(obj.value("roomType").toString());
    e.category = categoryFromString(obj.value("category").toString());
    e.courseName = obj.value("courseName").toString();
    e.mainTeacherId = obj.value("mainTeacherId").toString();
    e.requiredInvigilators = obj.value("requiredInvigilators").toInt(0);

    const auto as = obj.value("assignedTeacherIds").toArray();
    for (const auto& v : as) e.assignedTeacherIds.push_back(v.toString());

    return e;
}

QJsonObject User::toJson() const {
    QJsonObject o;
    o["username"] = username;
    o["passwordHash"] = passwordHash;
    o["isAdmin"] = isAdmin;
    return o;
}

User User::fromJson(const QJsonObject& obj) {
    User u;
    u.username = obj.value("username").toString();
    u.passwordHash = obj.value("passwordHash").toString();
    u.isAdmin = obj.value("isAdmin").toBool(false);
    return u;
}

QString OperationLog::toLine() const {
    return QString("[%1] %2 %3 %4").arg(time, operatorId, action, details);
}

QJsonObject SchedulePlan::toJson() const {
    QJsonObject o;
    o["planId"] = planId;
    o["createdTime"] = createdTime;
    o["createdBy"] = createdBy;
    o["note"] = note;

    QJsonArray arr;
    for (auto it = assignments.begin(); it != assignments.end(); ++it) {
        QJsonObject item;
        item["examId"] = it.key();
        QJsonArray tids;
        for (const auto& tid : it.value()) tids.append(tid);
        item["teacherIds"] = tids;
        arr.append(item);
    }
    o["assignments"] = arr;
    return o;
}

SchedulePlan SchedulePlan::fromJson(const QJsonObject& obj) {
    SchedulePlan p;
    p.planId = obj.value("planId").toString();
    p.createdTime = obj.value("createdTime").toString();
    p.createdBy = obj.value("createdBy").toString();
    p.note = obj.value("note").toString();

    const auto arr = obj.value("assignments").toArray();
    for (const auto& v : arr) {
        const auto item = v.toObject();
        const QString examId = item.value("examId").toString();
        QStringList tids;
        for (const auto& tv : item.value("teacherIds").toArray()) tids.push_back(tv.toString());
        p.assignments.insert(examId, tids);
    }
    return p;
}

QString slotKey(const QString& date, const QString& timeSlot) {
    return QString("%1|%2").arg(trimmed(date), normalizeTimeSlot(timeSlot));
}

QString normalizeDate(const QDate& d) {
    return d.toString("yyyy-MM-dd");
}

QString normalizeTimeSlot(const QString& timeSlot) {
    // 允许用户输入有空格/全角冒号/不同破折号：统一成 "HH:MM-HH:MM" 形式（尽量）
    QString ts = normalizePunctuation(timeSlot);
    ts = ts.trimmed();

    // 去掉多余空格
    ts.replace(" ", "");
    ts.replace("\t", "");

    // 常见写法："09:00~11:00"、"09:00—11:00" 等，统一分隔符为 '-'
    ts.replace('~', '-');
    ts.replace(QChar(0xFF5E), '-'); // 全角波浪号

    return ts;
}

bool isValidDate(const QString& date) {
    const QString value = date.trimmed();
    if (!QRegularExpression(QStringLiteral("^\\d{4}-\\d{2}-\\d{2}$")).match(value).hasMatch()) {
        return false;
    }
    return QDate::fromString(value, QStringLiteral("yyyy-MM-dd")).isValid();
}

bool isValidTimeSlot(const QString& timeSlot) {
    const QString value = normalizeTimeSlot(timeSlot);
    const auto match = QRegularExpression(QStringLiteral("^(\\d{2}:\\d{2})-(\\d{2}:\\d{2})$")).match(value);
    if (!match.hasMatch()) return false;

    const QTime start = QTime::fromString(match.captured(1), QStringLiteral("HH:mm"));
    const QTime end = QTime::fromString(match.captured(2), QStringLiteral("HH:mm"));
    return start.isValid() && end.isValid() && start < end;
}

QString csvEscape(const QString& value) {
    if (!value.contains(',') && !value.contains('"') && !value.contains('\n') && !value.contains('\r')) {
        return value;
    }

    QString escaped = value;
    escaped.replace('"', QStringLiteral("\"\""));
    return QStringLiteral("\"") + escaped + QStringLiteral("\"");
}

static QString startTimeAsHHmm(const QString& timeSlot) {
    // timeSlot: "09:00-11:00" -> "0900"
    const QString ts = normalizeTimeSlot(timeSlot);
    const int dash = ts.indexOf('-');
    QString start = dash >= 0 ? ts.left(dash) : ts;
    start = start.trimmed();
    start.remove(':');
    if (start.size() == 3) start = "0" + start;
    return start;
}

QString buildExamId(const QString& date, const QString& timeSlot, const QString& roomName) {
    const QDate d = QDate::fromString(trimmed(date), "yyyy-MM-dd");
    const QString dpart = d.isValid() ? d.toString("yyyyMMdd") : trimmed(date).remove('-');
    const QString tpart = startTimeAsHHmm(timeSlot);
    return QString("%1-%2-%3").arg(dpart, tpart, trimmed(roomName));
}

QString titleToString(Title t) {
    switch (t) {
    case Title::PROFESSOR: return "PROFESSOR";
    case Title::ASSOCIATE_PROFESSOR: return "ASSOCIATE_PROFESSOR";
    case Title::LECTURER: return "LECTURER";
    case Title::ASSISTANT: return "ASSISTANT";
    }
    return "LECTURER";
}

Title titleFromString(const QString& s) {
    const QString v = s.trimmed().toUpper();
    if (v == "PROFESSOR") return Title::PROFESSOR;
    if (v == "ASSOCIATE_PROFESSOR") return Title::ASSOCIATE_PROFESSOR;
    if (v == "LECTURER") return Title::LECTURER;
    if (v == "ASSISTANT") return Title::ASSISTANT;
    // 兼容中文
    if (s.contains("教授")) return Title::PROFESSOR;
    if (s.contains("副教授")) return Title::ASSOCIATE_PROFESSOR;
    if (s.contains("讲师")) return Title::LECTURER;
    if (s.contains("助教")) return Title::ASSISTANT;
    return Title::LECTURER;
}

QString roomTypeToString(RoomType t) {
    return t == RoomType::BIG_ROOM ? "BIG_ROOM" : "SMALL_ROOM";
}

RoomType roomTypeFromString(const QString& s) {
    const QString v = s.trimmed().toUpper();
    if (v == "BIG_ROOM") return RoomType::BIG_ROOM;
    if (v == "SMALL_ROOM") return RoomType::SMALL_ROOM;
    // 兼容中文
    if (s.contains("大")) return RoomType::BIG_ROOM;
    return RoomType::SMALL_ROOM;
}

QString categoryToString(ExamCategory c) {
    return c == ExamCategory::PATROL ? "PATROL" : "NORMAL";
}

ExamCategory categoryFromString(const QString& s) {
    const QString v = s.trimmed().toUpper();
    if (v == "PATROL") return ExamCategory::PATROL;
    if (v == "NORMAL") return ExamCategory::NORMAL;
    if (s.contains("巡")) return ExamCategory::PATROL;
    return ExamCategory::NORMAL;
}

} // namespace model
