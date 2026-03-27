#include "streampreviewdialog.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

StreamPreviewDialog::StreamPreviewDialog(const lsl::stream_info &info, QWidget *parent)
	: QDialog(parent),
	  channelCount(info.channel_count()),
	  isString(info.channel_format() == lsl::cf_string) {

	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(QString("Preview: %1  [%2]")
					   .arg(QString::fromStdString(info.name()),
						   QString::fromStdString(info.type())));
	resize(720, 480);

	auto *vlay = new QVBoxLayout(this);

	// Bilgi satırı
	QString rateStr = (info.nominal_srate() == 0)
		? QString("düzensiz")
		: QString::number(info.nominal_srate()) + " Hz";
	auto *infoLabel = new QLabel(
		QString("Channel: %1  |  Sampling Rate: %2  |  Source: %3")
			.arg(channelCount)
			.arg(rateStr)
			.arg(QString::fromStdString(info.hostname())));
	vlay->addWidget(infoLabel);

	// Tablo: sütun 0 = zaman damgası, 1..N = kanallar
	table = new QTableWidget(this);
	table->setColumnCount(channelCount + 1);
	QStringList headers;
	headers << "Timestamp";
	for (int i = 0; i < channelCount; ++i)
		headers << QString("Channel %1").arg(i + 1);
	table->setHorizontalHeaderLabels(headers);
	table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	for (int i = 1; i <= channelCount; ++i)
		table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->verticalHeader()->setVisible(false);
	table->setAlternatingRowColors(true);
	vlay->addWidget(table);

	// Alt satır: durum + kapat
	auto *botlay = new QHBoxLayout();
	statusLabel = new QLabel("Connecting...");
	botlay->addWidget(statusLabel);
	botlay->addStretch();
	auto *closeBtn = new QPushButton("Close");
	connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
	botlay->addWidget(closeBtn);
	vlay->addLayout(botlay);

	// Inlet aç
	try {
		inlet = std::make_unique<lsl::stream_inlet>(info, 360, 0, false);
		inlet->open_stream(1.0);
	} catch (const std::exception &e) {
		statusLabel->setText(QString("Connection error: %1").arg(e.what()));
		return;
	}

	// Zamanlayıcı: 100 ms'de bir örnek çek
	timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &StreamPreviewDialog::pullSamples);
	timer->start(100);
}

StreamPreviewDialog::~StreamPreviewDialog() {
	if (timer) timer->stop();
	try {
		if (inlet) inlet->close_stream();
	} catch (...) {}
}

void StreamPreviewDialog::pullSamples() {
	int pulled = 0;
	try {
		if (isString) {
			std::vector<std::string> sample(channelCount);
			double ts = 0.0;
			while ((ts = inlet->pull_sample(sample, 0.0)) != 0.0 && pulled < kMaxPullPerTick) {
				addStringRow(ts, sample);
				++pulled;
			}
		} else {
			std::vector<double> sample(channelCount);
			double ts = 0.0;
			while ((ts = inlet->pull_sample(sample, 0.0)) != 0.0 && pulled < kMaxPullPerTick) {
				addNumericRow(ts, sample);
				++pulled;
			}
		}
	} catch (const std::exception &e) {
		statusLabel->setText(QString("Error: %1").arg(e.what()));
		timer->stop();
		return;
	}

	// Maksimum satır sınırını aş
	while (table->rowCount() > kMaxRows)
		table->removeRow(table->rowCount() - 1);

	if (pulled > 0)
		statusLabel->setText(
			QString("%1 new samples  |  Total: %2 rows").arg(pulled).arg(table->rowCount()));
}

void StreamPreviewDialog::addNumericRow(double timestamp, const std::vector<double> &sample) {
	table->insertRow(0);
	table->setItem(0, 0, new QTableWidgetItem(QString::number(timestamp, 'f', 4)));
	for (int c = 0; c < channelCount; ++c)
		table->setItem(0, c + 1, new QTableWidgetItem(QString::number(sample[c], 'g', 6)));
}

void StreamPreviewDialog::addStringRow(double timestamp,
	const std::vector<std::string> &sample) {
	table->insertRow(0);
	table->setItem(0, 0, new QTableWidgetItem(QString::number(timestamp, 'f', 4)));
	for (int c = 0; c < channelCount; ++c)
		table->setItem(0, c + 1, new QTableWidgetItem(QString::fromStdString(sample[c])));
}
