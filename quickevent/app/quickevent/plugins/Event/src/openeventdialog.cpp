#include "openeventdialog.h"
#include "eventconfig.h"

#include <qf/core/utils.h>
#include <qf/core/collator.h>
#include <qf/gui/style.h>

#include <QCoreApplication>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QTimer>

namespace Event {

// ── Delegate: prevents hover from greying out custom-coloured cells,
//             highlights cells matching the search needle in yellow ──────────
class RowColorDelegate : public QStyledItemDelegate
{
public:
	using QStyledItemDelegate::QStyledItemDelegate;

	void setNeedle(const QByteArray &needle) { m_needle = needle; }

	void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
	{
		QStyledItemDelegate::initStyleOption(option, index);
		if (!m_needle.isEmpty()) {
			const QString text = index.data(Qt::DisplayRole).toString();
			const QByteArray hay = qf::core::Collator::toAscii7(QLocale::Czech, text, true);
			if (hay.contains(m_needle)) {
				option->backgroundBrush = QBrush(QColor(220, 185, 0));
				option->palette.setColor(QPalette::Text, Qt::black);
				option->state &= ~QStyle::State_MouseOver;
				return;
			}
		}
		if (index.data(Qt::BackgroundRole).isValid())
			option->state &= ~QStyle::State_MouseOver;
	}

private:
	QByteArray m_needle;
};

namespace {

// ── QTableWidgetItem that sorts by an integer stored in UserRole ───────────────
class IntSortItem : public QTableWidgetItem
{
public:
	IntSortItem(const QString &text, int sort_key) : QTableWidgetItem(text)
	{
		setData(Qt::UserRole, sort_key);
		setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
	}
	bool operator<(const QTableWidgetItem &other) const override
	{
		return data(Qt::UserRole).toInt() < other.data(Qt::UserRole).toInt();
	}
};

// ── helpers ────────────────────────────────────────────────────────────────────
QString sportName(int sport_id)
{
	switch (static_cast<EventConfig::Sport>(sport_id)) {
	case EventConfig::Sport::OB:    return QStringLiteral("OB");
	case EventConfig::Sport::LOB:   return QStringLiteral("LOB");
	case EventConfig::Sport::MTBO:  return QStringLiteral("MTBO");
	case EventConfig::Sport::TRAIL: return QStringLiteral("Trail");
	}
	return QString();
}

QString disciplineName(int disc_id)
{
	switch (static_cast<EventConfig::Discipline>(disc_id)) {
	case EventConfig::Discipline::LongDistance:      return QCoreApplication::translate("OpenEventDialog", "Long");
	case EventConfig::Discipline::ShortDistance:     return QCoreApplication::translate("OpenEventDialog", "Middle");
	case EventConfig::Discipline::Sprint:            return QCoreApplication::translate("OpenEventDialog", "Sprint");
	case EventConfig::Discipline::UltralongDistance: return QCoreApplication::translate("OpenEventDialog", "Ultralong");
	case EventConfig::Discipline::Relays:            return QCoreApplication::translate("OpenEventDialog", "Relays");
	case EventConfig::Discipline::Teams:             return QCoreApplication::translate("OpenEventDialog", "Teams");
	case EventConfig::Discipline::FreeOrder:         return QCoreApplication::translate("OpenEventDialog", "Free order");
	case EventConfig::Discipline::NightRace:         return QCoreApplication::translate("OpenEventDialog", "Night");
	case EventConfig::Discipline::TempO:             return QCoreApplication::translate("OpenEventDialog", "TempO");
	case EventConfig::Discipline::MultiStages:       return QCoreApplication::translate("OpenEventDialog", "Multi stages");
	case EventConfig::Discipline::MassStart:         return QCoreApplication::translate("OpenEventDialog", "Mass start");
	case EventConfig::Discipline::SprintRelays:      return QCoreApplication::translate("OpenEventDialog", "Sprint relays");
	case EventConfig::Discipline::KnocOutSprint:     return QCoreApplication::translate("OpenEventDialog", "Knock-out");
	case EventConfig::Discipline::Indoor:            return QCoreApplication::translate("OpenEventDialog", "Indoor");
	}
	return QString();
}

enum Col { ColId = 0, ColDate, ColName, ColSport, ColDiscipline, ColDbVersion, ColAction, ColCount };

} // namespace

OpenEventDialog::OpenEventDialog(const QList<EventInfo> &events, int appDbVersion,
                                 const QStringList &existing_names, QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Open event"));
	setMinimumSize(1000, 450);

	const bool dark = qf::gui::isDarkTheme();
	const QColor compatible_bg = dark ? QColor(30,  90, 30)  : QColor(200, 245, 200);
	const QColor older_bg      = dark ? QColor(90,  25, 25)  : QColor(255, 190, 190);

	auto *rootLayout = new QVBoxLayout(this);
	rootLayout->setContentsMargins(6, 6, 6, 6);
	rootLayout->setSpacing(4);

	// ── search bar ────────────────────────────────────────────────────────────
	auto *searchRow = new QHBoxLayout();
	m_searchEdit = new QLineEdit(this);
	m_searchEdit->setPlaceholderText(tr("Filter events…"));
	m_searchEdit->setClearButtonEnabled(true);
	searchRow->addWidget(new QLabel(tr("Search:"), this));
	searchRow->addWidget(m_searchEdit, 1);
	rootLayout->addLayout(searchRow);

	// ── table ─────────────────────────────────────────────────────────────────
	m_table = new QTableWidget(this);
	m_table->setColumnCount(ColCount);
	m_table->setHorizontalHeaderLabels({
		tr("ID"), tr("Date"), tr("Name"),
		tr("Sport"), tr("Discipline"), tr("DB version"), tr("Action")
	});
	m_table->setRowCount(events.count());
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setSelectionMode(QAbstractItemView::SingleSelection);
	m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_table->setAlternatingRowColors(false);
	m_table->setSortingEnabled(true);
	m_table->verticalHeader()->setVisible(false);
	m_table->setShowGrid(false);

	m_delegate = new RowColorDelegate(m_table);
	m_table->setItemDelegate(m_delegate);

	auto *hdr = m_table->horizontalHeader();
	hdr->setSectionResizeMode(ColId,         QHeaderView::Interactive);
	hdr->setSectionResizeMode(ColDate,        QHeaderView::Interactive);
	hdr->setSectionResizeMode(ColName,        QHeaderView::Stretch);
	hdr->setSectionResizeMode(ColSport,       QHeaderView::Interactive);
	hdr->setSectionResizeMode(ColDiscipline,  QHeaderView::Interactive);
	hdr->setSectionResizeMode(ColDbVersion,   QHeaderView::Interactive);
	hdr->setSectionResizeMode(ColAction,      QHeaderView::Fixed);
	hdr->setStretchLastSection(false);

	// ── right-click header context menu ──────────────────────────────────────
	hdr->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(hdr, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
		auto *hdr = m_table->horizontalHeader();
		const int col = hdr->logicalIndexAt(pos);
		QMenu menu(this);
		if (col >= 0 && col != ColAction) {
			menu.addAction(tr("Resize section to contents"), [this, col]() {
				m_table->resizeColumnToContents(col);
			});
		}
		menu.addAction(tr("Resize all sections to contents"), [this]() {
			m_table->resizeColumnsToContents();
			m_table->setColumnWidth(ColAction, 110);
		});
		menu.exec(hdr->viewport()->mapToGlobal(pos));
	});

