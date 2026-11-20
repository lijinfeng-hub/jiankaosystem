#pragma once

#include <QDate>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

namespace model {

enum class Title {
    PROFESSOR,
    ASSOCIATE_PROFESSOR,
    LECTURER,
    ASSISTANT
};

enum class RoomType {
    BIG_ROOM,
    SMALL_ROOM
};

enum class ExamCategory {
    NORMAL,
    PATROL // 巡考：按监考计数
};

struct BusySlot {
    // 统一用 slotKey 表示："YYYY-MM-DD|HH:MM-HH:MM"
    QString slotKey;
    QString reason; // 出差/生病/其他

    QJsonObject toJson() const;
    static BusySlot fromJson(const QJsonObject& obj);
};

struct Teacher {
    QString id;                // 工号（唯一）
    QString name;
    Title title = Title::LECTURER;
    QStringList courses;       // 可选：教授/任课老师课程列表（用于展示与辅助）
    QList<BusySlot> busySlots; // 特殊要求：不能监考的时间段

    // 排班结果统计（每次生成方案时刷新）
    int assignedCount = 0;
    QStringList assignedExamIds;

    bool isProfessor() const { return title == Title::PROFESSOR; }

    QJsonObject toJson() const;
    static Teacher fromJson(const QJsonObject& obj);
};

struct ExamSession {
    QString id;          // 统一编号：如 "20240115-0900-A101"
    QString date;        // "YYYY-MM-DD"
    QString timeSlot;    // "HH:MM-HH:MM"
    QString roomName;    // "A101" 或 "PATROL"
    RoomType type = RoomType::SMALL_ROOM;
    ExamCategory category = ExamCategory::NORMAL;

    QString courseName;
    QString mainTeacherId; // 任课老师（必须至少监考一个同课程场次）

    int requiredInvigilators = 3; // 小教室3，大教室6；巡考通常1

    QStringList assignedTeacherIds; // 排班结果

    QJsonObject toJson() const;
    static ExamSession fromJson(const QJsonObject& obj);
};

struct User {
    QString username;
    QString passwordHash;
    bool isAdmin = false;

    QJsonObject toJson() const;
    static User fromJson(const QJsonObject& obj);
};

struct OperationLog {
    QString time;
    QString operatorId;
    QString action;
    QString details;

    QString toLine() const;
};

struct SchedulePlan {
    QString planId;      // UUID/时间戳
    QString createdTime; // ISO
    QString createdBy;
    QString note;

    // examId -> teacherIds
    QMap<QString, QStringList> assignments;

    QJsonObject toJson() const;
    static SchedulePlan fromJson(const QJsonObject& obj);
};

QString slotKey(const QString& date, const QString& timeSlot);
QString normalizeDate(const QDate& d);
QString normalizeTimeSlot(const QString& timeSlot);
bool isValidDate(const QString& date);
bool isValidTimeSlot(const QString& timeSlot);
QString csvEscape(const QString& value);
QString buildExamId(const QString& date, const QString& timeSlot, const QString& roomName);

QString titleToString(Title t);
Title titleFromString(const QString& s);
QString roomTypeToString(RoomType t);
RoomType roomTypeFromString(const QString& s);
QString categoryToString(ExamCategory c);
ExamCategory categoryFromString(const QString& s);

} // namespace model
