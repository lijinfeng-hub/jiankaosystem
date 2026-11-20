#include "Scheduler.h"

#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QSet>
#include <QTime>
#include <QUuid>

using namespace model;

static QString mkSlotKey(const ExamSession& e) {
    return model::slotKey(e.date, e.timeSlot);
}

int Scheduler::parseStartMinutes(const QString& timeSlot) {
    // "09:00-11:00" -> 540
    const QString ts = model::normalizeTimeSlot(timeSlot);
    const int dash = ts.indexOf('-');
    const QString start = (dash >= 0 ? ts.left(dash) : ts).trimmed();
    const QTime t = QTime::fromString(start, "HH:mm");
    if (!t.isValid()) return 0;
    return t.hour() * 60 + t.minute();
}

int Scheduler::compareExamTime(const ExamSession& a, const ExamSession& b) {
    if (a.date != b.date) return a.date < b.date ? -1 : 1;
    const int am = parseStartMinutes(a.timeSlot);
    const int bm = parseStartMinutes(b.timeSlot);
    if (am != bm) return am < bm ? -1 : 1;
    return a.id < b.id ? -1 : (a.id > b.id ? 1 : 0);
}

bool Scheduler::isBusy(const Teacher& t, const QString& slotKey) {
    for (const auto& b : t.busySlots) {
        if (b.slotKey == slotKey) return true;
    }
    return false;
}

bool Scheduler::hasTimeConflict(const TState& s, const QString& slotKey) {
    return s.assignedSlots.contains(slotKey);
}

int Scheduler::scoreCandidate(const TState& s,
                             const ExamSession& e,
                             int avgTarget,
                             int maxAllowed,
                             const QMap<QString, int>& /*globalDateLoad*/) {
    // 不允许超过 maxAllowed（硬限制）
    if (s.t.assignedCount >= maxAllowed) return -1;

    int score = 0;

    // 负荷越少越高
    score += 1000 - s.t.assignedCount * 60;

    // 同一天连排倾向：当天已有安排则加分
    const int todayCount = s.dateCount.value(e.date, 0);
    if (todayCount > 0) score += 120;

    // 更倾向把“需要多次监考的人”压缩到少数日期：
    // 如果该老师已在其他日期有安排，但今天没有，给少量分（让其尽量同日）
    int otherDays = 0;
    for (auto it = s.dateCount.begin(); it != s.dateCount.end(); ++it) {
        if (it.key() != e.date && it.value() > 0) otherDays += 1;
    }
    if (todayCount == 0 && otherDays > 0) score += 25;

    // 平均目标附近微调：低于 avgTarget 优先
    if (s.t.assignedCount < avgTarget) score += 80;

    return score;
}

