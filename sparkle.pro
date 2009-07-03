# #####################################################################
# Automatically generated by qmake (2.01a) ?? ???? 2 10:53:54 2009
# #####################################################################
TEMPLATE = app
TARGET = sparkgap
DEPENDPATH += .
INCLUDEPATH += .
QMAKE_LIBS += -lssl
QT -= gui
QT += network

# Input
HEADERS += RSAKeyPair.h \
    ArgumentParser.h \
    LinkLayer.h \
    SparkleNode.h \
    BlowfishKey.h \
    PacketTransport.h \
    UdpPacketTransport.h \
    SHA1Digest.h \
    LinuxTAP.h
SOURCES += main.cpp \
    RSAKeyPair.cpp \
    ArgumentParser.cpp \
    LinkLayer.cpp \
    SparkleNode.cpp \
    BlowfishKey.cpp \
    UdpPacketTransport.cpp \
    SHA1Digest.cpp \
    LinuxTAP.cpp
