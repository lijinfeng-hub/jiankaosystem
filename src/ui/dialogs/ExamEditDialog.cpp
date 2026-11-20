#include "ExamEditDialog.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>
#include <QVBoxLayout>

#include "app/AppContext.h"
#include "model/Models.h"

using namespace model;

ExamEditDialog::ExamEditDialog(AppContext* ctx, const ExamSession* editingExam, QWidget* parent)
    : QDialog(parent), m_ctx(ctx) {
    setWindowTitle(editingExam ? "编辑考场/场次" : "新增考场/场次");
    resize(560, 420);

    m_isEdit = (editingExam != nullptr);

    m_id = new QLineEdit(this);
    m_id->setReadOnly(true);

    m_date = new QDateEdit(this);
    m_date->setCalendarPopup(true);
    m_date->setDate(QDate::currentDate());

    m_timeSlot = new QLineEdit(this);
    m_timeSlot->setPlaceholderText("如 09:00-11:00");

    m_room = new QLineEdit(this);
    m_room->setPlaceholderText("如 A101 或 PATROL");

    m_category = new QComboBox(this);
    m_category->addItem("正常考试", categoryToString(ExamCategory::NORMAL));
    m_category->addItem("巡考", categoryToString(ExamCategory::PATROL));

    m_roomType = new QComboBox(this);
    m_roomType->addItem("大教室(6人)", roomTypeToString(RoomType::BIG_ROOM));
    m_roomType->addItem("小教室(3人)", roomTypeToString(RoomType::SMALL_ROOM));

    m_course = new QLineEdit(this);
    m_course->setPlaceholderText("考试科目（巡考可留空）");

    m_mainTeacher = new QComboBox(this);
    populateTeachers();

    m_required = new QSpinBox(this);
    m_required->setRange(1, 30);

    connect(m_date, &QDateEdit::dateChanged, this, &ExamEditDialog::onFieldsChanged);
    connect(m_timeSlot, &QLineEdit::textChanged, this, &ExamEditDialog::onFieldsChanged);
    connect(m_room, &QLineEdit::textChanged, this, &ExamEditDialog::onFieldsChanged);
    connect(m_category, &QComboBox::currentIndexChanged, this, &ExamEditDialog::onFieldsChanged);
    connect(m_roomType, &QComboBox::currentIndexChanged, this, &ExamEditDialog::onFieldsChanged);

    auto* form = new QFormLayout();
    form->addRow("编号（自动）：", m_id);
    form->addRow("日期：", m_date);
    form->addRow("时间段：", m_timeSlot);
    form->addRow("教室：", m_room);
    form->addRow("类别：", m_category);
    form->addRow("教室大小：", m_roomType);
    form->addRow("科目：", m_course);
    form->addRow("任课老师：", m_mainTeacher);
    form->addRow("需求人数：", m_required);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        syncToModel();
        if (m_exam.date.trimmed().isEmpty() || m_exam.timeSlot.trimmed().isEmpty() || m_exam.roomName.trimmed().isEmpty()) {
            QMessageBox::warning(this, "校验失败", "日期/时间段/教室不能为空");
            return;
        }
        if (m_exam.category == ExamCategory::NORMAL) {
            if (m_exam.courseName.trimmed().isEmpty() || m_exam.mainTeacherId.trimmed().isEmpty()) {
                QMessageBox::warning(this, "校验失败", "正常考试必须填写科目与任课老师");
                return;
            }
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
    setLayout(layout);

    if (editingExam) {
        loadFrom(*editingExam);
    } else {
        onFieldsChanged();
    }
}

void ExamEditDialog::populateTeachers() {
    m_mainTeacher->clear();
    m_mainTeacher->addItem("(请选择)", "");
    for (const auto& t : m_ctx->data().teachers()) {
        m_mainTeacher->addItem(QString("%1(%2)").arg(t.name, t.id), t.id);
    }
}

void ExamEditDialog::loadFrom(const ExamSession& e) {
    m_exam = e;

    m_id->setText(e.id);
    m_date->setDate(QDate::fromString(e.date, "yyyy-MM-dd"));
    m_timeSlot->setText(e.timeSlot);
    m_room->setText(e.roomName);

    // category
    for (int i = 0; i < m_category->count(); ++i) {
        if (m_category->itemData(i).toString() == categoryToString(e.category)) {
            m_category->setCurrentIndex(i);
            break;
        }
    }

    // roomType
    for (int i = 0; i < m_roomType->count(); ++i) {
        if (m_roomType->itemData(i).toString() == roomTypeToString(e.type)) {
            m_roomType->setCurrentIndex(i);
            break;
        }
    }

    m_course->setText(e.courseName);

    for (int i = 0; i < m_mainTeacher->count(); ++i) {
        if (m_mainTeacher->itemData(i).toString() == e.mainTeacherId) {
            m_mainTeacher->setCurrentIndex(i);
            break;
        }
    }

    m_required->setValue(e.requiredInvigilators);
}

void ExamEditDialog::onFieldsChanged() {
    const QString date = model::normalizeDate(m_date->date());
    const QString ts = model::normalizeTimeSlot(m_timeSlot->text());
    const QString room = m_room->text().trimmed();

    // 自动生成 id
    if (!date.isEmpty() && !ts.isEmpty() && !room.isEmpty()) {
        m_id->setText(model::buildExamId(date, ts, room));
    } else {
        m_id->setText("");
    }

    // 自动设置需求人数
    const auto cat = categoryFromString(m_category->currentData().toString());
    const auto rt = roomTypeFromString(m_roomType->currentData().toString());

    if (cat == ExamCategory::PATROL) {
        m_required->setValue(1);
        m_roomType->setEnabled(false);
        m_course->setEnabled(true);
        m_mainTeacher->setEnabled(false);
    } else {
        m_roomType->setEnabled(true);
        m_mainTeacher->setEnabled(true);
        m_required->setValue(rt == RoomType::BIG_ROOM ? 6 : 3);
    }
}

void ExamEditDialog::syncToModel() {
    m_exam.id = m_id->text().trimmed();
    m_exam.date = model::normalizeDate(m_date->date());
    m_exam.timeSlot = model::normalizeTimeSlot(m_timeSlot->text());
    m_exam.roomName = m_room->text().trimmed();

    m_exam.category = categoryFromString(m_category->currentData().toString());
    m_exam.type = roomTypeFromString(m_roomType->currentData().toString());

    m_exam.courseName = m_course->text().trimmed();
    m_exam.mainTeacherId = m_mainTeacher->currentData().toString();
    m_exam.requiredInvigilators = m_required->value();
}

model::ExamSession ExamEditDialog::exam() const {
    return m_exam;
}