SchedulerResult Scheduler::generate(const QList<Teacher>& teachers,
                                   const QList<ExamSession>& examsIn,
                                   const QString& operatorId,
                                   const QString& note) {
    SchedulerResult r;

    if (teachers.isEmpty()) {
        r.success = false;
        r.reason = "没有教师数据，无法排班";
        return r;
    }
    if (examsIn.isEmpty()) {
        r.success = false;
        r.reason = "没有考场/场次数据，无法排班";
        return r;
    }

    // 初始化状态
    QList<TState> states;
    states.reserve(teachers.size());
    for (const auto& t : teachers) {
        TState s;
        s.t = t;
        s.t.assignedCount = 0;
        s.t.assignedExamIds.clear();
        states.push_back(s);
    }

    // 复制并排序 exams
    QList<ExamSession> exams = examsIn;
    std::sort(exams.begin(), exams.end(), [](const ExamSession& a, const ExamSession& b) {
        return Scheduler::compareExamTime(a, b) < 0;
    });

    auto findStateByTeacherId = [&](const QString& tid) -> TState* {
        for (auto& s : states) if (s.t.id == tid) return &s;
        return nullptr;
    };

    // 统计非教授人数
    int nonProfCount = 0;
    for (const auto& s : states) if (!s.t.isProfessor()) nonProfCount++;
    if (nonProfCount <= 0) {
        r.success = false;
        r.reason = "没有非教授教师，无法完成大/小教室的补充监考";
        return r;
    }

    // 预估：教授只做“自己课程的一次任课监考”，不参与补充
    // 先计算总需求与固定教授位
    int totalRequired = 0;
    int fixedProfessor = 0;
    QSet<QString> professorMandatoryDone; // teacherId|courseName

    for (const auto& e : exams) {
        totalRequired += e.requiredInvigilators;

        if (e.category == ExamCategory::NORMAL && !e.mainTeacherId.trimmed().isEmpty()) {
            auto* ms = findStateByTeacherId(e.mainTeacherId);
            if (ms && ms->t.isProfessor()) {
                const QString key = e.mainTeacherId + "|" + e.courseName;
                // 教授也遵循“多班只需一次”
                if (!professorMandatoryDone.contains(key)) {
                    fixedProfessor += 1;
                    professorMandatoryDone.insert(key);
                }
            }
        }
    }

    const int totalForNonProf = qMax(0, totalRequired - fixedProfessor);
    const int avgTarget = totalForNonProf / nonProfCount;
    const int maxAllowed = avgTarget + 1;

    // 全局日期负荷（可用于更多评分，暂保留）
    QMap<QString, int> globalDateLoad;
    for (const auto& e : exams) globalDateLoad[e.date] += e.requiredInvigilators;

    // 第一轮：硬约束填任课老师（每个课程只需一次）
    QSet<QString> courseMandatoryDone; // teacherId|courseName

    for (auto& e : exams) {
        e.assignedTeacherIds.clear();

        if (e.category != ExamCategory::NORMAL) {
            continue;
        }

        TState* main = findStateByTeacherId(e.mainTeacherId);
        if (!main) {
            r.success = false;
            r.reason = QString("考场 %1 的任课老师不存在：%2").arg(e.id, e.mainTeacherId);
            return r;
        }

        const QString sKey = mkSlotKey(e);
        const QString key = e.mainTeacherId + "|" + e.courseName;

        if (!courseMandatoryDone.contains(key)) {
            // 必须安排一次
            if (isBusy(main->t, sKey)) {
                r.success = false;
                r.reason = QString("任课老师 %1 在 %2 不能监考（%3），请调整特殊要求或考场时间")
                               .arg(main->t.name, sKey,
                                    [&]() -> QString {
                                        for (const auto& b : main->t.busySlots) if (b.slotKey == sKey) return b.reason;
                                        return "";
                                    }());
                return r;
            }
            if (hasTimeConflict(*main, sKey)) {
                r.success = false;
                r.reason = QString("任课老师 %1 在 %2 时间冲突，无法同时监考多场")
                               .arg(main->t.name, sKey);
                return r;
            }

            // 安排
            e.assignedTeacherIds.push_back(main->t.id);
            main->assignedSlots.insert(sKey);
            main->t.assignedCount += 1;
            main->t.assignedExamIds.push_back(e.id);
            main->dateCount[e.date] += 1;
            courseMandatoryDone.insert(key);
        }
    }

    // 第二轮：补齐每场剩余监考员
    for (auto& e : exams) {
        const QString sKey = mkSlotKey(e);

        while (e.assignedTeacherIds.size() < e.requiredInvigilators) {
            int bestScore = -1;
            int bestIdx = -1;

            for (int i = 0; i < states.size(); ++i) {
                auto& cand = states[i];

                // 已在该场则跳过
                if (e.assignedTeacherIds.contains(cand.t.id)) continue;

                // 教授不参与补充/巡考
                if (cand.t.isProfessor()) continue;

                // 忙碌/冲突
                if (isBusy(cand.t, sKey)) continue;
                if (hasTimeConflict(cand, sKey)) continue;

                const int sc = scoreCandidate(cand, e, avgTarget, maxAllowed, globalDateLoad);
                if (sc > bestScore) {
                    bestScore = sc;
                    bestIdx = i;
                }
            }

            if (bestIdx < 0) {
                r.success = false;
                r.reason = QString("考场 %1 (%2 %3 %4) 无法补齐监考员：可能是人手不足或特殊要求/冲突过多")
                               .arg(e.id, e.date, e.timeSlot, e.roomName);
                return r;
            }

            // 分配
            auto& chosen = states[bestIdx];
            e.assignedTeacherIds.push_back(chosen.t.id);
            chosen.assignedSlots.insert(sKey);
            chosen.t.assignedCount += 1;
            chosen.t.assignedExamIds.push_back(e.id);
            chosen.dateCount[e.date] += 1;
        }
    }

    // 第三轮：平衡修正（差<=1）
    QString balanceReason;
    if (!tryBalance(states, exams, 1, &balanceReason)) {
        r.success = false;
        r.reason = QString("排班已填满但无法满足均衡性(差<=1)：%1")
                       .arg(balanceReason);
        return r;
    }

    // 输出结果
    QList<Teacher> outTeachers;
    outTeachers.reserve(states.size());
    for (const auto& s : states) outTeachers.push_back(s.t);

    SchedulePlan plan;
    plan.planId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    plan.createdTime = QDateTime::currentDateTime().toString(Qt::ISODate);
    plan.createdBy = operatorId;
    plan.note = note;

    for (const auto& e : exams) {
        plan.assignments.insert(e.id, e.assignedTeacherIds);
    }

    r.success = true;
    r.teachers = outTeachers;
    r.exams = exams;
    r.plan = plan;
    return r;
}

