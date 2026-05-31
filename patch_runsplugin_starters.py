import sys

path = 'quickevent/app/quickevent/plugins/Runs/src/runsplugin.cpp'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Update startListStartersTable signature
content = content.replace(
    'qf::core::utils::TreeTable RunsPlugin::startListStartersTable(const QString &where_expr)',
    'qf::core::utils::TreeTable RunsPlugin::startListStartersTable(const QString &where_expr, quickevent::gui::ReportOptionsDialog::VacantsOption vacants_option)'
)

# 2. Extract appendVacantsToClassTable
old_vacants_logic = '''		if(start_interval > 0 && vacants_option != quickevent::gui::ReportOptionsDialog::VacantsOption::OnlyRunners) {
			for(int j=0; j<tt2.rowCount(); j++) {
				qf::core::utils::TreeTableRow tt2_row = tt2.row(j);
				int start_time = tt2_row.value(QStringLiteral("startTimeMs")).toInt();
				//console.info(j, "t0:", start_time_0, start_time_0/60/1000, "start:", start_time, start_time/60/1000)
				while(start_time_0 < start_time) {
					// insert vakant row
					//qfInfo() << "adding row:" << j << (start_time_0 / 60 / 1000);
					tt2.insertRow(j);
					qf::core::utils::TreeTableRow n_row = tt2.row(j);
					n_row.setValue(QStringLiteral("startTimeMs"), start_time_0);
					n_row.setValue(QStringLiteral("competitorName"), "---");
					n_row.setValue(QStringLiteral("registration"), "");
					n_row.setValue(QStringLiteral("siId"), 0);
					n_row.setValue(QStringLiteral("startNumber"), 0);
					start_time_0 += start_interval;
					tt2.setRow(j, n_row);
					j++;
				}
				start_time_0 += start_interval;
			}
			while(start_time_0 <= start_time_last) {
				// insert vakants after
				int ix = tt2.appendRow();
				qf::core::utils::TreeTableRow tt2_row = tt2.row(ix);
				tt2_row.setValue(QStringLiteral("startTimeMs"), start_time_0);
				tt2_row.setValue(QStringLiteral("competitorName"), "---");
				tt2_row.setValue(QStringLiteral("registration"), QString());
				tt2_row.setValue(QStringLiteral("siId"), 0);
				tt2_row.setValue(QStringLiteral("startNumber"), 0);
				tt2.setRow(ix, tt2_row);
				start_time_0 += start_interval;
			}
		} else if (start_interval == 0 && vacants_option == quickevent::gui::ReportOptionsDialog::VacantsOption::AllVacants) {
			int mapCount = tt_row.value(QStringLiteral("mapCount")).toInt();
			int cnt = tt2.rowCount();
			int total_vacants = mapCount - cnt;
			if (total_vacants < 0) total_vacants = 0;
			for (int k = 0; k < total_vacants; ++k) {
				int ix = tt2.appendRow();
				qf::core::utils::TreeTableRow tt2_row = tt2.row(ix);
				tt2_row.setValue(QStringLiteral("startTimeMs"), QVariant());
				tt2_row.setValue(QStringLiteral("competitorName"), "---");
				tt2_row.setValue(QStringLiteral("registration"), QString());
				tt2_row.setValue(QStringLiteral("siId"), 0);
				tt2_row.setValue(QStringLiteral("startNumber"), 0);
				tt2.setRow(ix, tt2_row);
			}
		}'''

new_vacants_logic = '		appendVacantsToClassTable(tt2, tt_row, vacants_option);'

content = content.replace(old_vacants_logic, new_vacants_logic)

append_vacants_func = '''void RunsPlugin::appendVacantsToClassTable(qf::core::utils::TreeTable &tt2, const qf::core::utils::TreeTableRow &tt_row, quickevent::gui::ReportOptionsDialog::VacantsOption vacants_option)
{
''' + old_vacants_logic.replace('		if(', '	if(').replace('		} else', '	} else').replace('			int', '		int').replace('			for', '		for').replace('			while', '		while').replace('				int', '			int').replace('				qf::', '			qf::').replace('				tt2.', '			tt2.').replace('				tt2_', '			tt2_').replace('				n_row', '			n_row').replace('				start_', '			start_').replace('				while', '			while').replace('					tt2.', '				tt2.').replace('					n_row', '				n_row').replace('					start_', '				start_').replace('					//', '				//').replace('					j++;', '				j++;') + '''
}

'''
content = content.replace('qf::core::utils::TreeTable RunsPlugin::startListClubsTable', append_vacants_func + 'qf::core::utils::TreeTable RunsPlugin::startListClubsTable')

