#include "ReaderLaunch.h"

#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QUrl>

#include "interface/constants/SettingsConstant.h"
#include "interface/localization.h"

#include "platform/PlatformUtil.h"
#include "util/files.h"

#include "shared/ReaderOpenPath.h"

using namespace HomeCompa;

namespace HomeCompa::Flibrary
{

namespace
{

constexpr auto CONTEXT                     = "ReaderController";
constexpr auto DIALOG_TITLE                = QT_TRANSLATE_NOOP("ReaderController", "Select %1 reader");
constexpr auto DIALOG_FILTER               = QT_TRANSLATE_NOOP("ReaderController", "Applications (*.exe)");
constexpr auto USE_DEFAULT                 = QT_TRANSLATE_NOOP("ReaderController", "Use the default reader?");
constexpr auto CANNOT_START_DEFAULT_READER = QT_TRANSLATE_NOOP("ReaderController", "Cannot start default reader. Will you specify the application manually?");
constexpr auto CANNOT_START_READER         = QT_TRANSLATE_NOOP("ReaderController", "'%1' not found. Will you specify another application?");
constexpr auto UNSUPPORTED                 = QT_TRANSLATE_NOOP("ReaderController", "Unsupported file type");
constexpr auto SELECT_FILE                 = QT_TRANSLATE_NOOP("ReaderController", "Specify the file to be read");

constexpr auto READER_KEY = "Reader/%1";
constexpr auto DIALOG_KEY = "Reader";
constexpr auto DEFAULT    = "default";

TR_DEF

class ReaderProcess final : public QProcess
{
public:
	ReaderProcess(const QString& process, const QString& fileName, std::shared_ptr<ILogicFactory::ITemporaryDir> temporaryDir, QObject* parent = nullptr)
		: QProcess(parent)
		, m_temporaryDir(std::move(temporaryDir))
	{
		start(process, { fileName });
		waitForStarted();
		connect(this, qOverload<int, ExitStatus>(&QProcess::finished), this, &QObject::deleteLater);
	}

private:
	std::shared_ptr<ILogicFactory::ITemporaryDir> m_temporaryDir;
};

} // namespace

bool OpenDefaultReader(const QString& fileName, const QString& ext)
{
	if (!QFile::exists(fileName) || QFileInfo(fileName).size() == 0)
		return false;

#ifdef Q_OS_MACOS
	{
		QStringList args;
		if (ext.compare(QStringLiteral("epub"), Qt::CaseInsensitive) == 0)
			args << QStringLiteral("-a") << QStringLiteral("Books") << fileName;
		else
			args << fileName;

		return QProcess::startDetached(QStringLiteral("/usr/bin/open"), args);
	}
#endif

	return QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
}

void LaunchConfiguredReader(
	ISettings&                                  settings,
	const IUiFactory&                           uiFactory,
	std::shared_ptr<ILogicFactory::ITemporaryDir> temporaryDir,
	QString                                     fileName,
	const QString&                              error,
	const long long                             bookId
)
{
	if (fileName.isEmpty())
	{
		if (!error.isEmpty())
			return uiFactory.ShowError(error);

		fileName = uiFactory.GetOpenFileName({}, Tr(SELECT_FILE), {}, temporaryDir->path());
		if (fileName.isEmpty())
			return;
	}

	if (!temporaryDir)
		return uiFactory.ShowError(error);

	fileName = PrepareReaderFile(fileName, bookId);

	const auto ext = QFileInfo(fileName).suffix();
	if (ext.isEmpty())
		return uiFactory.ShowError(Tr(UNSUPPORTED));

	auto       key    = QString(READER_KEY).arg(ext);
	auto       reader = settings.Get(key).toString();
	const auto getReader = [&] {
		reader = uiFactory.GetOpenFileName(DIALOG_KEY, Tr(DIALOG_TITLE).arg(ext), Tr(DIALOG_FILTER));
		if (reader.isEmpty())
			return;

		if (settings.Get(Constant::Settings::PREFER_RELATIVE_PATHS, false))
			reader = Util::ToRelativePath(reader);
		settings.Set(key, reader);
	};

	if (reader.isEmpty())
	{
		if (Platform::IsRegisteredExtension(ext))
			switch (uiFactory.ShowQuestion(Tr(USE_DEFAULT), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes)) // NOLINT(clang-diagnostic-switch-enum)
			{
				case QMessageBox::Yes:
					settings.Set(key, (reader = DEFAULT));
					break;
				case QMessageBox::No:
					break;
				case QMessageBox::Cancel:
				default:
					return;
			}

		if (reader.isEmpty())
		{
			getReader();
			if (reader.isEmpty())
				return;
		}
	}

	if (reader == DEFAULT)
	{
		if (OpenDefaultReader(fileName, ext))
			return;

		settings.Remove(key);
		if (uiFactory.ShowQuestion(Tr(CANNOT_START_DEFAULT_READER), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes) != QMessageBox::Yes)
			return;

		getReader();
		if (reader.isEmpty())
			return;
	}

	reader = Util::ToAbsolutePath(reader);
	while (!QFile::exists(reader))
	{
		if (uiFactory.ShowQuestion(Tr(CANNOT_START_READER).arg(QFileInfo(reader).fileName()), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes) != QMessageBox::Yes)
			return;

		getReader();
		if (reader.isEmpty())
			return;
	}

	assert(!reader.isEmpty());
	new ReaderProcess(reader, fileName, std::move(temporaryDir), uiFactory.GetParentObject());
}

} // namespace HomeCompa::Flibrary