bool Scheduler::tryBalance(QList<TState>& states,
                          QList<ExamSession>& exams,
                          int maxDiffAllowed,
                          QString* reason) {
    // 只看非教授
    auto nonProfIdx = [&]() {
        QList<int> idx;
        for (int i = 0; i < states.size(); ++i) if (!states[i].t.isProfessor()) idx.push_back(i);
        return idx;
    }();

    if (nonProfIdx.isEmpty()) return true;

    auto calcMinMax = [&]() {
        int mn = INT_MAX;
        int mx = INT_MIN;
        for (int i : nonProfIdx) {
            mn = qMin(mn, states[i].t.assignedCount);
            mx = qMax(mx, states[i].t.assignedCount);
        }
        return qMakePair(mn, mx);
    };

    // 最多尝试有限次交换
    for (int iter = 0; iter < 200; ++iter) {
        const auto [mn, mx] = calcMinMax();
        if (mx - mn <= maxDiffAllowed) return true;

        // 找到一个“忙”老师和一个“闲”老师
        int high = -1, low = -1;
        for (int i : nonProfIdx) {
            if (states[i].t.assignedCount == mx) { high = i; break; }
        }
        for (int i : nonProfIdx) {
            if (states[i].t.assignedCount == mn) { low = i; break; }
        }
        if (high < 0 || low < 0) break;

        // 从 high 的一个非硬性场次里拿出，给 low
        bool swapped = false;

        // 先建立“任课老师硬性场次”集合：teacherId|courseName|examId
        QSet<QString> mandatory;
        QSet<QString> seen;
        for (const auto& e : exams) {
            if (e.category != ExamCategory::NORMAL) continue;
            const QString key = e.mainTeacherId + "|" + e.courseName;
            if (!seen.contains(key)) {
                seen.insert(key);
                mandatory.insert(key + "|" + e.id);
            }
        }

        for (auto& e : exams) {
            const QString sKey = mkSlotKey(e);
            if (!e.assignedTeacherIds.contains(states[high].t.id)) continue;

            // 不能动任课老师的硬性那一次
            const QString mkey = e.mainTeacherId + "|" + e.courseName + "|" + e.id;
            if (mandatory.contains(mkey) && states[high].t.id == e.mainTeacherId) continue;

            // low 必须可用
            if (e.assignedTeacherIds.contains(states[low].t.id)) continue;
            if (isBusy(states[low].t, sKey)) continue;
            if (hasTimeConflict(states[low], sKey)) continue;

            // 执行替换
            for (int k = 0; k < e.assignedTeacherIds.size(); ++k) {
                if (e.assignedTeacherIds[k] == states[high].t.id) {
                    e.assignedTeacherIds[k] = states[low].t.id;
                    break;
                }
            }

            states[high].t.assignedCount -= 1;
            states[low].t.assignedCount += 1;

            states[high].assignedSlots.remove(sKey);
            states[low].assignedSlots.insert(sKey);

            // assignedExamIds 粗粒度维护（用于查询/展示，不做严格顺序）
            states[high].t.assignedExamIds.removeAll(e.id);
            states[low].t.assignedExamIds.push_back(e.id);

            states[high].dateCount[e.date] = qMax(0, states[high].dateCount.value(e.date, 0) - 1);
            states[low].dateCount[e.date] += 1;

            swapped = true;
            break;
        }

        if (!swapped) {
            if (reason) *reason = "无法找到可交换的场次（时间冲突或忙碌时段限制）";
            return false;
        }
    }

    if (reason) *reason = "迭代上限内未达到差<=1";
    return false;
}
