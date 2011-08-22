#
#  qmpwidget - A Qt widget for embedding MPlayer
#  Copyright (C) 2010 by Jonas Gehring
#

TEMPLATE = lib
DESTDIR = ..
TARGET = qmplayer

QT += network 
CONFIG += staticlib

# Optional features
# not implemented yet
#QT += opengl
#CONFIG += pipemode

include(qmplayer.pri)