# 3. Add competitors.id to qb2 inside startListClassesTable
content = content.replace('.select2("competitors", "lastName, firstName, registration, iofId, startNumber, country, club")', '.select2("competitors", "id, lastName, firstName, registration, iofId, startNumber, country, club")')

# 4. Update report_startListStarters call and dialog
content = content.replace('auto tt = startListStartersTable(dlg.sqlWhereExpression(getPlugin<EventPlugin>()->currentStageId()));', 'auto tt = startListStartersTable(dlg.sqlWhereExpression(getPlugin<EventPlugin>()->currentStageId()), dlg.startListPrintVacantsOption());')
content = content.replace('dlg.setStartListPrintVacantsVisible(false);', 'dlg.setStartListPrintVacantsVisible(true);')

# 5. Redefine startListStartersTable to flatten from startListClassesTable
old_starters = '''qf::core::utils::TreeTable RunsPlugin::startListStartersTable(const QString &where_expr, quickevent::gui::ReportOptionsDialog::VacantsOption vacants_option)
{
	int stage_id = selectedStageId();
	auto start00_epoch_sec = getPlugin<EventPlugin>()->stageStartDateTime(stage_id).toSecsSinceEpoch();

	qfs::QueryBuilder qb;
	qb.select2("competitors", "registration, id, startNumber, country")
			.select("COALESCE(competitors.lastName, '') || ' ' || COALESCE(competitors.firstName, '') AS competitorName")
			.select("COALESCE(runs.startTimeMs / 1000 / 60, 0) AS startTimeMin")
			.select2("runs", "siId, startTimeMs")
			.select2("classes", "name")
			.from("competitors")
			.joinRestricted("competitors.id", "runs.competitorId", "runs.stageId={{stage_id}} AND runs.isRunning", "INNER JOIN")
			.join("competitors.classId", "classes.id")
			.orderBy("runs.startTimeMs, classes.name, competitors.lastName");//.limit(1);
	if(!where_expr.isEmpty())
		qb.where(where_expr);
	QVariantMap qpm;
	qpm["stage_id"] = stage_id;
	qf::gui::model::SqlTableModel m;
	m.setQueryBuilder(qb);
	m.setQueryParameters(qpm);
	m.reload();
	auto tt = m.toTreeTable();
	addStartTimeTextToClass(tt,start00_epoch_sec, quickevent::gui::ReportOptionsDialog::StartTimeFormat::DayTime);
	tt.setValue("stageId", stage_id);
	tt.setValue("event", getPlugin<EventPlugin>()->eventConfig()->value("event"));
	tt.setValue("stageStart", getPlugin<EventPlugin>()->stageStartDateTime(stage_id));
	return tt;
}'''

new_starters = '''qf::core::utils::TreeTable RunsPlugin::startListStartersTable(const QString &where_expr, quickevent::gui::ReportOptionsDialog::VacantsOption vacants_option)
{
	int stage_id = selectedStageId();
	auto start00_epoch_sec = getPlugin<EventPlugin>()->stageStartDateTime(stage_id).toSecsSinceEpoch();

	auto tt_classes = startListClassesTable(where_expr, vacants_option, quickevent::gui::ReportOptionsDialog::StartTimeFormat::DayTime);

	qf::core::utils::TreeTable tt;
	tt.setValue("stageId", stage_id);
	tt.setValue("event", getPlugin<EventPlugin>()->eventConfig()->value("event"));
	tt.setValue("stageStart", getPlugin<EventPlugin>()->stageStartDateTime(stage_id));

	tt.appendColumn("competitors.registration", QMetaType(QMetaType::QString));
	tt.appendColumn("competitors.id", QMetaType(QMetaType::Int));
	tt.appendColumn("competitors.startNumber", QMetaType(QMetaType::Int));
	tt.appendColumn("competitors.country", QMetaType(QMetaType::QString));
	tt.appendColumn("competitorName", QMetaType(QMetaType::QString));
	tt.appendColumn("startTimeMin", QMetaType(QMetaType::Int));
	tt.appendColumn("runs.siId", QMetaType(QMetaType::Int));
	tt.appendColumn("startTimeMs", QMetaType(QMetaType::Int));
	tt.appendColumn("classes.name", QMetaType(QMetaType::QString));
	tt.appendColumn("startTimeText", QMetaType(QMetaType::QString));
	tt.appendColumn("startTimeMsText", QMetaType(QMetaType::QString));

	for(int i=0; i<tt_classes.rowCount(); i++) {
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

content = content.replace(old_starters, new_starters)
with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
