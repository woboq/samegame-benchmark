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
#ifndef GAMEAREAIMPL_H
#define GAMEAREAIMPL_H

#include <QQuickItem>

class SameGameImpl;

// One of QML's limitations that is making it difficult to integrate
// the two languages at the moment is the fact that, given:
//     main.qml: MyItem { id: myItem }
//     MyItem.qml: CppItem { property int myProp; Rectangle { id: myRect } }
// doing qmlContext(myItem) from C++ will return the QQmlContext of main.qml,
// which won't allow us to access properties and IDs defined in MyItem.qml if
// we want to place the C++ implementation as the root item.
// Having the C++ implementation as the root of components would make passing
// whole components between QML and C++ implementations much simpler and would
// allow us to define more properties in C++.
// For now we'll place our C++ implementation as a child item of MyItem.qml
// to be able to get the right QQmlContext, and we'll rely on the fact that
// our C++ implementation is intantiated as an "impl" property of its component's root.
template <typename Impl>
class QuickImplPointer
{
    QQuickItem *m_root;
    Impl *m_impl;
public:
    QuickImplPointer() : m_root(nullptr), m_impl(nullptr) { }
    QuickImplPointer(QQuickItem *root)
        : m_root(root)
        , m_impl(root->property("impl").value<Impl*>())
    { }

    void reset(QQuickItem *root) { *this = QuickImplPointer<Impl>(root); }
    QQuickItem &operator*() const { return *m_root; }
    QQuickItem *operator->() const { return m_root; }
    operator QQuickItem*() { return m_root; }

    Impl *impl() { return m_impl; }
};

class GameAreaImpl : public QObject, public QQmlParserStatus {
    Q_OBJECT

    Q_PROPERTY(int curTurn READ curTurn WRITE setCurTurn NOTIFY curTurnChanged FINAL)
    Q_PROPERTY(bool swapping MEMBER m_swapping NOTIFY swappingChanged FINAL)

    QuickImplPointer<SameGameImpl> m_sameGame;
    QQuickItem *m_puzzleWin = nullptr;
    QQuickItem *m_puzzleFail = nullptr;
    QQuickItem *m_puzzleTextBubble = nullptr;
    QQuickItem *m_puzzleTextLabel = nullptr;
    QQuickItem *m_p1Text = nullptr;
    QQuickItem *m_p2Text = nullptr;
    QQuickItem *m_p1WonImg = nullptr;
    QQuickItem *m_p2WonImg = nullptr;
    QObject *m_clickDelay = nullptr;
    int m_curTurn = 1;
    bool m_swapping = false;

    QQuickItem *componentRoot() const { return qobject_cast<QQuickItem*>(parent()); }
signals:
    void curTurnChanged();
    void swappingChanged();

public:
    void classBegin() Q_DECL_OVERRIDE { }
    void componentComplete() Q_DECL_OVERRIDE;

    int curTurn() const { return m_curTurn; }
    void setCurTurn(int curTurn) {
        if (curTurn == m_curTurn)
            return;
        m_curTurn = curTurn;
        emit curTurnChanged();
    }

    void showPuzzleEnd(bool won);
    void showPuzzleGoal (const QString &str);
    void swapPlayers();

public slots:
    void onMouseClicked(QObject *mouse);
    void showWonImg();
    void hideWonImg();
    void hidePuzzleTextBubble();
};

#endif
