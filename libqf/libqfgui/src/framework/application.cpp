#include "application.h"

#include <qf/core/log.h>
#include <qf/core/assert.h>
#include <qf/core/utils/fileutils.h>
#include <qf/core/sql/qxsql.h>

#include <QQmlEngine>
#include <QQmlContext>
#include <QFile>
#include <QJsonParseError>

namespace qfu = qf::core::utils;
using namespace qf::gui::framework;

Application::Application(int &argc, char **argv) :
	Super(argc, argv)
{
}

qint64 Application::createDbRecord(const QString &table, const QVariantMap &record) const
{
	using namespace qf::core::sql;
	QxSql sql;
	connect(&sql, &QxSql::dbRecChng, this, &Application::dbRecChng);
	return sql.createRecord(table, record);
}

std::optional<QVariantMap> Application::readDbRecord(const QString &table, qint64 id, const std::optional<QStringList> &fields) const
{
	using namespace qf::core::sql;
	QxSql sql;
	return sql.readRecord(table, id, fields);
}

bool Application::updateDbRecord(const QString &table, qint64 id, const QVariantMap &record) const
{
	using namespace qf::core::sql;
	QxSql sql;
	connect(&sql, &QxSql::dbRecChng, this, &Application::dbRecChng);
	return sql.updateRecord(table, id, record);
}

bool Application::deleteDbRecord(const QString &table, qint64 id) const
{
	using namespace qf::core::sql;
	QxSql sql;
	connect(&sql, &QxSql::dbRecChng, this, &Application::dbRecChng);
	return sql.deleteRecord(table, id);
}

QString Application::versionString() const
{
	return QCoreApplication::applicationVersion();
}

void Application::emitDbRecInserted(const QString &table, qint64 id, const QVariantMap &record, const QString &issuer)
{
	emit dbRecChng(qf::core::sql::RecChng{
		.table = table,
		.id = id,
		.record = record,
		.op = qf::core::sql::RecOp::Insert,
		.issuer = issuer
	});
}

void Application::emitDbRecUpdated(const QString &table, qint64 id, const QVariantMap &record, const QString &issuer)
{
	emit dbRecChng(qf::core::sql::RecChng{
		.table = table,
		.id = id,
		.record = record,
		.op = qf::core::sql::RecOp::Update,
		.issuer = issuer
	});
}

void Application::emitDbRecDeleted(const QString &table, qint64 id, const QString &issuer)
{
	emit dbRecChng(qf::core::sql::RecChng{
		.table = table,
		.id = id,
		.record = {},
		.op = qf::core::sql::RecOp::Delete,
		.issuer = issuer
	});
}

Application *Application::instance(bool must_exist)
{
	auto *ret = qobject_cast<Application*>(Super::instance());
	if(!ret && must_exist) {
		throw std::runtime_error("qf::gui::framework::Application instance MUST exist.");
	}
	return ret;
}

MainWindow *Application::frameWork()
{
	QF_ASSERT_EX(m_frameWork != nullptr, "FrameWork is not set.");
	return m_frameWork;
}

QString Application::applicationDirPath()
{
	return QCoreApplication::applicationDirPath();
}

QString Application::applicationName()
{
	return QCoreApplication::applicationName();
}

QStringList Application::arguments()
{
	return QCoreApplication::arguments();
}

void Application::loadStyleSheet(const QString &file)
{
	QString css_file_name = file;
	if(css_file_name.isEmpty()) {
		QString app_name = Application::applicationName().toLower();
		css_file_name = qfu::FileUtils::joinPath(Application::applicationDirPath(), "/" + app_name + "-data/style/default.css");
		if(!QFile::exists(css_file_name))
			css_file_name = ":/" + app_name + "/style/default.css";
	}
	qfInfo() << "Opening style sheet:" << css_file_name;
	QFile f(css_file_name);
	if(f.open(QFile::ReadOnly)) {
		QByteArray ba = f.readAll();
		QString ss = QString::fromUtf8(ba);
		setStyleSheet(ss);
	}
	else {
		qfWarning() << "Cannot open style sheet:" << css_file_name;
	}
}

