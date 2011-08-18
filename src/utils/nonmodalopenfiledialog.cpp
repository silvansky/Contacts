#include "nonmodalopenfiledialog.h"
#include <QThread>
#ifdef Q_WS_WIN
# include <Windows.h>
#endif

class FileDialogThread : public QThread
{
	Q_OBJECT
public:
	QString title;
	QString filter;
protected:
	void run()
	{
#ifdef Q_WS_WIN
		OPENFILENAME ofn;       // common dialog box structure
		wchar_t szFile[260];       // buffer for file name
		HWND hwnd = NULL;              // owner window

		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hwnd;
		ofn.lpstrFile = szFile;
		// Set lpstrFile[0] to '\0' so that GetOpenFileName does not
		// use the contents of szFile to initialize itself.
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(szFile);
		QString mfilter = filter.replace(QChar('|'), QChar('\0'));
		wchar_t * filter_w = new wchar_t[mfilter.length() + 1];
		mfilter.toWCharArray(filter_w);
		filter_w[mfilter.length()] = '\0';
		ofn.lpstrFilter = filter_w;
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		wchar_t * title_w = new wchar_t[title.length() + 1];
		title.toWCharArray(title_w);
		title_w[title.length()] = '\0';
		ofn.lpstrTitle = title_w;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

		// Display the Open dialog box.

		if (GetOpenFileName(&ofn))
			emit finished(QString::fromWCharArray(ofn.lpstrFile));
		else
			emit finished(QString::null);
		delete title_w;
		delete filter_w;
#endif
		exec();
	}
signals:
	void finished(const QString & fileName);
};

NonModalOpenFileDialog::NonModalOpenFileDialog() :
	QObject(NULL)
{
	_autoDelete = true;
	thread = new FileDialogThread;
	connect(thread, SIGNAL(finished(const QString &)), SLOT(threadFinished(const QString &)));
}

NonModalOpenFileDialog::~NonModalOpenFileDialog()
{
	thread->terminate();
	thread->deleteLater();
}

void NonModalOpenFileDialog::show(const QString & title, const QString & filter, bool autoDelete)
{
	_autoDelete = autoDelete;
	thread->title = title;
	thread->filter = filter;
	thread->start();
}

void NonModalOpenFileDialog::threadFinished(const QString & fileName)
{
	if (fileName.isEmpty())
		emit rejected();
	else
		emit fileNameSelected(fileName);
	if (_autoDelete)
		deleteLater();
}

#include "nonmodalopenfiledialog.moc"
