#include "drawingganttwidget.h"
#include "ui_drawingganttwidget.h"

#include "classitem.h"
#include "ganttitem.h"
#include "ganttscene.h"

#include <qf/gui/style.h>
#include <qf/gui/toolbar.h>
#include <qf/gui/menubar.h>
#include <qf/gui/action.h>
#include <qf/gui/dialogs/dialog.h>

#include <QLineEdit>
#include <QMessageBox>
#include <QCheckBox>

using namespace drawing;

DrawingGanttWidget::DrawingGanttWidget(QWidget *parent) :
	Super(parent),
	ui(new Ui::DrawingGanttWidget)
{
	setTitle(tr("Draw tool"));
	setPersistentSettingsId("DrawingToolWidget");
	ui->setupUi(this);

	connect(ui->actSave, &QAction::triggered, this, &DrawingGanttWidget::onActSaveTriggered);
	connect(ui->actFind, &QAction::triggered, this, &DrawingGanttWidget::onActFindTriggered);

	ui->actSave->setIcon(qf::gui::Style::instance()->icon("save"));
	ui->actFind->setIcon(qf::gui::Style::instance()->icon("find"));

	ui->actSave->setToolTip(tr("Write start times and start slots of all classes back to the Classes table\naccording to the current layout."));
	ui->actFind->setToolTip(tr("Find class by name"));

	m_ganttScene = new GanttScene(this);
	ui->ganttView->setScene(m_ganttScene);
}

DrawingGanttWidget::~DrawingGanttWidget()
{
	delete ui;
}

void DrawingGanttWidget::settleDownInDialog(qf::gui::dialogs::Dialog *dlg)
{
	qf::gui::ToolBar *tb = dlg->toolBar("main", true);
	tb->addAction(ui->actSave);
	m_edFind = new QLineEdit();
	m_edFind->setMaximumWidth(QFontMetrics(font()).horizontalAdvance('X') * 8);
	m_edFind->setPlaceholderText(tr("Class"));
	m_edFind->setToolTip(tr("Find class by name"));
	connect(m_edFind, &QLineEdit::textEdited, this, &DrawingGanttWidget::onActFindTriggered);
	tb->addWidget(m_edFind);
	tb->addAction(ui->actFind);

	auto *cb_check_runners = new QCheckBox(tr("Runners clash"));
	cb_check_runners->setToolTip(tr("Highlight classes overlapping in time which share the first control\nand whose start intervals would let two runners punch it at the same moment."));
	auto *cb_check_courses = new QCheckBox(tr("Courses clash"));
	cb_check_courses->setToolTip(tr("Highlight classes overlapping in time which run on the same course."));
	auto update_class_check = [this, cb_check_runners, cb_check_courses]() {
		QSet<ClassItem::ClashType> checks;
		if (cb_check_runners->isChecked()) {
			checks << ClassItem::ClashType::RunnersOverlap;
		}
		if (cb_check_courses->isChecked()) {
			checks << ClassItem::ClashType::CourseOverlap;
		}
		m_ganttScene->ganttItem()->setClashTypesToCheck(checks);
	};
	{
		auto *cb = cb_check_runners;
		cb->setChecked(true);
		tb->addWidget(cb);
#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
		connect(cb, &QCheckBox::stateChanged, this, update_class_check);
#else
		connect(cb, &QCheckBox::checkStateChanged, this, update_class_check);
#endif
	}
	{
		auto *cb = cb_check_courses;
		cb->setChecked(true);
		tb->addWidget(cb);
#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
		connect(cb, &QCheckBox::stateChanged, this, update_class_check);
#else
		connect(cb, &QCheckBox::checkStateChanged, this, update_class_check);
#endif
	}
	update_class_check();

	//auto *menu = dlg->menuBar();
	//auto *a_draw = menu->actionForPath("draw");
	//a_draw->setText(tr("&Draw"));
	//a_draw->addActionInto(ui->actSave);
}

void DrawingGanttWidget::load(int stage_id)
{
	setTitle(tr("E%1 Draw tool").arg(stage_id));
	m_ganttScene->load(stage_id);
}

void drawing::DrawingGanttWidget::onActSaveTriggered()
{
	if(QMessageBox::information(this, tr("Save classes start times"),
								tr("All the user edited classes start times and start intervals will be overridden.\n"
								   "Do you want to save your changes?"),
								QMessageBox::Save | QMessageBox::Cancel,
								QMessageBox::Save))
	{

		m_ganttScene->save();
	}
}

void DrawingGanttWidget::onActFindTriggered()
{
	QString txt = m_edFind->text().trimmed().toUpper();
	for(QGraphicsItem *it : m_ganttScene->items()) {
		if(auto *cit = dynamic_cast<ClassItem *>(it)) {
			if(cit->data().className().toUpper().contains(txt)) {
				m_ganttScene->clearSelection();
				cit->setSelected(true);
				return;
			}
		}
	}
}

