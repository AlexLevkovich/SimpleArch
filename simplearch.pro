QT       += core gui declarative network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

isEmpty(INSTALL_PREFIX) {
    INSTALL_PREFIX = /usr
}

isEmpty(TOOLS_PATH) {
    TOOLS_PATH = $$INSTALL_PREFIX/bin
}

DEFINES += INSTALL_PREFIX=\\\"$$INSTALL_PREFIX\\\"
DEFINES += TOOLS_PATH=\\\"$$TOOLS_PATH\\\"
DEFINES += HAVE_GETPT
CONFIG += link_pkgconfig
PKGCONFIG += gobject-2.0 gio-2.0

lessThan(QT_MAJOR_VERSION, 5): {
INCLUDEPATH += ./mimetypes
INCLUDEPATH += ./io
}

HEADERS     = \
              filetreeitem.h \
    filetreemodel.h \
    byteshumanizer.h \
    iconprovider.h \
    sortfilterproxymodel.h \
    mainwindow.h \
    dirtreeview.h \
    dircontentstreeview.h \
    basearchengine.h \
    tararchengine.h \
    bziparchengine.h \
    gziparchengine.h \
    xziparchengine.h \
    lziparchengine.h \
    waitview.h \
    busyindicator.h \
    lzip4archengine.h \
    complextararchengine.h \
    dragarchengine.h \
    eventloop.h \
    archiverprocess.h \
    onlydirssortfilterproxymodel.h \
    droparchengine.h \
    basetreeview.h \
    removearchengine.h \
    filesselectordialog.h \
    folderchooser.h \
    extractfolderchooser.h \
    extractarchengine.h \
    openarchivedialog.h \
    createarchivedialog.h \
    createarchengine.h \
    scrollmessagebox.h \
    saveasarchengine.h \
    extracttotemparchengine.h \
    deletetararchengine.h \
    updatetararchengine.h \
    settingsdialog.h \
    categorylistview.h \
    mimesettingspage.h \
    mimestable.h \
    archivesettingspage.h \
    ziparchengine.h \
    staticutils.h \
    kpty_p.h \
    kpty.h \
    statusbar.h \
    compressionlevelstableview.h \
    compressionlevelssettingspage.h \
    rararchengine.h \
    7zarchengine.h \
    waitdialog.h \
    flushedprocess.h \
    writeptyprocess.h \
    sequentialbuffer.h \
    process/unixprocess_p.h \
    process/unixprocess.h

RESOURCES   = \
    main.qrc
SOURCES     = \
              main.cpp \
              filetreeitem.cpp \
    filetreemodel.cpp \
    byteshumanizer.cpp \
    iconprovider.cpp \
    sortfilterproxymodel.cpp \
    mainwindow.cpp \
    dirtreeview.cpp \
    dircontentstreeview.cpp \
    basearchengine.cpp \
    tararchengine.cpp \
    bziparchengine.cpp \
    gziparchengine.cpp \
    xziparchengine.cpp \
    lziparchengine.cpp \
    waitview.cpp \
    busyindicator.cpp \
    lzip4archengine.cpp \
    complextararchengine.cpp \
    dragarchengine.cpp \
    eventloop.cpp \
    archiverprocess.cpp \
    onlydirssortfilterproxymodel.cpp \
    droparchengine.cpp \
    basetreeview.cpp \
    removearchengine.cpp \
    filesselectordialog.cpp \
    folderchooser.cpp \
    extractfolderchooser.cpp \
    extractarchengine.cpp \
    openarchivedialog.cpp \
    createarchivedialog.cpp \
    createarchengine.cpp \
    scrollmessagebox.cpp \
    saveasarchengine.cpp \
    extracttotemparchengine.cpp \
    deletetararchengine.cpp \
    updatetararchengine.cpp \
    settingsdialog.cpp \
    categorylistview.cpp \
    mimesettingspage.cpp \
    mimestable.cpp \
    archivesettingspage.cpp \
    ziparchengine.cpp \
    staticutils.cpp \
    kpty.cpp \
    statusbar.cpp \
    compressionlevelstableview.cpp \
    compressionlevelssettingspage.cpp \
    rararchengine.cpp \
    7zarchengine.cpp \
    waitdialog.cpp \
    flushedprocess.cpp \
    writeptyprocess.cpp \
    sequentialbuffer.cpp \
    process/unixprocess.cpp \
    process/unixprocess_p.cpp

lessThan(QT_MAJOR_VERSION, 5): {
        HEADERS += qtemporarydir.h
        SOURCES += qtemporarydir.cpp

        HEADERS += io/qstandardpaths.h
        SOURCES += io/qstandardpaths.cpp

        macx {
            SOURCES *= io/qstandardpaths_mac.cpp
        } else:unix {
            SOURCES *= io/qstandardpaths_unix.cpp
        }

        HEADERS += \
        mimetypes/qmimedatabase.h \
        mimetypes/qmimetype.h \
        mimetypes/qmimemagicrulematcher_p.h \
        mimetypes/qmimetype_p.h \
        mimetypes/qmimetypeparser_p.h \
        mimetypes/qmimedatabase_p.h \
        mimetypes/qmimemagicrule_p.h \
        mimetypes/qmimeglobpattern_p.h \
        mimetypes/qmimeprovider_p.h \
        mimetypes-qt4-helpers.h

        SOURCES += \
        mimetypes/qmimedatabase.cpp \
        mimetypes/qmimetype.cpp \
        mimetypes/qmimemagicrulematcher.cpp \
        mimetypes/qmimetypeparser.cpp \
        mimetypes/qmimemagicrule.cpp \
        mimetypes/qmimeglobpattern.cpp \
        mimetypes/qmimeprovider.cpp \
        mimetypes-qt4-helpers.cpp

        RESOURCES += mimetypes/mimetypes.qrc
}


# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/itemviews/simpletreemodel
INSTALLS += target 

FORMS += \
    mainwindow.ui \
    folderchooser.ui \
    settingsdialog.ui \
    mimesettingspage.ui \
    archivesettingspage.ui \
    compressionlevelssettingspage.ui

DISTFILES +=