	// ── populate rows ─────────────────────────────────────────────────────────
	m_table->setSortingEnabled(false);

	const QIcon deleteIcon = qf::gui::Style::icon(QStringLiteral("delete"));

	for (int row = 0; row < events.count(); ++row) {
		const EventInfo &info = events[row];
		const bool older = (info.dbVersion > 0 && info.dbVersion < appDbVersion);

		QColor bg;
		QString tooltip;
		if (info.dbVersion >= appDbVersion) {
			bg = compatible_bg;
		}
		else if (older) {
			bg = older_bg;
		}

		auto makeItem = [&](const QString &text, Qt::AlignmentFlag align = Qt::AlignLeft) {
			auto *item = new QTableWidgetItem(text);
			item->setTextAlignment(align | Qt::AlignVCenter);
			if (bg.isValid())
				item->setBackground(bg);
			if (!tooltip.isEmpty())
				item->setToolTip(tooltip);
			return item;
		};

		const QString dbVerStr = info.dbVersion > 0
			? qf::core::Utils::intToVersionString(info.dbVersion)
			: tr("?");

		m_table->setItem(row, ColId,        makeItem(info.id));
		m_table->setItem(row, ColDate,       makeItem(
			info.date.isValid() ? info.date.toString(QStringLiteral("yyyy-MM-dd")) : QString(),
			Qt::AlignCenter));
		m_table->setItem(row, ColName,       makeItem(info.name));
		m_table->setItem(row, ColSport,      makeItem(sportName(info.sportId),          Qt::AlignCenter));
		m_table->setItem(row, ColDiscipline, makeItem(disciplineName(info.disciplineId), Qt::AlignCenter));

		auto *verItem = new IntSortItem(dbVerStr, info.dbVersion);
		if (bg.isValid())        verItem->setBackground(bg);
		if (!tooltip.isEmpty())  verItem->setToolTip(tooltip);
		m_table->setItem(row, ColDbVersion, verItem);

		// ── action cell: [Open/Convert] [Delete] ─────────────────────────────
		auto *openConvertBtn = new QPushButton(older ? tr("Convert") : tr("Open"), this);
		if (older) {
			openConvertBtn->setToolTip(tr("Create a converted copy: version %1 → %2")
			                           .arg(dbVerStr)
			                           .arg(qf::core::Utils::intToVersionString(appDbVersion)));
			const QString eventId = info.id;
			connect(openConvertBtn, &QPushButton::clicked, this,
			        [this, eventId, existing_names, appDbVersion]() {
				onConvertClicked(eventId, existing_names, appDbVersion);
			});
		}
		else {
			if (!tooltip.isEmpty()) openConvertBtn->setToolTip(tooltip);
			const QString eventId = info.id;
			connect(openConvertBtn, &QPushButton::clicked, this, [this, eventId]() {
				onOpenClicked(eventId);
			});
		}

		auto *deleteBtn = new QPushButton(this);
		deleteBtn->setIcon(deleteIcon);
		deleteBtn->setFlat(true);
		deleteBtn->setToolTip(tr("Delete event permanently"));
		{
			const QString eventId = info.id;
			connect(deleteBtn, &QPushButton::clicked, this, [this, eventId]() {
				onDeleteClicked(eventId);
			});
		}

		auto *cell = new QWidget(this);
		auto *cellLayout = new QHBoxLayout(cell);
		cellLayout->setContentsMargins(3, 1, 3, 1);
		cellLayout->setSpacing(2);
		cellLayout->addWidget(openConvertBtn);
		cellLayout->addWidget(deleteBtn);
		m_table->setCellWidget(row, ColAction, cell);
	}

