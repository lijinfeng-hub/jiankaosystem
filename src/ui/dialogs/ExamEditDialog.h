#pragma once

#include <QDialog>

#include "model/Models.h"

class QComboBox;
class QDateEdit;
class QLineEdit;
class QSpinBox;

class AppContext;

class ExamEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit ExamEditDialog(AppContext* ctx, const model::ExamSession* editingExam, QWidget* parent = nullptr);

    model::ExamSession exam() const;

private slots:
    void onFieldsChanged();

private:
    AppContext* m_ctx;
    bool m_isEdit = false;

    QLineEdit* m_id;
    QDateEdit* m_date;
    QLineEdit* m_timeSlot;
    QLineEdit* m_room;

    QComboBox* m_category;
    QComboBox* m_roomType;

    QLineEdit* m_course;
    QComboBox* m_mainTeacher;
    QSpinBox* m_required;

    model::ExamSession m_exam;

    void loadFrom(const model::ExamSession& e);
    void syncToModel();
    void populateTeachers();
};
