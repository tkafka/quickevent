#include "qxsql.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>

namespace qf::core::sql {

//================================================================
// QxSqlApi
//================================================================
qint64 QxSqlApi::createRecord(const QString &table, const Record &record)
{
	QStringList keys;
	QStringList vals;

	for (auto it = record.begin(); it != record.end(); ++it) {
		keys << it.key();
		vals << (":" + it.key());
	}

	QString qs = QString("INSERT INTO %1 (%2) VALUES (%3) RETURNING id")
			.arg(table, keys.join(", "), vals.join(", "));

	QueryResult result = query(qs, record);

	if (result.rows.isEmpty() || result.rows[0].isEmpty()) {
		throw std::runtime_error("Insert should return an ID");
	}

	bool ok = false;
	qint64 id = result.rows[0][0].toLongLong(&ok);

	if (!ok) {
		throw std::runtime_error("Insert should return an integer ID");
	}

	return id;
}

std::optional<Record> QxSqlApi::readRecord(const QString &table, qint64 id, const std::optional<QStringList> &fields)
{
	auto records = listOneOrMoreRecords(table, fields, id, 1);
	if (!records.isEmpty()) {
		return records.first();
	}
	return std::nullopt;
}

bool QxSqlApi::updateRecord(const QString &table, qint64 id, const Record &record)
{
	QStringList keyVals;

	for (auto it = record.begin(); it != record.end(); ++it) {
		keyVals << QString("%1 = :%1").arg(it.key());
	}

	QString qs = QString("UPDATE %1 SET %2 WHERE id = %3")
			.arg(table, keyVals.join(", "), QString::number(id));

	ExecResult res = exec(qs, record);
	return res.rowsAffected == 1;
}

bool QxSqlApi::deleteRecord(const QString &table, qint64 id)
{
	QString qs =
			QString("DELETE FROM %1 WHERE id = %2").arg(table, QString::number(id));

	ExecResult res = exec(qs, {});
	return res.rowsAffected == 1;
}

QList<Record> QxSqlApi::listOneOrMoreRecords(const QString &table, const std::optional<QStringList> &fields, const std::optional<qint64> &id, const std::optional<qint64> &limit)
{
	QString fieldsStr =
			(fields && !fields->isEmpty()) ? fields->join(", ") : "*";

	QString qs = QString("SELECT %1 FROM %2").arg(fieldsStr, table);

	if (id.has_value()) {
		qs += QString(" WHERE id >= %1").arg(*id);
	}

	if (limit.has_value()) {
		qs += QString(" LIMIT %1").arg(*limit);
	}

	QueryResult result = query(qs, {});

	QList<Record> records;
	records.reserve(result.rows.size()); // still valid for QList (Qt 6)

	for (int i = 0; i < result.rows.size(); ++i) {
		auto rec = result.record(i);
		if (rec.has_value()) {
			records.append(*rec);
		}
	}

	return records;
}

//================================================================
// QxSql
//================================================================
QxSql::QxSql(const QSqlDatabase &db)
	: m_db(db)
{

}

QueryResult QxSql::query(const QString &query, const QVariantMap &params)
{
	QSqlQuery q(m_db);
	q.prepare(query);
	for (const auto &[key, val] : params.asKeyValueRange()) {
		q.bindValue(QStringLiteral(":%1").arg(key), val);
	}
	if (!q.exec()) {
		throw std::runtime_error(q.lastError().text().toStdString());
	}
	QueryResult result;
	for (int i = 0; i < q.record().count(); ++i) {
		result.columns.append(q.record().fieldName(i).toLower());
	}
	while (q.next()) {
		QList<QVariant> row;
		for (int i = 0; i < q.record().count(); ++i) {
			row.append(q.value(i));
		}
		result.rows.insert(result.rows.size(), row);
	}
	return result;
}

ExecResult QxSql::exec(const QString &query, const QVariantMap &params)
{
	QSqlQuery q(m_db);
	q.prepare(query);
	for (const auto &[key, val] : params.asKeyValueRange()) {
		q.bindValue(QStringLiteral(":%1").arg(key), val);
	}
	if (!q.exec()) {
		throw std::runtime_error(q.lastError().text().toStdString());
	}
	ExecResult result;
	result.rowsAffected = q.numRowsAffected();
	return result;
}

qint64 QxSql::createRecord(const QString &table, const Record &record)
{
	auto id = QxSqlApi::createRecord(table, record);
	emit dbRecChng(qf::core::sql::RecChng{.table = table,
										  .id = id,
										  .record = record,
										  .op = qf::core::sql::RecOp::Insert,
										  .issuer = {}});
	return id;
}

bool QxSql::updateRecord(const QString &table, qint64 id, const Record &record)
{
	auto ok = QxSqlApi::updateRecord(table, id, record);
	if (ok) {
		emit dbRecChng(qf::core::sql::RecChng{.table = table,
											  .id = id,
											  .record = record,
											  .op = qf::core::sql::RecOp::Update,
											  .issuer = {}});
	}
	return ok;
}

bool QxSql::deleteRecord(const QString &table, qint64 id)
{
	auto ok = QxSqlApi::deleteRecord(table, id);
	if (ok) {
		emit dbRecChng(qf::core::sql::RecChng{.table = table,
											  .id = id,
											  .record = {},
											  .op = qf::core::sql::RecOp::Delete,
											  .issuer = {}});
	}
	return ok;
}
}
