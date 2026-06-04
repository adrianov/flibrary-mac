#pragma once

#include <QApplication>
#include <QStringList>

namespace HomeCompa::Flibrary
{

class FlibraryApplication final : public QApplication
{
	Q_OBJECT

public:
	FlibraryApplication(int& argc, char** argv);

	void EnqueueFb2(const QString& path);
	void OpenPendingFb2();

protected:
	bool event(QEvent* event) override;

private:
	QStringList m_pendingFb2;
	bool        m_fb2OpenScheduled { false };
};

}
