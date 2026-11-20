#include "ScheduleWidget.h"

#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>
#include<QtGlobal>
#if QT_VERSION>= QT_VERSION_CHECK(6,0,0)
#include<QStringConverter>
#endif
#include "app/AppContext.h"
#include "model/Models.h"
#include "services/Scheduler.h"

using namespace model;

ScheduleWidget::ScheduleWidget(AppContext* ctx, QWidget* parent)
    : QWidget(parent), m_ctx(ctx) {
    m_planSelect = new QComboBox(this);
    m_note = new QLineEdit(this);
    m_note->setPlaceholderText("方案备注（可选）...");

    m_generate = new QPushButton("开始自动排班并保存方案", this);
    m_apply = new QPushButton("应用选中方案", this);
    m_updateNote = new QPushButton("更新备注", this);
    m_delete = new QPushButton("删除选中方案", this);
    m_export = new QPushButton("导出CSV", this);

    connect(m_generate, &QPushButton::clicked, this, &ScheduleWidget::onGenerate);
    connect(m_apply, &QPushButton::clicked, this, &ScheduleWidget::onApplySelectedPlan);
    connect(m_updateNote, &QPushButton::clicked, this, &ScheduleWidget::onUpdateNote);
    connect(m_delete, &QPushButton::clicked, this, &ScheduleWidget::onDeletePlan);
    connect(m_export, &QPushButton::clicked, this, &ScheduleWidget::onExportCsv);

    m_resultHint = new QTextEdit(this);
    m_resultHint->setReadOnly(true);
    m_resultHint->setMinimumHeight(90);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels({"考场编号", "日期", "时间段", "教室", "科目", "类别", "需求人数", "已安排(老师ID)"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto* top = new QHBoxLayout();
    top->addWidget(new QLabel("方案：", this));
    top->addWidget(m_planSelect, 2);
    top->addWidget(new QLabel("备注：", this));
    top->addWidget(m_note, 3);
    top->addWidget(m_generate);

    auto* actions = new QHBoxLayout();
    actions->addWidget(m_apply);
    actions->addWidget(m_updateNote);
    actions->addWidget(m_delete);
    actions->addWidget(m_export);
    actions->addStretch();

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addLayout(actions);
    layout->addWidget(m_resultHint);
    layout->addWidget(m_table);
    setLayout(layout);

    refreshPlans();
}

void ScheduleWidget::onUpdateNote() {
    if (m_planSelect->count() <= 0) return;
    const QString planId = m_planSelect->currentData().toString();

    bool ok = false;
    const QString newNote = QInputDialog::getText(this, "更新备注", "请输入新的方案备注：", QLineEdit::Normal, m_note->text(), &ok);
    if (!ok) return;

    QString err;
    const QString op = m_ctx->auth().currentUser().username;
    if (!m_ctx->data().updatePlanNote(planId, newNote.trimmed(), op, &err)) {
        QMessageBox::warning(this, "更新失败", err);
        return;
    }

    m_note->setText(newNote.trimmed());
    refreshPlans();
    // 尝试回到原来选中项
    for (int i = 0; i < m_planSelect->count(); ++i) {
        if (m_planSelect->itemData(i).toString() == planId) {
            m_planSelect->setCurrentIndex(i);
            break;
        }
    }
    showPlanInTable(planId);

    m_resultHint->setStyleSheet("color:#1b5e20;");
    m_resultHint->setText(QString("已更新方案备注：%1").arg(planId));
}

void ScheduleWidget::refreshPlans() {
    m_planSelect->clear();
    const auto& plans = m_ctx->data().plans();
    for (const auto& p : plans) {
        const QString label = QString("%1 | %2 | %3").arg(p.planId.left(8), p.createdTime, p.note);
        m_planSelect->addItem(label, p.planId);
    }

    if (m_planSelect->count() > 0) {
        showPlanInTable(m_planSelect->currentData().toString());
    } else {
        m_table->setRowCount(0);
        m_resultHint->setText("尚无排班方案。请先添加教师与考场，然后点击“开始自动排班并保存方案”。");
    }
}

void ScheduleWidget::onGenerate() {
    const QString op = m_ctx->auth().currentUser().username;

    Scheduler scheduler;
    const auto result = scheduler.generate(m_ctx->data().teachers(), m_ctx->data().exams(), op, m_note->text().trimmed());

    if (!result.success) {
        m_resultHint->setStyleSheet("color:#b71c1c;");
        m_resultHint->setText("排班失败：\n" + result.reason + "\n\n若方案为0种，请减少限制（忙碌时段）、增加教师人数或调整考场设置。");
        return;
    }

    // 保存方案
    QString err;
    if (!m_ctx->data().addPlan(result.plan, op, &err)) {
        QMessageBox::warning(this, "保存方案失败", err);
        return;
    }

    QString applyError;
    if (!applyPlanToRuntime(result.plan.planId, &applyError)) {
        m_resultHint->setStyleSheet("color:#b71c1c;");
        m_resultHint->setText("排班方案已保存，但应用失败：\n" + applyError);
        return;
    }

    m_resultHint->setStyleSheet("color:#1b5e20;");
    m_resultHint->setText(QString("排班成功：已生成并保存方案 %1\n备注：%2")
                              .arg(result.plan.planId)
                              .arg(result.plan.note));

    refreshPlans();
    // 选中新方案
    for (int i = 0; i < m_planSelect->count(); ++i) {
        if (m_planSelect->itemData(i).toString() == result.plan.planId) {
            m_planSelect->setCurrentIndex(i);
            break;
        }
    }
    showPlanInTable(result.plan.planId);
}

void ScheduleWidget::showPlanInTable(const QString& planId) {
    auto* plan = m_ctx->data().findPlanById(planId);
    if (!plan) return;

    m_table->setRowCount(0);

    for (const auto& e : m_ctx->data().exams()) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);

        const QStringList tids = plan->assignments.value(e.id);

        m_table->setItem(row, 0, new QTableWidgetItem(e.id));
        m_table->setItem(row, 1, new QTableWidgetItem(e.date));
        m_table->setItem(row, 2, new QTableWidgetItem(e.timeSlot));
        m_table->setItem(row, 3, new QTableWidgetItem(e.roomName));
        m_table->setItem(row, 4, new QTableWidgetItem(e.courseName));
        m_table->setItem(row, 5, new QTableWidgetItem(model::categoryToString(e.category)));
        m_table->setItem(row, 6, new QTableWidgetItem(QString::number(e.requiredInvigilators)));
        m_table->setItem(row, 7, new QTableWidgetItem(tids.join(",")));

        m_table->setRowHeight(row, 42);
    }
}

