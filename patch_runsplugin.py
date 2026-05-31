import re

path = 'quickevent/app/quickevent/plugins/Runs/src/runsplugin.cpp'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. startListClassesTable signature
content = content.replace(
    'qf::core::utils::TreeTable RunsPlugin::startListClassesTable(const QString &where_expr, const bool insert_vacants, const quickevent::gui::ReportOptionsDialog::StartTimeFormat start_time_format)',
    'qf::core::utils::TreeTable RunsPlugin::startListClassesTable(const QString &where_expr, const quickevent::gui::ReportOptionsDialog::VacantsOption vacants_option, const quickevent::gui::ReportOptionsDialog::StartTimeFormat start_time_format)'
)

# 2. Add vacants fields to query
content = content.replace(
    '.select2("classdefs", "startTimeMin, lastStartTimeMin, startIntervalMin")',
    '.select2("classdefs", "startTimeMin, lastStartTimeMin, startIntervalMin, vacantsBefore, vacantEvery, vacantsAfter")'
)

# 3. Vacants logic
old_vacants_logic = '''		if(start_interval > 0 && insert_vacants) {
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
		}'''

new_vacants_logic = '''		if(start_interval > 0 && vacants_option != quickevent::gui::ReportOptionsDialog::VacantsOption::OnlyRunners) {
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
			int vacantsBefore = tt_row.value(QStringLiteral("vacantsBefore")).toInt();
			int vacantEvery = tt_row.value(QStringLiteral("vacantEvery")).toInt();
			int vacantsAfter = tt_row.value(QStringLiteral("vacantsAfter")).toInt();
			int cnt = tt2.rowCount();
			int vacants = (vacantEvery > 0) ? cnt / vacantEvery : 0;
			int total_vacants = vacantsBefore + vacants + vacantsAfter;
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

content = content.replace(old_vacants_logic, new_vacants_logic)

# 4. XML 3.0 Export
content = content.replace(
    'QString RunsPlugin::startListStageIofXml30(int stage_id, bool with_vacants)',
    'QString RunsPlugin::startListStageIofXml30(int stage_id, quickevent::gui::ReportOptionsDialog::VacantsOption vacants_option)'
)

content = content.replace(
    'bool RunsPlugin::exportStartListStageIofXml30(int stage_id, const QString &file_name, bool with_vacants)',
    'bool RunsPlugin::exportStartListStageIofXml30(int stage_id, const QString &file_name, quickevent::gui::ReportOptionsDialog::VacantsOption vacants_option)'
)

content = content.replace(
    'QString str = startListStageIofXml30(stage_id, with_vacants);',
    'QString str = startListStageIofXml30(stage_id, vacants_option);'
)

content = content.replace(
    'auto tt1 = startListClassesTable("", with_vacants, quickevent::gui::ReportOptionsDialog::StartTimeFormat::RelativeToClassStart);',
    'auto tt1 = startListClassesTable("", vacants_option, quickevent::gui::ReportOptionsDialog::StartTimeFormat::RelativeToClassStart);'
)


# 5. UI calls mappings
content = content.replace(
    'auto tt = startListClassesTable(dlg.sqlWhereExpression(getPlugin<EventPlugin>()->currentStageId()), dlg.isStartListPrintVacants(), dlg.startTimeFormat());',
    'auto tt = startListClassesTable(dlg.sqlWhereExpression(getPlugin<EventPlugin>()->currentStageId()), dlg.startListPrintVacantsOption(), dlg.startTimeFormat());'
)

content = content.replace(
    'qf::core::utils::TreeTable tt1 = startListClassesTable("", false, quickevent::gui::ReportOptionsDialog::StartTimeFormat::DayTime);',
    'qf::core::utils::TreeTable tt1 = startListClassesTable("", quickevent::gui::ReportOptionsDialog::VacantsOption::OnlyRunners, quickevent::gui::ReportOptionsDialog::StartTimeFormat::DayTime);'
)

content = content.replace(
    'auto tt1 = startListClassesTable(sql_where, true, quickevent::gui::ReportOptionsDialog::StartTimeFormat::DayTime);',
    'auto tt1 = startListClassesTable(sql_where, quickevent::gui::ReportOptionsDialog::VacantsOption::RegularVacants, quickevent::gui::ReportOptionsDialog::StartTimeFormat::DayTime);'
)

content = content.replace(
    'auto tt1 = startListClassesTable("",true, quickevent::gui::ReportOptionsDialog::StartTimeFormat::DayTime);',
    'auto tt1 = startListClassesTable("", quickevent::gui::ReportOptionsDialog::VacantsOption::RegularVacants, quickevent::gui::ReportOptionsDialog::StartTimeFormat::DayTime);'
)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
