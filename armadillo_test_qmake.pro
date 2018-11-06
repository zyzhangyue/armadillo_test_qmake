TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp \
    module_predosimpledoppler.cpp \
    module_csisharedmemory.cpp \
    module_csirawdatafetcher.cpp

QMAKE_LFLAGS += -pthread

LIBS += /usr/lib/x86_64-linux-gnu/libarmadillo.so

HEADERS += \
    module_predosimpledoppler.h \
    csi_packet.h \
    iwl_connector.h \
    connector_users.h \
    module_csisharedmemory.h \
    module_csirawdatafetcher.h
