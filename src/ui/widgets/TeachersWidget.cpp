#include "TeachersWidget.h"

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
#include "ui/dialogs/TeacherEditDialog.h"

using namespace model;

TeachersWidget::TeachersWidget(AppContext* ctx, QWidget* parent)
    : QWidget(parent), m_ctx(ctx) {
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText("按姓名筛选...");

    m_add = new QPushButton("新增", this);
    m_edit = new QPushButton("编辑", this);
    m_del = new QPushButton("删除", this);
    m_refresh = new QPushButton("刷新", this);

    connect(m_add, &QPushButton::clicked, this, &TeachersWidget::onAdd);
    connect(m_edit, &QPushButton::clicked, this, &TeachersWidget::onEdit);
    connect(m_del, &QPushButton::clicked, this, &TeachersWidget::onDelete);
    connect(m_refresh, &QPushButton::clicked, this, &TeachersWidget::refresh);
    connect(m_search, &QLineEdit::textChanged, this, &TeachersWidget::onSearchChanged);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({"工号", "姓名", "职称", "课程(展示)", "监考次数", "特殊要求(不可监考)"});
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

void TeachersWidget::refresh() {
    rebuildTable(m_search->text());
}

void TeachersWidget::onSearchChanged(const QString& text) {
    rebuildTable(text);
}

static QString busyToText(const Teacher& t) {
    QStringList lines;
    for (const auto& b : t.busySlots) {
        const QString reason = b.reason.trimmed().isEmpty() ? "未说明" : b.reason.trimmed();
        lines << QString("%1（%2）").arg(b.slotKey, reason);
    }
    return lines.join("\n");
}

void TeachersWidget::rebuildTable(const QString& filterName) {
    const QString f = filterName.trimmed();
    const auto& list = m_ctx->data().teachers();

    m_table->setRowCount(0);

    for (const auto& t : list) {
        if (!f.isEmpty() && !t.name.contains(f, Qt::CaseInsensitive)) continue;

        const int row = m_table->rowCount();
        m_table->insertRow(row);

        m_table->setItem(row, 0, new QTableWidgetItem(t.id));
        m_table->setItem(row, 1, new QTableWidgetItem(t.name));
        m_table->setItem(row, 2, new QTableWidgetItem(model::titleToString(t.title)));
        m_table->setItem(row, 3, new QTableWidgetItem(t.courses.join(",")));
        m_table->setItem(row, 4, new QTableWidgetItem(QString::number(t.assignedCount)));
        m_table->setItem(row, 5, new QTableWidgetItem(busyToText(t)));

        m_table->setRowHeight(row, 52);
    }
}

QString TeachersWidget::selectedTeacherId() const {
    const auto ranges = m_table->selectedRanges();
    if (ranges.isEmpty()) return {};
    const int row = ranges.first().topRow();
    auto* item = m_table->item(row, 0);
    return item ? item->text() : QString();
}

void TeachersWidget::onAdd() {
    TeacherEditDialog dlg(m_ctx, nullptr, this);
    if (dlg.exec() != QDialog::Accepted) return;

    Teacher t = dlg.teacher();
    QString err;
    const QString op = m_ctx->auth().currentUser().username;
    if (!m_ctx->data().addTeacher(t, op, &err)) {
        QMessageBox::warning(this, "新增失败", err);
        return;
    }
    refresh();
}

void TeachersWidget::onEdit() {
    const QString id = selectedTeacherId();
    if (id.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选中一位老师");
        return;
    }

    auto* t = m_ctx->data().findTeacherById(id);
    if (!t) {
        QMessageBox::warning(this, "错误", "未找到该老师");
        return;
    }

    TeacherEditDialog dlg(m_ctx, t, this);
    if (dlg.exec() != QDialog::Accepted) return;

    Teacher nt = dlg.teacher();
    QString err;
    const QString op = m_ctx->auth().currentUser().username;
    if (!m_ctx->data().updateTeacher(nt, op, &err)) {
        QMessageBox::warning(this, "编辑失败", err);
        return;
    }
    refresh();
}

void TeachersWidget::onDelete() {
    const QString id = selectedTeacherId();
    if (id.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选中一位老师");
        return;
    }

    bool ok = false;
    const QString reason = QInputDialog::getText(this, "删除原因", "请输入删除原因（例如：离职/调岗）：", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    QString err;
    const QString op = m_ctx->auth().currentUser().username;
    if (!m_ctx->data().deleteTeacher(id, op, reason, &err)) {
        QMessageBox::warning(this, "删除失败", err);
        return;
    }
    refresh();
}