bool ScheduleWidget::applyPlanToRuntime(const QString& planId, QString* err) {
    auto* plan = m_ctx->data().findPlanById(planId);
    if (!plan) {
        if (err) *err = "未找到要应用的方案";
        return false;
    }

    // exams
    for (auto& e : m_ctx->data().exams()) {
        e.assignedTeacherIds = plan->assignments.value(e.id);
    }
    if (!m_ctx->data().saveExams()) {
        if (err) *err = "保存考试安排失败";
        return false;
    }

    // teachers
    for (auto& t : m_ctx->data().teachers()) {
        t.assignedCount = 0;
        t.assignedExamIds.clear();
    }
    for (auto it = plan->assignments.begin(); it != plan->assignments.end(); ++it) {
        const QString examId = it.key();
        for (const auto& tid : it.value()) {
            auto* tp = m_ctx->data().findTeacherById(tid);
            if (!tp) continue;
            tp->assignedCount += 1;
            tp->assignedExamIds.push_back(examId);
        }
    }
    if (!m_ctx->data().saveTeachers()) {
        if (err) *err = "保存教师监考统计失败";
        return false;
    }

    return true;
}

void ScheduleWidget::onApplySelectedPlan() {
    if (m_planSelect->count() <= 0) return;
    const QString planId = m_planSelect->currentData().toString();
    QString err;
    if (!applyPlanToRuntime(planId, &err)) {
        m_resultHint->setStyleSheet("color:#b71c1c;");
        m_resultHint->setText("应用方案失败：" + err);
        return;
    }
    showPlanInTable(planId);
    m_resultHint->setStyleSheet("color:#1b5e20;");
    m_resultHint->setText(QString("已应用方案：%1（教师监考次数/查询结果已更新）").arg(planId));
}

void ScheduleWidget::onDeletePlan() {
    if (m_planSelect->count() <= 0) return;

    const QString planId = m_planSelect->currentData().toString();

    bool ok = false;
    const QString reason = QInputDialog::getText(this, "删除原因", "请输入删除原因（例如：方案作废/条件变更）：", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    QString err;
    const QString op = m_ctx->auth().currentUser().username;
    if (!m_ctx->data().deletePlan(planId, op, reason, &err)) {
        QMessageBox::warning(this, "删除失败", err);
        return;
    }

    refreshPlans();
}

void ScheduleWidget::onExportCsv() {
    if (m_planSelect->count() <= 0) return;
    const QString planId = m_planSelect->currentData().toString();

    auto* plan = m_ctx->data().findPlanById(planId);
    if (!plan) return;

    const QString path = QFileDialog::getSaveFileName(this, "导出监考表", QString("schedule_%1.csv").arg(planId.left(8)), "CSV (*.csv)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::warning(this, "导出失败", "无法写入文件：" + path);
        return;
    }

    QTextStream out(&f);
#if QT_VERSION<QT_VERSION_CHECK(6,0,0)
    out.setCodec("UTF-8");
#else
    out.setEncoding(QStringConverter::Utf8);
#endif
    out << "planId,createdTime,examId,date,timeSlot,room,course,category,required,teacherIds\n";
    for (const auto& e : m_ctx->data().exams()) {
        const QStringList tids = plan->assignments.value(e.id);
        const QStringList values = {
            plan->planId,
            plan->createdTime,
            e.id,
            e.date,
            e.timeSlot,
            e.roomName,
            e.courseName,
            model::categoryToString(e.category),
            QString::number(e.requiredInvigilators),
            tids.join(";")
        };
        for (int i = 0; i < values.size(); ++i) {
            if (i > 0) out << ',';
            out << model::csvEscape(values[i]);
        }
        out << '\n';
    }
    f.close();

    QMessageBox::information(this, "导出成功", "已导出到：" + path);
}
