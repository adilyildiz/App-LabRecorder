#pragma once

#include <QDialog>
#include <QLabel>
#include <QTableWidget>
#include <QTimer>
#include <memory>

#include <lsl_cpp.h>

class StreamPreviewDialog : public QDialog {
	Q_OBJECT

public:
	explicit StreamPreviewDialog(const lsl::stream_info &info, QWidget *parent = nullptr);
	~StreamPreviewDialog() override;

private slots:
	void pullSamples();

private:
	void addNumericRow(double timestamp, const std::vector<double> &sample);
	void addStringRow(double timestamp, const std::vector<std::string> &sample);

	std::unique_ptr<lsl::stream_inlet> inlet;
	QTableWidget *table;
	QLabel *statusLabel;
	QTimer *timer;
	int channelCount;
	bool isString;

	static constexpr int kMaxRows = 200;
	static constexpr int kMaxPullPerTick = 50;
};
