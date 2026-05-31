import sys

path = 'quickevent/app/quickevent/plugins/Runs/src/runsplugin.cpp'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

old_logic = '''		for(int j=0; j<tt2.rowCount(); j++) {
			auto tt2_row = tt2.row(j);
			QVariantList xml_person{"PersonStart"};
			QVariantList person{"Person"};
			if (!is_iof_race) {
				append_list(person, QVariantList{"Id", QVariantMap{{"type", "CZE"}}, tt2_row.value(QStringLiteral("competitors.registration"))});
			}
			append_list(person, QVariantList{"Id", QVariantMap{{"type", "QuickEvent"}}, tt2_row.value(QStringLiteral("runs.id"))});
			auto iof_id = tt2_row.value(QStringLiteral("competitors.iofId"));
			if (!iof_id.isNull())
				append_list(person, QVariantList{"Id", QVariantMap{{"type", "IOF"}}, iof_id});
			auto family = tt2_row.value(QStringLiteral("competitors.lastName"));
			auto given = tt2_row.value(QStringLiteral("competitors.firstName"));
			append_list(person, QVariantList{"Name", QVariantList{"Family", family}, QVariantList{"Given", given}});
			if (is_iof_race) {
				auto nationality = tt2_row.value(QStringLiteral("competitors.country"));
				QString nat_code = getClubAbbrFromName(nationality.toString());
				append_list(person, QVariantList{"Nationality", QVariantMap{{"code", nat_code}}, nationality});
			}
			QVariantList xml_start{"Start", (iof_xml_race_number != 0) ? QVariantMap{{"raceNumber", iof_xml_race_number}} : QVariantMap{}};
			auto bib_number = tt2_row.value(QStringLiteral("competitors.startNumber"));
			if(!bib_number.isNull())
				append_list(xml_start, QVariantList{"BibNumber", bib_number});
			if (has_fixed_start_time) {
				int stime_msec = tt2_row.value("startTimeMs").toInt();
				append_list(xml_start, QVariantList{"StartTime", datetime_to_string(start00.addMSecs(stime_msec))});
			}
			QVariant siId = tt2_row.value(QStringLiteral("runs.siId"));
			if (siId.toBool()) {
				append_list(xml_start, QVariantList{"ControlCard", siId.toInt()});
			}
			append_list(xml_person, person);
			if (is_iof_race){
				append_list(xml_person, QVariantList{"Organisation",
											QVariantList{"Id", QVariantMap{{"type", "IOF"}},tt2_row.value(QStringLiteral("clubs.importId"))},
											QVariantList{"Name", tt2_row.value(QStringLiteral("clubs.name"))},
										}
							);
			}
			else {
				auto club_abbr = tt2_row.value(QStringLiteral("clubs.abbr")).toString();
				if (!club_abbr.isEmpty()) {
					append_list(xml_person, QVariantList{"Organisation",
												QVariantList{"Name", tt2_row.value(QStringLiteral("clubs.name"))},
												QVariantList{"ShortName", club_abbr},
											}
					);
				}
				else {
					append_list(xml_person, QVariantList{"Organisation",
												QVariantList{"Name", QString()},
												QVariantList{"ShortName", tt2_row.value(QStringLiteral("competitors.registration")).toString().left(3)}
											}
					);
				}
			}
			append_list(xml_person, xml_start);
			append_list(class_start, xml_person);
		}'''

new_logic = '''		for(int j=0; j<tt2.rowCount(); j++) {
			auto tt2_row = tt2.row(j);
			QVariantList xml_person{"PersonStart"};
			
			bool is_vacant = tt2_row.value(QStringLiteral("competitorName")).toString() == "---";
			
			if (!is_vacant) {
				QVariantList person{"Person"};
				if (!is_iof_race) {
					append_list(person, QVariantList{"Id", QVariantMap{{"type", "CZE"}}, tt2_row.value(QStringLiteral("competitors.registration"))});
				}
				append_list(person, QVariantList{"Id", QVariantMap{{"type", "QuickEvent"}}, tt2_row.value(QStringLiteral("runs.id"))});
				auto iof_id = tt2_row.value(QStringLiteral("competitors.iofId"));
				if (!iof_id.isNull())
					append_list(person, QVariantList{"Id", QVariantMap{{"type", "IOF"}}, iof_id});
				auto family = tt2_row.value(QStringLiteral("competitors.lastName"));
				auto given = tt2_row.value(QStringLiteral("competitors.firstName"));
				append_list(person, QVariantList{"Name", QVariantList{"Family", family}, QVariantList{"Given", given}});
				if (is_iof_race) {
					auto nationality = tt2_row.value(QStringLiteral("competitors.country"));
					QString nat_code = getClubAbbrFromName(nationality.toString());
					append_list(person, QVariantList{"Nationality", QVariantMap{{"code", nat_code}}, nationality});
				}
				append_list(xml_person, person);
			}

			QVariantList xml_start{"Start", (iof_xml_race_number != 0) ? QVariantMap{{"raceNumber", iof_xml_race_number}} : QVariantMap{}};
			auto bib_number = tt2_row.value(QStringLiteral("competitors.startNumber"));
			if(!bib_number.isNull())
				append_list(xml_start, QVariantList{"BibNumber", bib_number});
			if (has_fixed_start_time) {
				int stime_msec = tt2_row.value("startTimeMs").toInt();
				append_list(xml_start, QVariantList{"StartTime", datetime_to_string(start00.addMSecs(stime_msec))});
			}
			QVariant siId = tt2_row.value(QStringLiteral("runs.siId"));
			if (siId.toBool()) {
				append_list(xml_start, QVariantList{"ControlCard", siId.toInt()});
			}

			if (!is_vacant) {
				if (is_iof_race){
					append_list(xml_person, QVariantList{"Organisation",
												QVariantList{"Id", QVariantMap{{"type", "IOF"}},tt2_row.value(QStringLiteral("clubs.importId"))},
												QVariantList{"Name", tt2_row.value(QStringLiteral("clubs.name"))},
											}
								);
				}
				else {
					auto club_abbr = tt2_row.value(QStringLiteral("clubs.abbr")).toString();
					if (!club_abbr.isEmpty()) {
						append_list(xml_person, QVariantList{"Organisation",
													QVariantList{"Name", tt2_row.value(QStringLiteral("clubs.name"))},
													QVariantList{"ShortName", club_abbr},
												}
						);
					}
					else {
						append_list(xml_person, QVariantList{"Organisation",
													QVariantList{"Name", QString()},
													QVariantList{"ShortName", tt2_row.value(QStringLiteral("competitors.registration")).toString().left(3)}
												}
						);
					}
				}
			}
			
			append_list(xml_person, xml_start);
			append_list(class_start, xml_person);
		}'''

content = content.replace(old_logic, new_logic)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
