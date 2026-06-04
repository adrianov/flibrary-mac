#include "FlibraryApplication.h"

#include <utility>

#include <QFileInfo>
#include <QFileOpenEvent>
#include <QTimer>

#include "util/Fb2Format.h"

#include "logic/shared/ReaderLaunch.h"

using namespace HomeCompa::Flibrary;

FlibraryApplication::FlibraryApplication(int& argc, char** argv)
	: QApplication(argc, argv)
{
	for (const auto& path : Util::CollectFb2Args(arguments().mid(1)))
		EnqueueFb2(path);
}

void FlibraryApplication::EnqueueFb2(const QString& path)
{
	const auto absolute = QFileInfo(path).absoluteFilePath();
	if (absolute.isEmpty() || m_pendingFb2.contains(absolute))
		return;

	m_pendingFb2.append(absolute);
}

void FlibraryApplication::OpenPendingFb2()
{
	if (m_pendingFb2.isEmpty() || m_fb2OpenScheduled)
		return;

	m_fb2OpenScheduled = true;
	QTimer::singleShot(0, this, [this] {
		m_fb2OpenScheduled = false;
		const auto paths = std::exchange(m_pendingFb2, {});
		for (const auto& path : paths)
			OpenFb2InBooks(path);
	});
}

bool FlibraryApplication::event(QEvent* event)
{
	if (event->type() == QEvent::FileOpen)
	{
		const auto* fileEvent = static_cast<QFileOpenEvent*>(event);
		if (Util::IsFb2Path(fileEvent->file()))
		{
			EnqueueFb2(fileEvent->file());
			OpenPendingFb2();
			return true;
		}
	}

	return QApplication::event(event);
}
