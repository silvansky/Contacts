#ifndef NONMODALOPENFILEDIALOG_H
#define NONMODALOPENFILEDIALOG_H

#include <QObject>
#include "utilsexport.h"

class FileDialogThread;

// NOTE: this class is designed to work only on Windows for now!
// Use instance of QFileDialog on Mac OS X - it will show native dialog.
// Linux behavior is untested.
class UTILS_EXPORT NonModalOpenFileDialog : public QObject
{
	Q_OBJECT
public:
	NonModalOpenFileDialog();
	~NonModalOpenFileDialog();

signals:
	void fileNameSelected(const QString &  fileName);
	void rejected();

public slots:
	void show(const QString & title, const QString & filter, bool autoDelete = true);
private slots:
	void threadFinished(const QString & fileName);
private:
	FileDialogThread * thread;
	bool _autoDelete;
};

#endif // NONMODALOPENFILEDIALOG_H
