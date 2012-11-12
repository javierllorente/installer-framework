/**************************************************************************
**
** This file is part of Installer Framework
**
** Copyright (c) 2011-2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qtpatchoperation.h"
#include "qtpatch.h"
#ifdef Q_OS_MAC
#include "macrelocateqt.h"
#include "constants.h"
#endif

#include "packagemanagercore.h"

#include <QSet>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDirIterator>
#include <QtCore/QDebug>

using namespace QInstaller;

static QHash<QByteArray, QByteArray> generatePatchValueHash(const QByteArray &newQtPath,
        const QHash<QString, QByteArray> &qmakeValueHash)
{
    QHash<QByteArray, QByteArray> replaceHash; //first == searchstring: second == replace string
    char nativeSeperator = QDir::separator().toAscii();
    QByteArray oldValue;

    oldValue = qmakeValueHash.value(QLatin1String("QT_INSTALL_PREFIX"));
    replaceHash.insert(QByteArray("qt_prfxpath=%1").replace("%1", oldValue),
        QByteArray("qt_prfxpath=%1/").replace("%1/", newQtPath));

    oldValue = qmakeValueHash.value(QLatin1String("QT_INSTALL_DOCS"));
    replaceHash.insert(QByteArray("qt_docspath=%1").replace("%1", oldValue),
        QByteArray("qt_docspath=%1/doc").replace("%1/", newQtPath + nativeSeperator));

    oldValue = qmakeValueHash.value(QLatin1String("QT_INSTALL_HEADERS"));
    replaceHash.insert(QByteArray("qt_hdrspath=%1").replace("%1", oldValue),
        QByteArray("qt_hdrspath=%1/include").replace("%1/", newQtPath + nativeSeperator));

    oldValue = qmakeValueHash.value(QLatin1String("QT_INSTALL_LIBS"));
    replaceHash.insert(QByteArray("qt_libspath=%1").replace("%1", oldValue),
        QByteArray("qt_libspath=%1/lib").replace("%1/", newQtPath + nativeSeperator));

    oldValue = qmakeValueHash.value(QLatin1String("QT_INSTALL_BINS"));
    replaceHash.insert(QByteArray("qt_binspath=%1").replace("%1", oldValue),
        QByteArray("qt_binspath=%1/bin").replace("%1/", newQtPath + nativeSeperator));

    oldValue = qmakeValueHash.value(QLatin1String("QT_INSTALL_PLUGINS"));
    replaceHash.insert(QByteArray("qt_plugpath=%1").replace("%1", oldValue),
        QByteArray("qt_plugpath=%1/plugins").replace("%1/", newQtPath + nativeSeperator));

    oldValue = qmakeValueHash.value(QLatin1String("QT_INSTALL_IMPORTS"));
    replaceHash.insert(QByteArray("qt_impspath=%1").replace("%1", oldValue),
        QByteArray("qt_impspath=%1/imports").replace("%1/", newQtPath + nativeSeperator));

    oldValue = qmakeValueHash.value(QLatin1String("QT_INSTALL_DATA"));
    replaceHash.insert( QByteArray("qt_datapath=%1").replace("%1", oldValue),
        QByteArray("qt_datapath=%1/").replace("%1/", newQtPath + nativeSeperator));

    oldValue = qmakeValueHash.value(QLatin1String("QT_INSTALL_TRANSLATIONS"));
    replaceHash.insert( QByteArray("qt_trnspath=%1").replace("%1", oldValue),
        QByteArray("qt_trnspath=%1/translations").replace("%1/", newQtPath + nativeSeperator));

    // This must not be patched. Commenting out to fix QTSDK-429
    //        oldValue = qmakeValueHash.value(QLatin1String("QT_INSTALL_CONFIGURATION"));
    //        replaceMap.insert( QByteArray("qt_stngpath=%1").replace("%1", oldValue),
    //                            QByteArray("qt_stngpath=%1").replace("%1", newQtPath));

    //examples and demoes can patched outside separately,
    //but for cosmetic reasons - if the qt version gets no examples later.
    oldValue = qmakeValueHash.value(QLatin1String("QT_INSTALL_EXAMPLES"));
    replaceHash.insert( QByteArray("qt_xmplpath=%1").replace("%1", oldValue),
        QByteArray("qt_xmplpath=%1/examples").replace("%1/", newQtPath + nativeSeperator));

    oldValue = qmakeValueHash.value(QLatin1String("QT_INSTALL_DEMOS"));
    replaceHash.insert( QByteArray("qt_demopath=%1").replace("%1", oldValue),
        QByteArray("qt_demopath=%1/demos").replace("%1/", newQtPath + nativeSeperator));

    oldValue = qmakeValueHash.value(QLatin1String("QT_INSTALL_TESTS"));
    replaceHash.insert(QByteArray("qt_tstspath=%1").replace("%1", oldValue),
        QByteArray("qt_tstspath=%1/tests").replace("%1/", newQtPath + nativeSeperator));

    oldValue = qmakeValueHash.value(QLatin1String("QT_HOST_PREFIX"));
    replaceHash.insert(QByteArray("qt_hpfxpath=%1").replace("%1", oldValue),
        QByteArray("qt_hpfxpath=%1/").replace("%1/", newQtPath));

    oldValue = qmakeValueHash.value(QLatin1String("QT_HOST_BINS"));
    replaceHash.insert( QByteArray("qt_hbinpath=%1").replace("%1", oldValue),
        QByteArray("qt_hbinpath=%1/bin").replace("%1/", newQtPath + nativeSeperator));

    oldValue = qmakeValueHash.value(QLatin1String("QT_HOST_DATA"));
    replaceHash.insert(QByteArray("qt_hdatpath=%1").replace("%1", oldValue),
        QByteArray("qt_hdatpath=%1/").replace("%1/", newQtPath));

    return replaceHash;
}

QtPatchOperation::QtPatchOperation()
{
    setName(QLatin1String("QtPatch"));
}

void QtPatchOperation::backup()
{
}

bool QtPatchOperation::performOperation()
{
    // Arguments:
    // 1. type
    // 2. new/target qtpath

    if (arguments().count() != 2) {
        setError(InvalidArguments);
        setErrorString(tr("Invalid arguments in %0: %1 arguments given, 2 expected.").arg(name())
            .arg(arguments().count()));
        return false;
    }

    QString type = arguments().at(0);
    bool isPlatformSupported = type.contains(QLatin1String("linux"), Qt::CaseInsensitive)
        || type.contains(QLatin1String("windows"), Qt::CaseInsensitive)
        || type.contains(QLatin1String("mac"), Qt::CaseInsensitive);
    if (!isPlatformSupported) {
        setError(InvalidArguments);
        setErrorString(tr("First argument should be 'linux', 'mac' or 'windows'. No other type is supported "
            "at this time."));
        return false;
    }
    
    const QString newQtPathStr = QDir::toNativeSeparators(arguments().at(1));
    const QByteArray newQtPath = newQtPathStr.toUtf8();

    QString qmakePath = QString::fromUtf8(newQtPath) + QLatin1String("/bin/qmake");
#ifdef Q_OS_WIN
    qmakePath = qmakePath + QLatin1String(".exe");
#endif

    if (!QFile::exists(qmakePath)) {
        setError(UserDefinedError);
        setErrorString(tr("QMake from the current Qt version \n(%1)is not existing. Please file a bugreport "
            "with this dialog at https://bugreports.qt-project.org.").arg(QDir::toNativeSeparators(qmakePath)));
        return false;
    }

    QByteArray qmakeOutput;
    QHash<QString, QByteArray> qmakeValueHash = QtPatch::qmakeValues(qmakePath, &qmakeOutput);

    if (qmakeValueHash.isEmpty()) {
        setError(UserDefinedError);
        setErrorString(tr("The output of \n%1 -query\nis not parseable. Please file a bugreport with this "
            "dialog https://bugreports.qt-project.org.\noutput: \"%2\"").arg(QDir::toNativeSeparators(qmakePath),
            QString::fromUtf8(qmakeOutput)));
        return false;
    }

    const QByteArray oldQtPath = qmakeValueHash.value(QLatin1String("QT_INSTALL_PREFIX"));
    bool oldQtPathFromQMakeIsEmpty = oldQtPath.isEmpty();

    //maybe we don't need this, but I 255 should be a rational limit
    if (255 < newQtPath.size()) {
        setError(UserDefinedError);
        setErrorString(tr("Qt patch error: new Qt dir(%1)\nneeds to be less than 255 characters.")
            .arg(newQtPathStr));
        return false;
    }

    QFile patchFileListFile;
    if (type == QLatin1String("windows"))
        patchFileListFile.setFileName(QLatin1String(":/files-to-patch-windows"));
    else if (type == QLatin1String("linux"))
        patchFileListFile.setFileName(QLatin1String(":/files-to-patch-linux"));
    else if (type == QLatin1String("linux-emb-arm"))
        patchFileListFile.setFileName(QLatin1String(":/files-to-patch-linux-emb-arm"));
    else if (type == QLatin1String("mac"))
        patchFileListFile.setFileName(QLatin1String(":/files-to-patch-macx"));

    if (!patchFileListFile.open(QFile::ReadOnly)) {
        setError(UserDefinedError);
        setErrorString(tr("Qt patch error: Can not open %1.(%2)").arg(patchFileListFile.fileName(),
            patchFileListFile.errorString()));
        return false;
    }

    QStringList filters;
    QStringList filesToPatch, textFilesToPatch;
    bool readingTextFilesToPatch = false;

    // read the input file
    QTextStream in(&patchFileListFile);

    forever {
        const QString line = in.readLine();

        if (line.isNull())
            break;

        else if (line.isEmpty())
            continue;

        else if (line.startsWith(QLatin1String("%%")))
            readingTextFilesToPatch = true;

        //with empty old path we don't know what we want to replace
        else if (readingTextFilesToPatch && !oldQtPathFromQMakeIsEmpty) {
            // check if file mask filter
            if (line.startsWith(QLatin1String("*."), Qt::CaseInsensitive)) {
                filters << line;
            }
            textFilesToPatch.append(line);
        }
        else
            filesToPatch.append(line);
    }

    QString prefix = QFile::decodeName(newQtPath);

    if (! prefix.endsWith(QLatin1Char('/')))
        prefix += QLatin1Char('/');

//BEGIN - patch binary files
    QHash<QByteArray, QByteArray> patchValueHash = generatePatchValueHash(newQtPath, qmakeValueHash);

    foreach (QString fileName, filesToPatch) {
        fileName.prepend(prefix);
        QFile file(fileName);

        //without a file we can't do anything
        if (!file.exists()) {
            continue;
        }

        if (!QtPatch::openFileForPatching(&file)) {
            setError(UserDefinedError);
            setErrorString(tr("Qt patch error: Can not open %1.(%2)").arg(file.fileName())
                .arg(file.errorString()));
            return false;
        }

        QHashIterator<QByteArray, QByteArray> it(patchValueHash);
        while (it.hasNext()) {
            it.next();
            bool isPatched = QtPatch::patchBinaryFile(&file, it.key(), it.value());
            if (!isPatched) {
                qDebug() << QString::fromLatin1("qpatch: warning: file '%1' could not patched").arg(fileName);
            }
        }
    } //foreach (QString fileName, filesToPatch)
//END - patch binary files

//BEGIN - patch text files
    QByteArray newQtPathWithNormalSlashes = QDir::fromNativeSeparators(newQtPathStr).toUtf8();

    QHash<QByteArray, QByteArray> searchReplacePairs;
    searchReplacePairs.insert(oldQtPath, newQtPathWithNormalSlashes);
    searchReplacePairs.insert(QByteArray(oldQtPath).replace("/", "\\"), newQtPathWithNormalSlashes);
    searchReplacePairs.insert(QByteArray(oldQtPath).replace("\\", "/"), newQtPathWithNormalSlashes);

#ifdef Q_OS_WIN
    QByteArray newQtPathWithDoubleBackSlashes = QByteArray(newQtPathWithNormalSlashes).replace("/", "\\\\");
    searchReplacePairs.insert(QByteArray(oldQtPath).replace("/", "\\\\"), newQtPathWithDoubleBackSlashes);
    searchReplacePairs.insert(QByteArray(oldQtPath).replace("\\", "\\\\"), newQtPathWithDoubleBackSlashes);

    //this is checking for a possible drive letter, which could be upper or lower
    if (oldQtPath.mid(1,1) == ":") {
        QHash<QByteArray, QByteArray> tempSearchReplacePairs;
        QHashIterator<QByteArray, QByteArray> it(searchReplacePairs);
        QByteArray driveLetter = oldQtPath.left(1);
        while (it.hasNext()) {
            it.next();
            QByteArray currentPossibleSearchByteArrayWithoutDriveLetter = QByteArray(it.key()).remove(0, 1);
            tempSearchReplacePairs.insert(driveLetter.toLower()
                + currentPossibleSearchByteArrayWithoutDriveLetter, it.value());
            tempSearchReplacePairs.insert(driveLetter.toUpper()
                + currentPossibleSearchByteArrayWithoutDriveLetter, it.value());
        }
        searchReplacePairs = tempSearchReplacePairs;
    }
#endif

    // get file list defined by filters and patch them
    if (filters.count() > 0) {
        const QStringList filteredContent = getDirContent(prefix, filters);
        foreach (const QString &fileName, filteredContent) {
            if (QFile::exists(fileName)) {
                QtPatch::patchTextFile(fileName, searchReplacePairs);
            }
        }
    }

    // patch single items
    foreach (QString fileName, textFilesToPatch) {
        fileName.prepend(prefix);

        if (QFile::exists(fileName)) {
            //TODO: use the return value for an error message at the end of the operation
            QtPatch::patchTextFile(fileName, searchReplacePairs);
        }
    }
//END - patch text files

#ifdef Q_OS_MAC
    Relocator relocator;
    bool successMacRelocating = false;
    PackageManagerCore *const core = qVariantValue<PackageManagerCore*>(value(QLatin1String("installer")));
    if (!core) {
        setError(UserDefinedError);
        setErrorString(tr("Needed installer object in \"%1\" operation is empty.").arg(name()));
        return false;
    }
    Q_CHECK_PTR(core);
    successMacRelocating = relocator.apply(newQtPathStr, core->value(scTargetDir));
    if (!successMacRelocating) {
        setError(UserDefinedError);
        setErrorString(tr("Error while relocating Qt: %1").arg(relocator.errorMessage()));
        return false;
    }
#endif
    if (oldQtPathFromQMakeIsEmpty) {
        setError(UserDefinedError);
        setErrorString(tr("The installer was not able to get the unpatched path from \n%1.(maybe it is "
            "broken or removed)\nIt tried to patch the Qt binaries, but all other files in Qt are unpatched."
            "\nThis could result in a broken Qt version.\nSometimes it helps to restart the installer with a "
            "switched off antivirus software.").arg(QDir::toNativeSeparators(qmakePath)));
        return false;
    }

    return true;
}

QStringList QtPatchOperation::getDirContent(const QString& aPath, QStringList aFilters)
{
    QStringList list;
    QDirIterator dirIterator(aPath, aFilters, QDir::AllDirs|QDir::Files|QDir::NoSymLinks,
                             QDirIterator::Subdirectories);
    while (dirIterator.hasNext()) {
        dirIterator.next();
        if (!dirIterator.fileInfo().isDir()) {
            list.append(dirIterator.fileInfo().absoluteFilePath());
            qDebug() << QString::fromLatin1("QtPatchOperation::getDirContent match: '%1'").arg(dirIterator.fileInfo().absoluteFilePath());
        }
    }

    return list;
}

bool QtPatchOperation::undoOperation()
{
    return true;
}

bool QtPatchOperation::testOperation()
{
    return true;
}

Operation *QtPatchOperation::clone() const
{
    return new QtPatchOperation();
}
