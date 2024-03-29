#include "filefilteritems.h"
#include <qdebug.h>
#include <QtGui/QImageReader>

namespace QmlProjectManager {

FileFilterBaseItem::FileFilterBaseItem(QObject *parent) :
        QmlProjectContentItem(parent),
        m_recurse(RecurseDefault)
{
    connect(&m_dirWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(updateFileList()));
}

QString FileFilterBaseItem::directory() const
{
    return m_rootDir;
}

void FileFilterBaseItem::setDirectory(const QString &dirPath)
{
    if (m_rootDir == dirPath)
        return;
    m_rootDir = dirPath;
    emit directoryChanged();

    updateFileList();
}

void FileFilterBaseItem::setDefaultDirectory(const QString &dirPath)
{
    if (m_defaultDir == dirPath)
        return;
    m_defaultDir = dirPath;

    updateFileList();
}

QString FileFilterBaseItem::filter() const
{
    return m_filter;
}

void FileFilterBaseItem::setFilter(const QString &filter)
{
    if (filter == m_filter)
        return;
    m_filter = filter;

    m_regExpList.clear();
    foreach (const QString &pattern, filter.split(QLatin1Char(';'))) {
        m_regExpList << QRegExp(pattern, Qt::CaseInsensitive, QRegExp::Wildcard);
    }

    emit filterChanged();
    updateFileList();
}

bool FileFilterBaseItem::recursive() const
{
    bool recursive;
    if (m_recurse == Recurse) {
        recursive = true;
    } else if (m_recurse == DoNotRecurse) {
        recursive = false;
    } else {  // RecurseDefault
        if (m_explicitFiles.isEmpty()) {
            recursive = true;
        } else {
            recursive = false;
        }
    }
    return recursive;
}

void FileFilterBaseItem::setRecursive(bool recurse)
{
    bool oldRecursive = recursive();

    if (recurse) {
        m_recurse = Recurse;
    } else {
            m_recurse = DoNotRecurse;
    }

    if (recurse != oldRecursive)
        updateFileList();
}

QStringList FileFilterBaseItem::pathsProperty() const
{
    return m_explicitFiles;
}

void FileFilterBaseItem::setPathsProperty(const QStringList &path)
{
    m_explicitFiles = path;
    updateFileList();
}

QStringList FileFilterBaseItem::files() const
{
    return m_files.toList();
}

/**
  Check whether filter matches a file path - regardless whether the file already exists or not.

  @param filePath: absolute file path
  */
bool FileFilterBaseItem::matchesFile(const QString &filePath) const
{
    foreach (const QString &explicitFile, m_explicitFiles) {
        if (absolutePath(explicitFile) == filePath)
            return true;
    }

    bool regexMatches = false;
    const QString &fileName = QFileInfo(filePath).fileName();
    foreach (const QRegExp &exp, m_regExpList) {
        if (exp.exactMatch(fileName)) {
            regexMatches = true;
            break;
        }
    }

    if (!regexMatches)
        return false;

    const QDir fileDir = QFileInfo(filePath).absoluteDir();
    foreach (const QString &watchedDirectory, m_dirWatcher.directories()) {
        if (QDir(watchedDirectory) == fileDir)
            return true;
    }

    return false;
}

QString FileFilterBaseItem::absolutePath(const QString &path) const
{
    if (QFileInfo(path).isAbsolute())
        return path;
    return QDir(absoluteDir()).absoluteFilePath(path);
}

QString FileFilterBaseItem::absoluteDir() const
{
    QString absoluteDir;
    if (QFileInfo(m_rootDir).isAbsolute()) {
        absoluteDir = m_rootDir;
    } else if (!m_defaultDir.isEmpty()) {
        absoluteDir = m_defaultDir + QLatin1Char('/') + m_rootDir;
    }

    return absoluteDir;
}

void FileFilterBaseItem::updateFileList()
{
    const QString projectDir = absoluteDir();
    if (projectDir.isEmpty())
        return;

    QSet<QString> dirsToBeWatched;
    QSet<QString> newFiles;
    foreach (const QString &explicitPath, m_explicitFiles) {
        newFiles << absolutePath(explicitPath);
    }
    if (!m_regExpList.isEmpty() && m_explicitFiles.isEmpty())
        newFiles += filesInSubTree(QDir(m_defaultDir), QDir(projectDir), &dirsToBeWatched);

    if (newFiles != m_files) {
        QSet<QString> addedFiles = newFiles;
        QSet<QString> removedFiles = m_files;
        QSet<QString> unchanged = newFiles;
        unchanged.intersect(m_files);
        addedFiles.subtract(unchanged);
        removedFiles.subtract(unchanged);

        m_files = newFiles;
        emit filesChanged(addedFiles, removedFiles);
    }

    // update watched directories
    const QSet<QString> oldDirs = m_dirWatcher.directories().toSet();
    const QSet<QString> unwatchDirs = oldDirs - dirsToBeWatched;
    const QSet<QString> watchDirs = dirsToBeWatched - oldDirs;

    if (!unwatchDirs.isEmpty())
        m_dirWatcher.removeDirectories(unwatchDirs.toList());
    if (!watchDirs.isEmpty())
        m_dirWatcher.addDirectories(watchDirs.toList());
}

QSet<QString> FileFilterBaseItem::filesInSubTree(const QDir &rootDir, const QDir &dir, QSet<QString> *parsedDirs)
{
    QSet<QString> fileSet;

    if (parsedDirs)
        parsedDirs->insert(dir.absolutePath());

    foreach (const QFileInfo &file, dir.entryInfoList(QDir::Files)) {
        const QString fileName = file.fileName();
        foreach (const QRegExp &filter, m_regExpList) {
            if (filter.exactMatch(fileName)) {
                fileSet.insert(file.absoluteFilePath());
                break;
            }
        }
    }

    if (recursive()) {
        foreach (const QFileInfo &subDir, dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            fileSet += filesInSubTree(rootDir, QDir(subDir.absoluteFilePath()), parsedDirs);
        }
    }
    return fileSet;
}

QmlFileFilterItem::QmlFileFilterItem(QObject *parent)
    : FileFilterBaseItem(parent)
{
    setFilter(QLatin1String("*.qml"));
}


JsFileFilterItem::JsFileFilterItem(QObject *parent)
    : FileFilterBaseItem(parent)
{
    setFilter(QLatin1String("*.js"));
}

ImageFileFilterItem::ImageFileFilterItem(QObject *parent)
    : FileFilterBaseItem(parent)
{
    QString filter;
    // supported image formats according to
    QList<QByteArray> extensions = QImageReader::supportedImageFormats();
    foreach (const QByteArray &extension, extensions) {
        filter.append(QString("*.%1;").arg(QString::fromAscii(extension)));
    }
    setFilter(filter);
}

CssFileFilterItem::CssFileFilterItem(QObject *parent)
    : FileFilterBaseItem(parent)
{
    setFilter(QLatin1String("*.css"));
}

} // namespace QmlProjectManager

