TEMPLATE = app
TARGET = sippy
DESTDIR = ../output

DEPENDPATH += . ../libsparkle/headers
INCLUDEPATH += ../libsparkle/headers
	
QT += network

debug:DEFINES += DEBUG

win32:LIBS += -ladvapi32
	
LIBS += -lsparkle -L../output

# Input
HEADERS += ConfigurationStorage.h \
    ConnectDialog.h \
    Singleton.h \
    MessagingApplicationLayer.h \
    ContactWidget.h \
    Roster.h \
    AddContactDialog.h \
    Contact.h \
    ContactList.h \
    EditContactDialog.h \
    StatusBox.h \
    pixmaps.h \
    PreferencesDialog.h \
    ChatWindow.h \
    ChatMessageEdit.h \
    Messaging.h
	
SOURCES += main.cpp \
    ConfigurationStorage.cpp \
    ConnectDialog.cpp \
    MessagingApplicationLayer.cpp \
    ContactWidget.cpp \
    Roster.cpp \
    AddContactDialog.cpp \
    Contact.cpp \
    ContactList.cpp \
    EditContactDialog.cpp \
    StatusBox.cpp \
    PreferencesDialog.cpp \
    ChatWindow.cpp \
    ChatMessageEdit.cpp \
    Messaging.cpp
	
FORMS += Roster.ui \
    ConnectDialog.ui \
    AddContactDialog.ui \
    EditContactDialog.ui \
    PreferencesDialog.ui

unix:QMAKE_LFLAGS += -Wl,-rpath ${PWD}/../output

equals(QT_MAJOR_VERSION, "4"):lessThan(QT_MINOR_VERSION, "6") { 
    warning("Using bundled QtMultimedia from Qt 4.6.1")
    include("multimedia/audio.pri")
    INCLUDEPATH += multimedia/include \
        multimedia/include/QtMultimedia
    DEFINES += QT_BUILD_MULTIMEDIA_LIB
}