	m_table->setSortingEnabled(true);

	m_table->setColumnWidth(ColId,        220);
	m_table->setColumnWidth(ColDate,       90);
	m_table->setColumnWidth(ColSport,      55);
	m_table->setColumnWidth(ColDiscipline, 90);
	m_table->setColumnWidth(ColDbVersion,  80);
	m_table->setColumnWidth(ColAction,    110);

	rootLayout->addWidget(m_table);

	// ── legend ────────────────────────────────────────────────────────────────
	auto makeSwatch = [](const QColor &c) {
		return QStringLiteral("<span style='background:%1;border:1px solid gray'>&nbsp;&nbsp;&nbsp;</span>")
		       .arg(c.name());
	};
	auto *legend = new QLabel(this);
	legend->setText(
		makeSwatch(compatible_bg) + QLatin1Char(' ') + tr("Compatible") +
		QStringLiteral("&nbsp;&nbsp;&nbsp;") +
		makeSwatch(older_bg) + QLatin1Char(' ') + tr("Older version (convert required)")
	);
	rootLayout->addWidget(legend);

	// ── cancel button ─────────────────────────────────────────────────────────
	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	rootLayout->addWidget(buttons);

	// ── search / sort filter wiring ───────────────────────────────────────────
	connect(m_searchEdit, &QLineEdit::textChanged, this, &OpenEventDialog::applyFilter);
	connect(m_table->horizontalHeader(), &QHeaderView::sortIndicatorChanged,
	        this, [this](int, Qt::SortOrder) {
		QTimer::singleShot(0, this, &OpenEventDialog::applyFilter);
	});
}

