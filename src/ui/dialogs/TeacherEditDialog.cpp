#include "TeacherEditDialog.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "app/AppContext.h"

using namespace model;

static int RoleSlotKey() { return Qt::UserRole + 1; }
static int RoleReason() { return Qt::UserRole + 2; }

static QString busyDisplayText(const QString& slotKey, const QString& reason) {
    // slotKey: "YYYY-MM-DD|HH:MM-HH:MM"
    const int sep = slotKey.indexOf('|');
    const QString date = (sep >= 0) ? slotKey.left(sep) : slotKey;
    const QString ts = (sep >= 0) ? slotKey.mid(sep + 1) : "";
    const QString r = reason.trimmed().isEmpty() ? "未说明" : reason.trimmed();
    if (!ts.isEmpty()) return QString("%1 (%2) %3").arg(date, ts, r);
    return QString("%1 %2").arg(date, r);
}

TeacherEditDialog::TeacherEditDialog(AppContext* ctx, const Teacher* editingTeacher, QWidget* parent)
    : QDialog(parent), m_ctx(ctx) {
    setWindowTitle(editingTeacher ? "编辑教师" : "新增教师");
    resize(560, 520);

    m_isEdit = (editingTeacher != nullptr);

    m_id = new QLineEdit(this);
    m_name = new QLineEdit(this);

    m_title = new QComboBox(this);
    m_title->addItem("教授", titleToString(Title::PROFESSOR));
    m_title->addItem("副教授", titleToString(Title::ASSOCIATE_PROFESSOR));
    m_title->addItem("讲师", titleToString(Title::LECTURER));
    m_title->addItem("助教", titleToString(Title::ASSISTANT));

    m_courses = new QLineEdit(this);
    m_courses->setPlaceholderText("课程列表（可选，用逗号分隔）");

    if (m_isEdit) m_id->setReadOnly(true);

    auto* form = new QFormLayout();
    form->addRow("工号：", m_id);
    form->addRow("姓名：", m_name);
    form->addRow("职称：", m_title);
    form->addRow("课程：", m_courses);

    // busy
    m_busyDate = new QDateEdit(this);
    m_busyDate->setCalendarPopup(true);
    m_busyDate->setDate(QDate::currentDate());

    m_busyTimeSlot = new QLineEdit(this);
    m_busyTimeSlot->setPlaceholderText("如 09:00-11:00");

    m_busyReason = new QLineEdit(this);
    m_busyReason->setPlaceholderText("出差/生病/其他");

    m_busyAdd = new QPushButton("添加不可监考时段", this);
    m_busyRemove = new QPushButton("移除选中时段", this);

    connect(m_busyAdd, &QPushButton::clicked, this, &TeacherEditDialog::onAddBusy);
    connect(m_busyRemove, &QPushButton::clicked, this, &TeacherEditDialog::onRemoveBusy);

    m_busyList = new QListWidget(this);

    auto* busyTop = new QHBoxLayout();
    busyTop->addWidget(new QLabel("日期：", this));
    busyTop->addWidget(m_busyDate);
    busyTop->addWidget(new QLabel("时间段：", this));
    busyTop->addWidget(m_busyTimeSlot);
    busyTop->addWidget(new QLabel("原因：", this));
    busyTop->addWidget(m_busyReason);

    auto* busyBtns = new QHBoxLayout();
    busyBtns->addWidget(m_busyAdd);
    busyBtns->addWidget(m_busyRemove);
    busyBtns->addStretch();

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        syncToModel();
        if (m_teacher.id.trimmed().isEmpty() || m_teacher.name.trimmed().isEmpty()) {
            QMessageBox::warning(this, "校验失败", "工号与姓名不能为空");
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addSpacing(10);
    layout->addWidget(new QLabel("特殊要求：不可监考时段（会影响排班）", this));
    layout->addLayout(busyTop);
    layout->addLayout(busyBtns);
    layout->addWidget(m_busyList, 1);
    layout->addWidget(buttons);
    setLayout(layout);

    if (editingTeacher) {
        loadFrom(*editingTeacher);
    }
}

void TeacherEditDialog::loadFrom(const Teacher& t) {
    m_teacher = t;
    m_id->setText(t.id);
    m_name->setText(t.name);

    // title
    const QString v = titleToString(t.title);
    for (int i = 0; i < m_title->count(); ++i) {
        if (m_title->itemData(i).toString() == v) {
            m_title->setCurrentIndex(i);
            break;
        }
    }

    m_courses->setText(t.courses.join(","));

    m_busyList->clear();
    for (const auto& b : t.busySlots) {
        auto* item = new QListWidgetItem(busyDisplayText(b.slotKey, b.reason), m_busyList);
        item->setData(RoleSlotKey(), b.slotKey);
        item->setData(RoleReason(), b.reason);
        m_busyList->addItem(item);
    }
}

void TeacherEditDialog::syncToModel() {
    m_teacher.id = m_id->text().trimmed();
    m_teacher.name = m_name->text().trimmed();
    m_teacher.title = titleFromString(m_title->currentData().toString());

    const QStringList cs = m_courses->text().split(',', Qt::SkipEmptyParts);
    m_teacher.courses.clear();
    for (auto c : cs) m_teacher.courses.push_back(c.trimmed());

    // busySlots 从 list 反解析
    m_teacher.busySlots.clear();
    for (int i = 0; i < m_busyList->count(); ++i) {
        auto* it = m_busyList->item(i);
        BusySlot b;
        b.slotKey = it->data(RoleSlotKey()).toString().trimmed();
        b.reason = it->data(RoleReason()).toString().trimmed();

        // 兼容：如果老数据 item 没有 data，就尽量从文本中猜
        if (b.slotKey.isEmpty()) {
            const QString text = it->text();
            // 允许 "YYYY-MM-DD (HH:MM-HH:MM) 原因" 或 "YYYY-MM-DD|HH:MM-HH:MM"
            const int pipe = text.indexOf('|');
            if (pipe >= 0) {
                b.slotKey = text.left(text.indexOf(' ', pipe) >= 0 ? text.indexOf(' ', pipe) : text.size()).trimmed();
            } else {
                b.slotKey = text.section(' ', 0, 0).trimmed();
            }
        }
        if (!b.slotKey.isEmpty()) {
            m_teacher.busySlots.push_back(b);
        }
    }
}

model::Teacher TeacherEditDialog::teacher() const {
    return m_teacher;
}

void TeacherEditDialog::onAddBusy() {
    const QString date = model::normalizeDate(m_busyDate->date());
    const QString ts = model::normalizeTimeSlot(m_busyTimeSlot->text());
    if (ts.isEmpty()) {
        QMessageBox::warning(this, "校验失败", "时间段不能为空（例如 09:00-11:00）");
        return;
    }
    const QString key = model::slotKey(date, ts);
    const QString reason = m_busyReason->text().trimmed();

    // 去重
    for (int i = 0; i < m_busyList->count(); ++i) {
        auto* it = m_busyList->item(i);
        const QString existedKey = it->data(RoleSlotKey()).toString();
        if (!existedKey.isEmpty() ? (existedKey == key) : it->text().contains(key)) {
            QMessageBox::information(this, "已存在", "该时段已在列表中");
            return;
        }
    }

    auto* item = new QListWidgetItem(busyDisplayText(key, reason), m_busyList);
    item->setData(RoleSlotKey(), key);
    item->setData(RoleReason(), reason);
    m_busyList->addItem(item);
}

void TeacherEditDialog::onRemoveBusy() {
    const int row = m_busyList->currentRow();
    if (row < 0) return;
    delete m_busyList->takeItem(row);
}
