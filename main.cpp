#include <QCoreApplication>
#include <QResource>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <magic.h>


/// extend QResource class to give access
/// to protected functions
class ExtRes : public QResource
{
public:
    ExtRes(const QString & file = QString(), const QLocale & locale = QLocale())
        : QResource(file, locale)
    {
    }

    virtual ~ExtRes()
    {
    }

    virtual QStringList children() const
    {
        return QResource::children();
    }

    virtual bool isDir() const
    {
        return QResource::isDir();
    }

    virtual bool isFile() const
    {
        return QResource::isFile();
    }
};

/// cout like stuff Qt taste
QTextStream& qStdOut()
{
    static QTextStream ts( stdout );
    return ts;
}

/// tak to run in our application
class Task : public QObject
{
    Q_OBJECT

public:
    Task(QObject *parent = 0) : QObject(parent)
    {
        mCookie = magic_open(MAGIC_MIME_TYPE);
        magic_load(mCookie, NULL);
    }

    ~Task()
    {
        magic_close(mCookie);
    }

protected:

    void parseResources(const ExtRes& res)
    {
        if (res.isValid())
        {
            if (res.isDir())
            {
                foreach (const QString& fName, res.children())
                {
                    QString combName = res.fileName().isEmpty() ? QString(":/%1").arg(fName) : QString("%1/%2").arg(res.fileName()).arg(fName);
                    parseResources(ExtRes(combName));
                }
            }
            else if (res.isFile())
            {
                qStdOut() << "Trying to extract " << res.absoluteFilePath() << " (" << res.size() << "B)\n";
                QFileInfo fi(res.fileName());

                QString outName = QString("%1.raw").arg(fi.baseName());
                QFile::copy(res.fileName(), outName);
                QString tmpName = outName;

                if (!createFileName(tmpName))
                {
                    QFile::rename(outName, tmpName);
                }
            }
        }
        else
        {
            qStdOut() << "No valid resource!\n";
        }
    }

    int createFileName (QString& tmpName)
    {

        QRegExp rx("^(.*)/(.*)$");
        QString n = magic_file(mCookie, tmpName.toUtf8().constData());

        if (rx.indexIn(n) > -1)
        {
            QFileInfo fi(tmpName);
            tmpName = QString("%1.%2").arg(fi.baseName()).arg(rx.cap((2)));
            return 0;
        }

        return -1;
    }

public slots:
    void run()
    {
        QStringList args = QCoreApplication::arguments();

        if (args.count() < 2)
        {
            qStdOut() << "Give the file you want to extract as parameter!\n";
        }
        else
        {
            QResource::registerResource(args.at(1));
            parseResources(ExtRes());
        }

        emit finished();
    }

signals:
    void finished();

private:
    magic_t mCookie;
};

#include "main.moc"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Task parented to the application so that it
    // will be deleted by the application.
    Task *task = new Task(&a);

    // This will cause the application to exit when
    // the task signals finished.
    QObject::connect(task, SIGNAL(finished()), &a, SLOT(quit()));

    // This will run the task from the application event loop.
    QTimer::singleShot(0, task, SLOT(run()));

    return a.exec();
}
