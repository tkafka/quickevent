import sys

path = 'quickevent/app/quickevent/plugins/Runs/src/runsplugin.cpp'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Fix appendVacantsToClassTable
old_append = '''void RunsPlugin::appendVacantsToClassTable(qf::core::utils::TreeTable &tt2, const qf::core::utils::TreeTableRow &tt_row, quickevent::gui::ReportOptionsDialog::VacantsOption vacants_option)
{
	if(start_interval > 0 && vacants_option != quickevent::gui::ReportOptionsDialog::VacantsOption::OnlyRunners) {'''
new_append = '''void RunsPlugin::appendVacantsToClassTable(qf::core::utils::TreeTable &tt2, const qf::core::utils::TreeTableRow &tt_row, quickevent::gui::ReportOptionsDialog::VacantsOption vacants_option)
{
	int start_time_0 = tt_row.value(QStringLiteral("startTimeMin")).toInt() * 60 * 1000;
	int start_time_last = tt_row.value(QStringLiteral("lastStartTimeMin")).toInt() * 60 * 1000;
	int start_interval = tt_row.value(QStringLiteral("startIntervalMin")).toInt() * 60 * 1000;
	if(start_interval > 0 && vacants_option != quickevent::gui::ReportOptionsDialog::VacantsOption::OnlyRunners) {'''
content = content.replace(old_append, new_append)

# 2. Fix startListClassesTable where I forgot to remove the variables
content = content.replace(
'''	for(int i=0; i<tt.rowCount(); i++) {
		qf::core::utils::TreeTableRow tt_row = tt.row(i);
		int start_time_0 = tt_row.value(QStringLiteral("startTimeMin")).toInt() * 60 * 1000;
		int start_time_last = tt_row.value(QStringLiteral("lastStartTimeMin")).toInt() * 60 * 1000;
		int start_interval = tt_row.value(QStringLiteral("startIntervalMin")).toInt() * 60 * 1000;''',
'''	for(int i=0; i<tt.rowCount(); i++) {
		qf::core::utils::TreeTableRow tt_row = tt.row(i);'''
)

# 3. Fix sort and unused start00_epoch_sec in startListStartersTable
old_starters = '''	for(int i=0; i<tt_classes.rowCount(); i++) {
		qf::core::utils::TreeTableRow tt_class_row = tt_classes.row(i);
		qf::core::utils::TreeTable tt2 = tt_class_row.table();
		QString class_name = tt_class_row.value("classes.name").toString();

		for(int j=0; j<tt2.rowCount(); j++) {
			qf::core::utils::TreeTableRow tt2_row = tt2.row(j);
			int ix = tt.appendRow();
			qf::core::utils::TreeTableRow tt_row = tt.row(ix);

			tt_row.setValue("competitors.registration", tt2_row.value("registration"));
			tt_row.setValue("competitors.id", tt2_row.value("competitors.id"));
			tt_row.setValue("competitors.startNumber", tt2_row.value("competitors.startNumber"));
			tt_row.setValue("competitors.country", tt2_row.value("competitors.country"));
			tt_row.setValue("competitorName", tt2_row.value("competitorName"));

			int startTimeMs = tt2_row.value("startTimeMs").toInt();
			tt_row.setValue("startTimeMin", startTimeMs / 1000 / 60);
			tt_row.setValue("runs.siId", tt2_row.value("runs.siId"));
			tt_row.setValue("startTimeMs", startTimeMs);
			tt_row.setValue("classes.name", class_name);
			tt_row.setValue("startTimeText", tt2_row.value("startTimeText"));
			tt_row.setValue("startTimeMsText", tt2_row.value("startTimeMsText"));

			tt.setRow(ix, tt_row);
		}
	}
	tt.sort(QStringList() << "startTimeMs" << "classes.name" << "competitorName");

	return tt;
}'''

new_starters = '''	QList<QVariantMap> rowsData;

	for(int i=0; i<tt_classes.rowCount(); i++) {
		qf::core::utils::TreeTableRow tt_class_row = tt_classes.row(i);
		qf::core::utils::TreeTable tt2 = tt_class_row.table();
		QString class_name = tt_class_row.value("classes.name").toString();

		for(int j=0; j<tt2.rowCount(); j++) {
			qf::core::utils::TreeTableRow tt2_row = tt2.row(j);
			QVariantMap rowMap;
			
			rowMap["competitors.registration"] = tt2_row.value("registration");
			rowMap["competitors.id"] = tt2_row.value("competitors.id");
			rowMap["competitors.startNumber"] = tt2_row.value("competitors.startNumber");
			rowMap["competitors.country"] = tt2_row.value("competitors.country");
			rowMap["competitorName"] = tt2_row.value("competitorName");

			int startTimeMs = tt2_row.value("startTimeMs").toInt();
			rowMap["startTimeMin"] = startTimeMs / 1000 / 60;
			rowMap["runs.siId"] = tt2_row.value("runs.siId");
			rowMap["startTimeMs"] = startTimeMs;
			rowMap["classes.name"] = class_name;
			rowMap["startTimeText"] = tt2_row.value("startTimeText");
			rowMap["startTimeMsText"] = tt2_row.value("startTimeMsText");

			rowsData.append(rowMap);
		}
	}
	
	std::sort(rowsData.begin(), rowsData.end(), [](const QVariantMap &a, const QVariantMap &b) {
		int tA = a["startTimeMs"].toInt();
		int tB = b["startTimeMs"].toInt();
		if (tA != tB) return tA < tB;
		QString cA = a["classes.name"].toString();
		QString cB = b["classes.name"].toString();
		if (cA != cB) return cA < cB;
		return a["competitorName"].toString() < b["competitorName"].toString();
	});

	for(const QVariantMap &rowMap : std::as_const(rowsData)) {
		int ix = tt.appendRow();
		qf::core::utils::TreeTableRow tt_row = tt.row(ix);
		for(auto it = rowMap.constBegin(); it != rowMap.constEnd(); ++it) {
			tt_row.setValue(it.key(), it.value());
		}
		tt.setRow(ix, tt_row);
	}

	return tt;
}'''

content = content.replace(old_starters, new_starters)

content = content.replace('auto start00_epoch_sec = getPlugin<EventPlugin>()->stageStartDateTime(stage_id).toSecsSinceEpoch();\n\n\tauto tt_classes', 'auto tt_classes')

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
