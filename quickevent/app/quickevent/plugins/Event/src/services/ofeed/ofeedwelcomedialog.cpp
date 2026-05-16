#include "ofeedwelcomedialog.h"
#include "ui_ofeedwelcomedialog.h"

#include <QLocale>

namespace Event::services {

static QString docsBaseUrl()
{
	const bool isCzech = QLocale().language() == QLocale::Czech;
	return isCzech
		? QStringLiteral("https://docs.orienteerfeed.com/cs")
		: QStringLiteral("https://docs.orienteerfeed.com");
}

OFeedWelcomeDialog::OFeedWelcomeDialog(QWidget *parent)
	: Super(parent)
	, ui(new Ui::OFeedWelcomeDialog)
{
	ui->setupUi(this);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	const QString base = docsBaseUrl();
	ui->lbLinks->setText(
		QStringLiteral("<a href=\"%1/\">%2</a>"
					   "&nbsp;&nbsp;|&nbsp;&nbsp;"
					   "<a href=\"%3/category/tutorials/\">%4</a>"
					   "&nbsp;&nbsp;|&nbsp;&nbsp;"
					   "<a href=\"%5/integrations/quickevent/#how-does-the-service-work\">%6</a>")
			.arg(base, tr("About"))
			.arg(base, tr("Tutorials"))
			.arg(base, tr("How it works"))
	);

	connect(ui->btGotIt, &QPushButton::clicked, this, &OFeedWelcomeDialog::accept);
}

OFeedWelcomeDialog::~OFeedWelcomeDialog()
{
	delete ui;
}

} // namespace Event::services
