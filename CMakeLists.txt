cmake_minimum_required(VERSION 3.16)

project(jiankaosystem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prefer Qt6, fallback to Qt5
find_package(Qt6 COMPONENTS Widgets Core Gui QUIET)
if (Qt6_FOUND)
  set(QT_PACKAGE Qt6)
else()
  find_package(Qt5 COMPONENTS Widgets Core Gui REQUIRED)
  set(QT_PACKAGE Qt5)
endif()

set(SOURCES
  src/main.cpp
  src/app/AppContext.cpp
  src/app/AppContext.h
  src/app/AppInfo.h

  src/model/Models.h
  src/model/Models.cpp

  src/services/AuthManager.cpp
  src/services/AuthManager.h
  src/services/DataManager.cpp
  src/services/DataManager.h
  src/services/LogManager.cpp
  src/services/LogManager.h
  src/services/Scheduler.cpp
  src/services/Scheduler.h

  src/ui/LoginDialog.cpp
  src/ui/LoginDialog.h
  src/ui/MainWindow.cpp
  src/ui/MainWindow.h

  src/ui/widgets/TeachersWidget.cpp
  src/ui/widgets/TeachersWidget.h
  src/ui/widgets/ExamsWidget.cpp
  src/ui/widgets/ExamsWidget.h
  src/ui/widgets/ScheduleWidget.cpp
  src/ui/widgets/ScheduleWidget.h
  src/ui/widgets/QueryWidget.cpp
  src/ui/widgets/QueryWidget.h
  src/ui/widgets/LogsWidget.cpp
  src/ui/widgets/LogsWidget.h

  src/ui/dialogs/TeacherEditDialog.cpp
  src/ui/dialogs/TeacherEditDialog.h
  src/ui/dialogs/ExamEditDialog.cpp
  src/ui/dialogs/ExamEditDialog.h
)

if (${QT_PACKAGE} STREQUAL "Qt6")
  qt_standard_project_setup()
  qt_add_executable(jiankaosystem
    MANUAL_FINALIZATION
    ${SOURCES}
  )
  target_link_libraries(jiankaosystem PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui)
  qt_finalize_executable(jiankaosystem)
else()
  add_executable(jiankaosystem ${SOURCES})
  target_link_libraries(jiankaosystem PRIVATE Qt5::Widgets Qt5::Core Qt5::Gui)
endif()

target_include_directories(jiankaosystem PRIVATE src)

include(CTest)
if (BUILD_TESTING)
  add_executable(models_contract_test
    tests/ModelsContractTest.cpp
    src/model/Models.cpp
    src/model/Models.h
    src/services/DataManager.cpp
    src/services/DataManager.h
    src/services/LogManager.cpp
    src/services/LogManager.h
  )
  target_include_directories(models_contract_test PRIVATE src)
  target_link_libraries(models_contract_test PRIVATE ${QT_PACKAGE}::Core)
  add_test(NAME models_contract_test COMMAND models_contract_test)

  add_executable(scheduler_data_contract_test
    tests/SchedulerDataContractTest.cpp
    src/model/Models.cpp
    src/model/Models.h
    src/services/DataManager.cpp
    src/services/DataManager.h
    src/services/LogManager.cpp
    src/services/LogManager.h
    src/services/Scheduler.cpp
    src/services/Scheduler.h
  )
  target_include_directories(scheduler_data_contract_test PRIVATE src)
  target_link_libraries(scheduler_data_contract_test PRIVATE ${QT_PACKAGE}::Core)
  add_test(NAME scheduler_data_contract_test COMMAND scheduler_data_contract_test)
  set_tests_properties(scheduler_data_contract_test PROPERTIES
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  )
endif()

# Copy runtime assets (e.g. window icon) next to the executable after build.
add_custom_command(TARGET jiankaosystem POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets
          $<TARGET_FILE_DIR:jiankaosystem>/assets
  COMMENT "Copy assets/ to output directory"
)
