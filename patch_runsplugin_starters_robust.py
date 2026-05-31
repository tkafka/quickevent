import sys

path = 'quickevent/app/quickevent/plugins/Runs/src/runsplugin.cpp'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

old_logic = '''			rowMap["competitors.registration"] = tt2_row.value("registration");
			rowMap["competitors.id"] = tt2_row.value("competitors.id");
			rowMap["competitors.startNumber"] = tt2_row.value("competitors.startNumber");
			rowMap["competitors.country"] = tt2_row.value("competitors.country");
			rowMap["competitorName"] = tt2_row.value("competitorName");

			QVariant startTimeMsVar = tt2_row.value("startTimeMs");
			if (!startTimeMsVar.isNull())
				rowMap["startTimeMin"] = startTimeMsVar.toInt() / 1000 / 60;
			else
				rowMap["startTimeMin"] = QVariant();
			rowMap["runs.siId"] = tt2_row.value("runs.siId");'''

new_logic = '''			auto val = [tt2_row](const char* key1, const char* key2) {
				QVariant v = tt2_row.value(key1);
				if (v.isValid()) return v;
				return tt2_row.value(key2);
			};

			rowMap["competitors.registration"] = val("competitors.registration", "registration");
			rowMap["competitors.id"] = val("competitors.id", "id");
			rowMap["competitors.startNumber"] = val("competitors.startNumber", "startNumber");
			rowMap["competitors.country"] = val("competitors.country", "country");
			rowMap["competitorName"] = tt2_row.value("competitorName");

			QVariant startTimeMsVar = val("runs.startTimeMs", "startTimeMs");
			if (!startTimeMsVar.isNull())
				rowMap["startTimeMin"] = startTimeMsVar.toInt() / 1000 / 60;
			else
				rowMap["startTimeMin"] = QVariant();
			rowMap["runs.siId"] = val("runs.siId", "siId");'''

content = content.replace(old_logic, new_logic)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
