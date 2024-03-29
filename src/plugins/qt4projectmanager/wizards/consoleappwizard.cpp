/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "consoleappwizard.h"

#include "consoleappwizarddialog.h"
#include "qt4projectmanagerconstants.h"

#include <cpptools/cppmodelmanagerinterface.h>

#include <QtGui/QIcon>

#include <QtCore/QTextStream>

static const char *mainCppC =
"#include <QtCore/QCoreApplication>\n\n"
"int main(int argc, char *argv[])\n"
"{\n"
"    QCoreApplication a(argc, argv);\n\n"
"    return a.exec();\n"
"}\n";

static const char *mainSourceFileC = "main";

namespace Qt4ProjectManager {
namespace Internal {

ConsoleAppWizard::ConsoleAppWizard()
  : QtWizard(QLatin1String("E.Qt4Core"),
             QLatin1String(Constants::QT_APP_WIZARD_CATEGORY),
             QLatin1String(Constants::QT_APP_WIZARD_TR_SCOPE),
             QLatin1String(Constants::QT_APP_WIZARD_TR_CATEGORY),
             tr("Qt Console Application"),
             tr("Creates a project containing a single main.cpp file with a stub implementation.\n\n"
                "Preselects a desktop Qt for building the application if available."),
             QIcon(QLatin1String(":/wizards/images/console.png")))
{
}

QWizard *ConsoleAppWizard::createWizardDialog(QWidget *parent,
                                              const QString &defaultPath,
                                              const WizardPageList &extensionPages) const
{
    ConsoleAppWizardDialog *dialog = new ConsoleAppWizardDialog(displayName(), icon(), extensionPages,
                                                                showModulesPageForApplications(), parent);
    dialog->setPath(defaultPath);
    dialog->setProjectName(ConsoleAppWizardDialog::uniqueProjectName(defaultPath));
    return dialog;
}

Core::GeneratedFiles
        ConsoleAppWizard::generateFiles(const QWizard *w,
                                        QString * /*errorMessage*/) const
{
    const ConsoleAppWizardDialog *wizard = qobject_cast< const ConsoleAppWizardDialog *>(w);
    const QtProjectParameters params = wizard->parameters();
    const QString projectPath = params.projectPath();
    const QString license = CppTools::AbstractEditorSupport::licenseTemplate();

    // Create files: Source

    const QString sourceFileName = Core::BaseFileWizard::buildFileName(projectPath, QLatin1String(mainSourceFileC), sourceSuffix());
    Core::GeneratedFile source(sourceFileName);
    source.setContents(license + QLatin1String(mainCppC));
    source.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    // Create files: Profile
    const QString profileName = Core::BaseFileWizard::buildFileName(projectPath, params.fileName, profileSuffix());

    Core::GeneratedFile profile(profileName);
    profile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);
    QString contents;
    {
        QTextStream proStr(&contents);
        QtProjectParameters::writeProFileHeader(proStr);
        params.writeProFile(proStr);
        proStr << "\n\nSOURCES += " << QFileInfo(sourceFileName).fileName() << '\n';
    }
    profile.setContents(contents);
    return Core::GeneratedFiles() <<  source << profile;
}

} // namespace Internal
} // namespace Qt4ProjectManager