void OpenEventDialog::applyFilter()
{
	const QByteArray needle = qf::core::Collator::toAscii7(QLocale::Czech, m_searchEdit->text(), true);
	m_delegate->setNeedle(needle);
	for (int row = 0; row < m_table->rowCount(); ++row) {
		bool visible = needle.isEmpty();
		for (int col = 0; !visible && col < ColAction; ++col) {
			const auto *item = m_table->item(row, col);
			if (item) {
				const QByteArray hay = qf::core::Collator::toAscii7(QLocale::Czech, item->text(), true);
				if (hay.contains(needle))
					visible = true;
			}
		}
		m_table->setRowHidden(row, !visible);
	}
	m_table->viewport()->update();
}

void OpenEventDialog::onOpenClicked(const QString &event_id)
{
	m_selectedEventId = event_id;
	m_selectedAction  = RowAction::Open;
	accept();
}

void OpenEventDialog::onConvertClicked(const QString &event_id, const QStringList &existing_names, int app_db_version)
{
	const QString suffix = QStringLiteral("_db%1").arg(app_db_version);
	QString suggested = event_id + suffix;
	while (existing_names.contains(suggested))
		suggested += suffix;

	QDialog dlg(this);
	dlg.setWindowTitle(tr("Convert event"));
	auto *ly = new QVBoxLayout(&dlg);
	ly->setSpacing(8);

	auto *infoLabel = new QLabel(
		tr("Convert event <b>%1</b> to the current version.<br>"
		   "A new event will be created with the ID below.").arg(event_id), &dlg);
	infoLabel->setWordWrap(true);
	ly->addWidget(infoLabel);

	ly->addWidget(new QLabel(tr("New event ID:"), &dlg));
	auto *idEdit = new QLineEdit(suggested, &dlg);
	idEdit->setMinimumWidth(350);
	idEdit->selectAll();
	ly->addWidget(idEdit);

	auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
	auto *okBtn = btns->button(QDialogButtonBox::Ok);
	connect(idEdit, &QLineEdit::textChanged, okBtn, [okBtn](const QString &t) {
		okBtn->setEnabled(!t.trimmed().isEmpty());
	});
	connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
	connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
	ly->addWidget(btns);

	if (dlg.exec() != QDialog::Accepted)
		return;

	const QString final_id = idEdit->text().trimmed();
	if (final_id.isEmpty())
		return;

	m_selectedEventId  = event_id;
	m_convertedEventId = final_id;
	m_selectedAction   = RowAction::Convert;
	accept();
}

void OpenEventDialog::onDeleteClicked(const QString &event_id)
{
	const auto res = QMessageBox::warning(this, tr("Delete event"),
		tr("Permanently delete event <b>%1</b>?<br>This action cannot be undone.").arg(event_id),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

	if (res == QMessageBox::Yes) {
		m_selectedEventId = event_id;
		m_selectedAction  = RowAction::Delete;
		accept();
	}
}

} // namespace Event
