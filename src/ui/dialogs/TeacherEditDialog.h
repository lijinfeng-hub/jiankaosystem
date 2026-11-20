#pragma once

#include <QDialog>

#include "model/Models.h"

class QComboBox;
class QDateEdit;
class QLineEdit;
class QListWidget;
class QPushButton;

class AppContext;

class TeacherEditDialog : public QDialog {
    Q_OBJECT
public:
    // editingTeacher!=nullptr 表示编辑；nullptr 表示新增
    explicit TeacherEditDialog(AppContext* ctx, const model::Teacher* editingTeacher, QWidget* parent = nullptr);

    model::Teacher teacher() const;

private slots:
    void onAddBusy();
    void onRemoveBusy();

private:
    AppContext* m_ctx;
    bool m_isEdit = false;

    QLineEdit* m_id;
    QLineEdit* m_name;
    QComboBox* m_title;
    QLineEdit* m_courses;

    // busy slot input
    QDateEdit* m_busyDate;
    QLineEdit* m_busyTimeSlot;
    QLineEdit* m_busyReason;
    QPushButton* m_busyAdd;
    QPushButton* m_busyRemove;
    QListWidget* m_busyList;

    model::Teacher m_teacher;

    void loadFrom(const model::Teacher& t);
    void syncToModel();
};
