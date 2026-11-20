#include "QueryWidget.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include "app/AppContext.h"
#include "model/Models.h"

using namespace model;

QueryWidget::QueryWidget(AppContext* ctx, QWidget* parent)
    : QWidget(parent), m_ctx(ctx) {
    m_name = new QLineEdit(this);
    m_name->setPlaceholderText("输入教师姓名，回车或点击查询...");

    m_query = new QPushButton("查询", this);
    connect(m_query, &QPushButton::clicked, this, &QueryWidget::onQuery);
    connect(m_name, &QLineEdit::returnPressed, this, &QueryWidget::onQuery);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({"考场编号", "日期", "时间段", "教室", "科目", "类别", "同场监考老师ID"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto* top = new QHBoxLayout();
    top->addWidget(m_name);
    top->addWidget(m_query);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addWidget(m_table);
    setLayout(layout);
}

void QueryWidget::onQuery() {
    showTeacher(m_name->text());
}

void QueryWidget::showTeacher(const QString& name) {
    const QString n = name.trimmed();
    if (n.isEmpty()) return;

    auto* t = m_ctx->data().findTeacherByName(n);
    if (!t) {
        QMessageBox::information(this, "未找到", "教师库中不存在该姓名：" + n);
        return;
    }

    m_table->setRowCount(0);

    for (const auto& examId : t->assignedExamIds) {
        auto* e = m_ctx->data().findExamById(examId);
        if (!e) continue;

        const int row = m_table->rowCount();
        m_table->insertRow(row);

        m_table->setItem(row, 0, new QTableWidgetItem(e->id));
        m_table->setItem(row, 1, new QTableWidgetItem(e->date));
        m_table->setItem(row, 2, new QTableWidgetItem(e->timeSlot));
        m_table->setItem(row, 3, new QTableWidgetItem(e->roomName));
        m_table->setItem(row, 4, new QTableWidgetItem(e->courseName));
        m_table->setItem(row, 5, new QTableWidgetItem(model::categoryToString(e->category)));
        m_table->setItem(row, 6, new QTableWidgetItem(e->assignedTeacherIds.join(",")));

        m_table->setRowHeight(row, 42);
    }

    if (t->assignedExamIds.isEmpty()) {
        QMessageBox::information(this, "查询结果", "该老师目前无监考安排（请先生成/应用排班方案）");
    }
}
