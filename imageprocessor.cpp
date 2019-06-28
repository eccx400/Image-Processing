#include "imageprocessor.h"
#include <QThreadPool>
#include <QList>
#include <QFile>
#include <QFileInfo>
#include <QRunnable>
#include <QEvent>
#include <QCoreApplication>
#include <QPointer>
#include <QUrl>
#include <QImage>
#include <QDebug>
#include <QDir>

typedef void (*AlgorithmFunction) (QString sourceFile, QString destFile);

class AlgorithmRunnable;
class ExecutedEvent: public QEvent {
public:
    ExecutedEvent (AlgorithmRunnable * r) : QEvent(evType()), m_runnable(r)
    {

    }
    AlgorithmRunnable * m_runnable;

    static QEvent::Type evType()
    {
        if(s_evType == QEvent::None)
        {
            s_evType = (QEvent::Type) registerEventType();
        }
        return s_evType;
    }

private:
    static QEvent::Type s_evType;
};
QEvent::Type ExecutedEvent::s_evType = QEvent::None;

static void _gray(QString sourceFile, QString destFile);
static void _binarize(QString sourceFile, QString destFile)
{
    QImage image(sourceFile);
    if(image.isNull())
    {
        qDebug() << "load " << sourceFile << " failed! ";
        return;
    }
    int width = image.width();
    int height = image.height();
    QRgb color;
    QRgb avg;
    QRgb black = qRgb(0, 0, 0);
    QRgb white = qRgb(255, 255, 255);

    for(int i = 0; i < width; i++)
    {
        for(int j = 0; j < height; j++)
        {
            color = image.pixel(i, j);
            avg = (qRed(color) + qGreen(color) + qBlue(color)) / 3;
            image.setPixel(i, j, avg >= 128 ? white : black);
        }
        image.save(destFile);
    }
}
static void _negative(QString sourceFile, QString destFile);
static void _emboss(QString sourceFile, QString destFile);
static void _sharpen(QString sourceFile, QString destFile);
static void _soften(QString sourceFile, QString destFile);

static AlgorithmFunction
    g_functions[ImageProcessor::AlgorithmCount] = {
    _gray,
    _binarize,
    _negative,
    _emboss,
    _sharpen,
    _soften
};

class AlgorithmRunnable : public QRunnable
{
public:
    AlgorithmRunnable(QString sourceFile, QString destFile, ImageProcessor::ImageAlgorithm algorithm, QObject * observer)
        :m_observer(observer),
         m_sourceFilePath(sourceFile),
         m_destFilePath(destFile),
         m_algorithm(algorithm)
    {

    }
    ~AlgorithmRunnable() {

    }

    void run()
    {
        g_functions[m_algorithm](m_sourceFilePath, m_destFilePath);
        QCoreApplication::postEvent(m_observer, new ExecutedEvent(this));
    }

    QPointer<QObject> m_observer;
    QString m_sourceFilePath;
    QString m_destFilePath;
    ImageProcessor::ImageAlgorithm m_algorithm;
};

class ImageProcessorPrivate : public QObject
{
public:
    ImageProcessorPrivate(ImageProcessor * processor)
        : QObject(processor), m_processor(processor),
          m_tempPath(QDir::currentPath())
    {
        ExecutedEvent::evType();
    }
    ~ImageProcessorPrivate(){};

    bool event(QEvent * e)
    {
        if(e->type() == ExecutedEvent::evType())
        {
            ExecutedEvent *ee = (ExecutedEvent *) e;
            if(m_runnables.contains(ee->m_runnable))
            {
                m_notifiedAlgorithm = ee->m_runnable->m_algorithm;
                m_notifiedSourceFile = ee -> m_runnable-> m_sourceFilePath;
                emit m_processor->finished(ee->m_runnable->m_destFilePath);
                m_runnables.removeOne(ee->m_runnable);
            }
            delete ee->m_runnable;
            return true;
        }
        return QObject::event(e);
    }

    void process(QString sourceFile, ImageProcessor::ImageAlgorithm algorithm)
    {
        QFileInfo fi(sourceFile);
        QString destFile = QString("%2/%2_%3").arg(m_tempPath)
                .arg((int)algorithm).arg(fi.fileName());
        AlgorithmRunnable *r = new AlgorithmRunnable(sourceFile, destFile, algorithm, this);
        m_runnables.append(r);
        r->setAutoDelete(false);
        QThreadPool::globalInstance()->start(r);
    }

    ImageProcessor * m_processor;
    QList<AlgorithmRunnable * > m_runnables;
    QString m_notifiedSourceFile;
    ImageProcessor::ImageAlgorithm m_notifiedAlgorithm;
    QString m_tempPath;
};

ImageProcessor::ImageProcessor(QObject * parent) : QObject(parent), m_d(new ImageProcessorPrivate(this))
{

}

ImageProcessor::~ImageProcessor()
{
    delete m_d;
}

QString ImageProcessor::sourceFile() const
{
    return m_d -> m_notifiedSourceFile;
}

ImageProcessor::ImageAlgorithm ImageProcessor::algorithm() const
{
    return m_d -> m_notifiedAlgorithm;
}

void ImageProcessor::setTempPath(QString tempPath)
{
    m_d->m_tempPath = tempPath;
}

void ImageProcessor::process(QString file, ImageAlgorithm algorithm)
{
    m_d->process(file, algorithm);
}

void ImageProcessor::abort(QString file, ImageAlgorithm algorithm)
{
    int size = m_d -> m_runnables.size();
    AlgorithmRunnable * r;
    for(int i = 0; i < size; i++)
    {
        r = m_d->m_runnables.at(i);
        if(r->m_sourceFilePath == file && r->m_algorithm == algorithm)
        {
            m_d->m_runnables.removeAt(i);
            break;
        }
    }
}
