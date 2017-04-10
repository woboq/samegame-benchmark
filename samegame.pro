TEMPLATE = app

QT += qml quick sql testlib quick-private qml-private
HEADERS += \
    samegameimpl.h \
    gameareaimpl.h
SOURCES += \
    main.cpp \
    gameareaimpl.cpp \
    samegameimpl.cpp
RESOURCES += samegame.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/demos/samegame
INSTALLS += target
