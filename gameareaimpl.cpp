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
#include "gameareaimpl.h"
#include "samegameimpl.h"

#include <QQmlContext>

void GameAreaImpl::componentComplete()
{
    QQmlContext *context = qmlContext(this);
    // Fetch the root through the parent context in a QuickImplPointer to
    // be able to access its C++ impl.
    m_sameGame.reset(context->contextProperty("root").value<QQuickItem*>());
    // Fetch context items by id and hold a reference on the C++ side.
    m_puzzleWin = context->contextProperty("puzzleWin").value<QQuickItem*>();
    m_puzzleFail = context->contextProperty("puzzleFail").value<QQuickItem*>();
    m_puzzleTextBubble = context->contextProperty("puzzleTextBubble").value<QQuickItem*>();
    m_puzzleTextLabel = context->contextProperty("puzzleTextLabel").value<QQuickItem*>();
    m_p1Text = context->contextProperty("p1Text").value<QQuickItem*>();
    m_p2Text = context->contextProperty("p2Text").value<QQuickItem*>();
    m_p1WonImg = context->contextProperty("p1WonImg").value<QQuickItem*>();
    m_p2WonImg = context->contextProperty("p2WonImg").value<QQuickItem*>();
    m_clickDelay = context->contextProperty("clickDelay").value<QObject*>();
}

void GameAreaImpl::showPuzzleEnd(bool won)
{
    if (won) {
        QMetaObject::invokeMethod(m_puzzleWin, "play");
    } else {
        QMetaObject::invokeMethod(m_puzzleFail, "play");
        QMetaObject::invokeMethod(componentRoot(), "puzzleLost");
    }
}
void GameAreaImpl::showPuzzleGoal(const QString &str)
{
    m_puzzleTextBubble->setProperty("opacity", 1);
    m_puzzleTextLabel->setProperty("text", str);
}

void GameAreaImpl::swapPlayers()
{
    m_sameGame.impl()->turnChange();
    if (m_curTurn == 1) {
        QMetaObject::invokeMethod(m_p1Text, "play");
    } else {
        QMetaObject::invokeMethod(m_p2Text, "play");
    }
    m_clickDelay->setProperty("running", true);
}

void GameAreaImpl::onMouseClicked(QObject *mouse)
{
    if (m_puzzleTextBubble->opacity() == 1) {
        m_puzzleTextBubble->setProperty("opacity", 0);
        m_sameGame.impl()->finishLoadingMap();
    } else if (!m_swapping) {
        m_sameGame.impl()->handleClick(mouse->property("x").toInt(), mouse->property("y").toInt());
    }
}

void GameAreaImpl::showWonImg()
{
    if (componentRoot()->property("mode") == "multiplayer") {
        if (componentRoot()->property("score") >= componentRoot()->property("score2")) {
            m_p1WonImg->setProperty("opacity", 1);
        } else {
            m_p2WonImg->setProperty("opacity", 1);
        }
    }
}

void GameAreaImpl::hideWonImg()
{
    m_p1WonImg->setProperty("opacity", 0);
    m_p2WonImg->setProperty("opacity", 0);
}

void GameAreaImpl::hidePuzzleTextBubble()
{
    if (componentRoot()->property("mode") != "puzzle" && m_puzzleTextBubble->opacity() > 0)
        m_puzzleTextBubble->setProperty("opacity", 0);
}
