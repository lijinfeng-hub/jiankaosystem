#include "ExamsWidget.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "app/AppContext.h"
#include "model/Models.h"
#include "ui/dialogs/ExamEditDialog.h"

using namespace model;

ExamsWidget::ExamsWidget(AppContext* ctx, QWidget* parent)
    : QWidget(parent), m_ctx(ctx) {
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText("按课程/教室/编号筛选...");

    m_add = new QPushButton("新增", this);
    m_edit = new QPushButton("编辑", this);
    m_del = new QPushButton("删除", this);
    m_refresh = new QPushButton("刷新", this);

    connect(m_add, &QPushButton::clicked, this, &ExamsWidget::onAdd);
    connect(m_edit, &QPushButton::clicked, this, &ExamsWidget::onEdit);
    connect(m_del, &QPushButton::clicked, this, &ExamsWidget::onDelete);
    connect(m_refresh, &QPushButton::clicked, this, &ExamsWidget::refresh);
    connect(m_search, &QLineEdit::textChanged, this, &ExamsWidget::onSearchChanged);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(9);
    m_table->setHorizontalHeaderLabels({"编号", "日期", "时间段", "教室", "类别", "大小", "科目", "任课老师ID", "需求人数"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto* top = new QHBoxLayout();
    top->addWidget(m_search);
    top->addWidget(m_add);
    top->addWidget(m_edit);
    top->addWidget(m_del);
    top->addWidget(m_refresh);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addWidget(m_table);
    setLayout(layout);

    refresh();
}

void ExamsWidget::refresh() {
    rebuildTable(m_search->text());
}

void ExamsWidget::onSearchChanged(const QString& text) {
    rebuildTable(text);
}

void ExamsWidget::rebuildTable(const QString& filter) {
    const QString f = filter.trimmed();
    const auto& list = m_ctx->data().exams();

    m_table->setRowCount(0);

    for (const auto& e : list) {
        if (!f.isEmpty()) {
            const QString hay = (e.id + " " + e.courseName + " " + e.roomName + " " + e.mainTeacherId);
            if (!hay.contains(f, Qt::CaseInsensitive)) continue;
        }

        const int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(e.id));
        m_table->setItem(row, 1, new QTableWidgetItem(e.date));
        m_table->setItem(row, 2, new QTableWidgetItem(e.timeSlot));
        m_table->setItem(row, 3, new QTableWidgetItem(e.roomName));
        m_table->setItem(row, 4, new QTableWidgetItem(model::categoryToString(e.category)));
        m_table->setItem(row, 5, new QTableWidgetItem(model::roomTypeToString(e.type)));
        m_table->setItem(row, 6, new QTableWidgetItem(e.courseName));
        m_table->setItem(row, 7, new QTableWidgetItem(e.mainTeacherId));
        m_table->setItem(row, 8, new QTableWidgetItem(QString::number(e.requiredInvigilators)));

        m_table->setRowHeight(row, 42);
    }
}

QString ExamsWidget::selectedExamId() const {
    const auto ranges = m_table->selectedRanges();
    if (ranges.isEmpty()) return {};
    const int row = ranges.first().topRow();
    auto* item = m_table->item(row, 0);
    return item ? item->text() : QString();
}

void ExamsWidget::onAdd() {
    ExamEditDialog dlg(m_ctx, nullptr, this);
    if (dlg.exec() != QDialog::Accepted) return;

    ExamSession e = dlg.exam();
    QString err;
    const QString op = m_ctx->auth().currentUser().username;
    if (!m_ctx->data().addExam(e, op, &err)) {
        QMessageBox::warning(this, "新增失败", err);
        return;
    }
    refresh();
}

void ExamsWidget::onEdit() {
    const QString id = selectedExamId();
    if (id.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选中一个考场/场次");
        return;
    }

    auto* e = m_ctx->data().findExamById(id);
    if (!e) {
        QMessageBox::warning(this, "错误", "未找到该考场");
        return;
    }

    ExamEditDialog dlg(m_ctx, e, this);
    if (dlg.exec() != QDialog::Accepted) return;

    ExamSession ne = dlg.exam();
    QString err;
    const QString op = m_ctx->auth().currentUser().username;
    if (!m_ctx->data().updateExam(ne, op, &err)) {
        QMessageBox::warning(this, "编辑失败", err);
        return;
    }
    refresh();
}

void ExamsWidget::onDelete() {
    const QString id = selectedExamId();
    if (id.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选中一个考场/场次");
        return;
    }

    bool ok = false;
    const QString reason = QInputDialog::getText(this, "删除原因", "请输入删除原因（例如：取消考试/改期）：", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    QString err;
    const QString op = m_ctx->auth().currentUser().username;
    if (!m_ctx->data().deleteExam(id, op, reason, &err)) {
        QMessageBox::warning(this, "删除失败", err);
        return;
    }
    refresh();
}
