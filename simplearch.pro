QT       += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

isEmpty(INSTALL_PREFIX) {
    INSTALL_PREFIX = /usr
}

isEmpty(TOOLS_PATH) {
    TOOLS_PATH = $$INSTALL_PREFIX/bin
}

TRANS_DIR1 = $$OUT_PWD/translations
TRANS_DIR2 = $$INSTALL_PREFIX/share/youtubeviewer

DEFINES += TRANS_DIR1=\\\"$$TRANS_DIR1\\\"
DEFINES += TRANS_DIR2=\\\"$$TRANS_DIR2\\\"
DEFINES += INSTALL_PREFIX=\\\"$$INSTALL_PREFIX\\\"
DEFINES += TOOLS_PATH=\\\"$$TOOLS_PATH\\\"
DEFINES += HAVE_GETPT
CONFIG += link_pkgconfig
PKGCONFIG += gobject-2.0 gio-2.0

INCLUDEPATH += .

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
    process/unixprocess.h \
    updatearchengine.h

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
    process/unixprocess_p.cpp \
    updatearchengine.cpp

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

TRANSLATIONS = $$PWD/translations/simplearch_ru.ts \
               $$PWD/translations/simplearch_be.ts

LUPDATE = $$[QT_INSTALL_BINS]/lupdate -locations relative -no-sort
LRELEASE = $$[QT_INSTALL_BINS]/lrelease

updatets.files = TRANSLATIONS
updatets.commands = $$LUPDATE $$PWD/simplearch.pro

QMAKE_EXTRA_TARGETS += updatets

updateqm.depends = updatets
updateqm.input = TRANSLATIONS
updateqm.output = translations/${QMAKE_FILE_BASE}.qm
updateqm.commands = $$LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
updateqm.name = LRELEASE ${QMAKE_FILE_IN}
updateqm.variable_out = PRE_TARGETDEPS
updateqm.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += updateqm

qm.files = $$TRANS_DIR1/*.qm
qm.path = $$INSTALL_ROOT/$$INSTALL_PREFIX/share/simplearch/
qm.CONFIG += no_check_exist

# install
target.path = $$INSTALL_ROOT/$$INSTALL_PREFIX/bin/
INSTALLS += target qm

FORMS += \
    mainwindow.ui \
    folderchooser.ui \
    settingsdialog.ui \
    mimesettingspage.ui \
    archivesettingspage.ui \
    compressionlevelssettingspage.ui


