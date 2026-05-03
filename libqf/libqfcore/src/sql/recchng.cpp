#include "recchng.h"

#include "../core/log.h"

namespace qf::core::sql {
namespace {
RecOp recopFromString(const QString &s)
{
	if (s == "Insert") return RecOp::Insert;
	if (s == "Delete") return RecOp::Delete;
	if (s == "Update") return RecOp::Update;
	qfWarning() << "Undefined record operation:" << s;
	return RecOp::Update;
}
}
QVariantMap RecChng::toVariantMap() const
{
	QVariantMap ret;
	ret["table"] = table;
	ret["op"] = recopToString(op);
	ret["id"] = id;
	if (op != RecOp::Delete) {
		ret["record"] = record;
	}
	if (!issuer.isEmpty()) {
		ret["issuer"] = issuer;
	}
	return ret;
}

RecChng RecChng::fromVariantMap(const QVariantMap &m)
{
	RecChng ret;
	ret.op = recopFromString(m.value("op").toString());
	ret.id = m.value("id").value<qint64>();
	if (ret.op != RecOp::Delete) {
		ret.record = m.value("record").toMap();
	}
	ret.issuer = m.value("issuer").toString();
	return ret;
}

QString RecChng::recopToString(RecOp op)
{
	switch (op) {
	case RecOp::Insert: return "Insert";
	case RecOp::Update: return "Update";
	case RecOp::Delete: return "Delete";
	}
}

}
