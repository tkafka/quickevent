#pragma once
#include <QDialog>
#include <QDate>
#include <QList>
#include <QString>
#include <QStringList>

class QTableWidget;
class QLineEdit;

namespace Event {

class RowColorDelegate; // defined in openeventdialog.cpp

class OpenEventDialog : public QDialog
{
	Q_OBJECT
public:
	struct EventInfo {
		QString id;
		QDate date;
		QString name;
		int sportId = 0;
		int disciplineId = 0;
		int dbVersion = 0;
	};

	enum class RowAction { Open, Convert, Delete };

	OpenEventDialog(const QList<EventInfo> &events, int appDbVersion,
	                const QStringList &existing_names, QWidget *parent = nullptr);

	QString selectedEventId() const { return m_selectedEventId; }
	QString convertedEventId() const { return m_convertedEventId; }
	RowAction selectedAction() const { return m_selectedAction; }

private:
	void onOpenClicked(const QString &event_id);
	void onConvertClicked(const QString &event_id, const QStringList &existing_names, int app_db_version);
	void onDeleteClicked(const QString &event_id);
	void applyFilter();

	QTableWidget *m_table = nullptr;
	QLineEdit *m_searchEdit = nullptr;
	RowColorDelegate *m_delegate = nullptr;
	QString m_selectedEventId;
	QString m_convertedEventId;
	RowAction m_selectedAction = RowAction::Open;
};

} // namespace Event
