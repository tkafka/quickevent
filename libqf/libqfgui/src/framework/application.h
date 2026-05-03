#ifndef QF_GUI_FRAMEWORK_APPLICATION_H
#define QF_GUI_FRAMEWORK_APPLICATION_H

#include "../guiglobal.h"

#include <qf/core/sql/recchng.h>

#include <QApplication>
#include <QJsonDocument>
#include <QQmlError>

class QQmlEngine;

namespace qf {
namespace gui {
namespace framework {

class MainWindow;

class QFGUI_DECL_EXPORT Application : public QApplication
{
	Q_OBJECT
	friend class MainWindow;
private:
	typedef QApplication Super;
public:
	explicit Application(int & argc, char ** argv);
	~Application() override = default;

	Q_SIGNAL void dbRecChng(const qf::core::sql::RecChng &recchng);
	qint64 createDbRecord(const QString &table, const QVariantMap &record) const;
	std::optional<QVariantMap> readDbRecord(const QString &table, qint64 id, const std::optional<QStringList> &fields = std::nullopt) const;
	bool updateDbRecord(const QString &table, qint64 id, const QVariantMap &record) const;
	bool deleteDbRecord(const QString &table, qint64 id) const;
	void emitDbRecInserted(const QString &table, qint64 id, const QVariantMap &record, const QString &issuer = {});
	void emitDbRecUpdated(const QString &table, qint64 id, const QVariantMap &record, const QString &issuer = {});
	void emitDbRecDeleted(const QString &table, qint64 id, const QString &issuer = {});

	Q_INVOKABLE QString versionString() const;
public:
	static Application* instance(bool must_exist = true);
	MainWindow* frameWork();

	void loadStyleSheet(const QString &file = QString());

	QString applicationDirPath();
	QString applicationName();
	QStringList arguments();
protected:
	MainWindow* m_frameWork = nullptr;
};

}}}

#endif // QF_GUI_FRAMEWORK_APPLICATION_H
