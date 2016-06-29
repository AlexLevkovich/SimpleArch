/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QApplication>
#include <QFileInfo>
#include "mainwindow.h"
#include "basearchengine.h"
#include "staticutils.h"
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>

const char * simplearch_version = "1.0";

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("AlexL");
    QCoreApplication::setApplicationName("SimpleArch");
    BaseArchEngine::initDefaults();


    QTranslator m_translator;
    QString lang = QLocale::system().name().split("_").at(0);
    if(!m_translator.load("simplearch_" + lang, TRANS_DIR2))
        m_translator.load("simplearch_" + lang, TRANS_DIR1);
    QApplication::installTranslator(&m_translator);

    QTranslator m_translator2;
    if (m_translator2.load(QLocale::system(), "qt", "_", QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        QApplication::installTranslator(&m_translator2);
    }

    bool ret = 0;
    QString input_file_name;
    if (argc <= 2) {
        if (argc == 2) input_file_name = StaticUtils::urlOrLocalPath(argv[1]);
        MainWindow mainWnd(input_file_name);
        mainWnd.show();
        ret = app.exec();
    }
    else {
        if (!strcmp(argv[1],"--create")) {
            QStringList items;
            QString item;
            for (int i=2;i<argc;i++) {
                item = StaticUtils::urlOrLocalPath(argv[i]);
                if (QFileInfo(item).exists()) items << item;
            }
            if (!StaticUtils::createArchive(items)) return 1;
        }
        else if (!strcmp(argv[1],"--extract")) {
            QString dir;
            if (argc > 3) dir = StaticUtils::urlOrLocalPath(argv[3]);
            if (!StaticUtils::extractArchive(StaticUtils::urlOrLocalPath(argv[2]),dir)) return 1;
        }
        else if (!strcmp(argv[1],"--add")) {
            QString archive_name;
            if (argc > 3) {
                archive_name = StaticUtils::urlOrLocalPath(argv[2]);
                if (!QFileInfo(archive_name).exists()) return 1;
            }
            else return 1;

            QStringList items;
            QString item;
            for (int i=3;i<argc;i++) {
                item = StaticUtils::urlOrLocalPath(argv[i]);
                if (QFileInfo(item).exists()) items << item;
            }
            if (!StaticUtils::addToArchive(archive_name,items)) return 1;
        }
    }

    BaseArchEngine::saveDefaults();
    return ret;
} 
