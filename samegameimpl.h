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
#ifndef SAMEGAMEIMPL_H
#define SAMEGAMEIMPL_H

#include "gameareaimpl.h"
#include <QQuickItem>
#include <QDateTime>
#include <cmath>
#include <QSqlDatabase>
#include <QSqlQuery>

#include <QStandardPaths>
#include <QQmlEngine>

class SameGameImpl : public QObject, public QQmlParserStatus {
    Q_OBJECT
    int m_maxColumn = 10;
    int m_maxRow = 13;
    const int m_types = 3;
    int m_maxIndex = m_maxColumn*m_maxRow;
    QVector<QQuickItem*> m_board;
    QString m_blockSrc = "Block.qml";
    QDateTime m_gameDuration;
    QScopedPointer<QQmlComponent> m_component;
    QuickImplPointer<GameAreaImpl> m_gameCanvas;
    bool m_betweenTurns = false;

    QQuickItem *m_puzzleLevel = nullptr;
    QString m_puzzlePath;

    QString m_gameMode = "arcade"; //Set in new game, then tweaks behaviour of other functions
    bool m_gameOver = false;

    int m_fillFound;  // Set after a floodFill call to the number of blocks found
    QVector<int> m_floodBoard; // Set to 1 if the floodFill reaches off that node

    Q_PROPERTY(QDateTime gameDuration MEMBER m_gameDuration NOTIFY gameDurationChanged FINAL)
signals:
    void gameDurationChanged();

public:
    SameGameImpl()
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "SameGame");
    }

    void classBegin() Q_DECL_OVERRIDE { }
    void componentComplete() Q_DECL_OVERRIDE
    {
        m_component.reset(new QQmlComponent(qmlEngine(this), QUrl("qrc:///demos/samegame/content/" + m_blockSrc)));
    }

    Q_INVOKABLE void changeBlock(const QString &src)
    {
        m_blockSrc = src;
        m_component.reset(new QQmlComponent(qmlEngine(this), QUrl("qrc:///demos/samegame/content/" + m_blockSrc)));
    }

    Q_INVOKABLE void cleanUp()
    {
        if (!m_gameCanvas)
            return;
        // Delete blocks from previous game
        qDeleteAll(m_board);
        m_board.clear();
        delete m_puzzleLevel;
        m_puzzleLevel = nullptr;

        m_gameCanvas->setProperty("mode", "");
    }

    Q_INVOKABLE void startNewGame(QQuickItem *gc, const QString &mode = "", const QString &map = "")
    {
        m_gameCanvas.reset(gc);
        if (mode.isEmpty())
            m_gameMode = "arcade";
        else
            m_gameMode = mode;
        m_gameOver = false;

        cleanUp();

        m_gameCanvas->setProperty("gameOver", false);
        m_gameCanvas->setProperty("mode", m_gameMode);
        // Calculate board size
        m_maxColumn = floor(m_gameCanvas->property("width").toDouble()/m_gameCanvas->property("blockSize").toDouble());
        m_maxRow = floor(m_gameCanvas->property("height").toDouble()/m_gameCanvas->property("blockSize").toDouble());
        m_maxIndex = m_maxRow * m_maxColumn;
        if (m_gameMode == "arcade") //Needs to be after board sizing
            getHighScore();


        // Initialize Board
        m_board.resize(m_maxIndex);
        m_gameCanvas->setProperty("score", 0);
        m_gameCanvas->setProperty("score2", 0);
        m_gameCanvas->setProperty("moves", 0);
        m_gameCanvas.impl()->setCurTurn(1);
        if (m_gameMode == "puzzle")
            loadMap(map);
        else // Note that we load them in reverse order for correct visual stacking
            for (int column = m_maxColumn - 1; column >= 0; column--)
                for (int row = m_maxRow - 1; row >= 0; row--)
                    createBlock(column, row);
        if (m_gameMode == "puzzle")
            getLevelHistory(); // Needs to be after map load
        m_gameDuration = QDateTime::currentDateTime();
        emit gameDurationChanged();
    }

    Q_INVOKABLE void handleClick(int x, int y)
    {
        if (m_betweenTurns || m_gameOver || !m_gameCanvas)
            return;
        int column = std::floor(x / m_gameCanvas->property("blockSize").toInt());
        int row = std::floor(y / m_gameCanvas->property("blockSize").toInt());
        if (column >= m_maxColumn || column < 0 || row >= m_maxRow || row < 0)
            return;
        if (!m_board[index(column, row)])
            return;
        // If it's a valid block, remove it and all connected (does nothing if it's not connected)
        floodFill(column, row, -1);
        if (m_fillFound <= 0)
            return;
        if (m_gameMode == "multiplayer" && m_gameCanvas.impl()->curTurn() == 2)
            m_gameCanvas->setProperty("score2", m_gameCanvas->property("score2").toInt() + (m_fillFound - 1) * (m_fillFound - 1));
        else
            m_gameCanvas->setProperty("score", m_gameCanvas->property("score").toInt() + (m_fillFound - 1) * (m_fillFound - 1));
        if (m_gameMode == "multiplayer" && m_gameCanvas.impl()->curTurn() == 2)
            shuffleUp();
        else
            shuffleDown();
        m_gameCanvas->setProperty("moves", m_gameCanvas->property("moves").toInt() + 1);
        if (m_gameMode == "endless")
            refill();
        else if (m_gameMode != "multiplayer")
            victoryCheck();
        if (m_gameMode == "multiplayer" && !m_gameCanvas->property("gameOver").toBool()){
            m_betweenTurns = true;
            m_gameCanvas.impl()->swapPlayers();// animate and call turnChange() when ready
        }
    }

    Q_INVOKABLE void turnChange()//called by ui outside
    {
        m_betweenTurns = false;
        if (m_gameCanvas.impl()->curTurn() == 1){
            shuffleUp();
            m_gameCanvas.impl()->setCurTurn(2);
            victoryCheck();
        }else{
            shuffleDown();
            m_gameCanvas.impl()->setCurTurn(1);
            victoryCheck();
        }
    }

    Q_INVOKABLE void finishLoadingMap()
    {
        QJSValue jsStartingGrid = m_puzzleLevel->property("startingGrid").value<QJSValue>();
        QVector<int> startingGrid(jsStartingGrid.property("length").toInt());
        for (int i = 0; i < startingGrid.length(); ++i) {
            startingGrid[i] = jsStartingGrid.property(i).toInt();
            if (! (startingGrid[i] >= 0 && startingGrid[i] <= 9) )
                startingGrid[i] = 0;
        }
        //TODO: Don't allow loading larger levels, leads to cheating
        while (startingGrid.length() > m_maxIndex) startingGrid.removeFirst();
        while (startingGrid.length() < m_maxIndex) startingGrid.prepend(0);
        for (int i = 0; i < startingGrid.length(); ++i)
            if (startingGrid[i] > 0)
                createBlock(i % m_maxColumn, std::floor(i / m_maxColumn), startingGrid[i] - 1);

        //### Experimental feature - allow levels to contain arbitrary QML scenes as well!
        //while (puzzleLevel.children.length)
        //    puzzleLevel.children[0].parent = gameCanvas;
        m_gameDuration = QDateTime::currentDateTime(); //Don't start until we finish loading
        emit gameDurationChanged();
    }

    Q_INVOKABLE void nuke() //For "Debug mode"
    {
        for (int row = 1; row <= 5; row++) {
            for (int col = 0; col < 5; col++) {
                if (m_board[index(col, m_maxRow - row)]) {
                    QMetaObject::invokeMethod(m_board[index(col, m_maxRow - row)], "fadeOut");
                    m_board[index(col, m_maxRow - row)] = nullptr;
                }
            }
        }
        if (m_gameMode == "multiplayer" && m_gameCanvas.impl()->curTurn() == 2)
            shuffleUp();
        else
            shuffleDown();
        if (m_gameMode == "endless")
            refill();
        else
            victoryCheck();
    }

private:
    // Index function used instead of a 2D array
    int index(int column, int row)
    {
        return column + row * m_maxColumn;
    }

    void floodFill(int column, int row, int type)
    {
        if (index(column, row) < 0 || index(column, row) >= m_board.size() || !m_board[index(column, row)])
            return;
        bool first = false;
        if (type == -1) {
            first = true;
            type = m_board[index(column,row)]->property("type").toInt();

            // Flood fill initialization
            m_fillFound = 0;
            m_floodBoard = QVector<int>(m_maxIndex);
        }
        if (column >= m_maxColumn || column < 0 || row >= m_maxRow || row < 0)
            return;
        if (m_floodBoard[index(column, row)] == 1 || (!first && type != m_board[index(column, row)]->property("type").toInt()))
            return;
        m_floodBoard[index(column, row)] = 1;
        floodFill(column + 1, row, type);
        floodFill(column - 1, row, type);
        floodFill(column, row + 1, type);
        floodFill(column, row - 1, type);
        if (first == true && m_fillFound == 0)
            return; // Can't remove single blocks
        QMetaObject::invokeMethod(m_board[index(column, row)], "fadeOut");
        m_board[index(column, row)] = nullptr;
        m_fillFound += 1;
    }

    void shuffleDown()
    {
        // Fall down
        for (int column = 0; column < m_maxColumn; column++) {
            int fallDist = 0;
            for (int row = m_maxRow - 1; row >= 0; row--) {
                if (!m_board[index(column,row)]) {
                    fallDist += 1;
                } else {
                    if (fallDist > 0) {
                        auto obj = m_board[index(column, row)];
                        QMetaObject::invokeMethod(obj, "animateYTo", Q_ARG(QVariant, (row + fallDist) * m_gameCanvas->property("blockSize").toInt()));
                        m_board[index(column, row + fallDist)] = obj;
                        m_board[index(column, row)] = nullptr;
                    }
                }
            }
        }
        // Fall to the left
        int fallDist = 0;
        for (int column = 0; column < m_maxColumn; column++) {
            if (!m_board[index(column, m_maxRow - 1)]) {
                fallDist += 1;
            } else {
                if (fallDist > 0) {
                    for (int row = 0; row < m_maxRow; row++) {
                        auto obj = m_board[index(column, row)];
                        if (!obj)
                            continue;
                        QMetaObject::invokeMethod(obj, "animateXTo", Q_ARG(QVariant, (column - fallDist) * m_gameCanvas->property("blockSize").toInt()));
                        m_board[index(column - fallDist,row)] = obj;
                        m_board[index(column, row)] = nullptr;
                    }
                }
            }
        }
    }

    void shuffleUp()
    {
        // Fall up
        for (int column = 0; column < m_maxColumn; column++) {
            int fallDist = 0;
            for (int row = 0; row < m_maxRow; row++) {
                if (!m_board[index(column,row)]) {
                    fallDist += 1;
                } else {
                    if (fallDist > 0) {
                        auto obj = m_board[index(column, row)];
                        QMetaObject::invokeMethod(obj, "animateYTo", Q_ARG(QVariant, (row - fallDist) * m_gameCanvas->property("blockSize").toInt()));
                        m_board[index(column, row - fallDist)] = obj;
                        m_board[index(column, row)] = nullptr;
                    }
                }
            }
        }
        // Fall to the left (or should it be right, so as to be left for P2?)
        int fallDist = 0;
        for (int column = 0; column < m_maxColumn; column++) {
            if (!m_board[index(column, 0)]) {
                fallDist += 1;
            } else {
                if (fallDist > 0) {
                    for (int row = 0; row < m_maxRow; row++) {
                        auto obj = m_board[index(column, row)];
                        if (!obj)
                            continue;
                        QMetaObject::invokeMethod(obj, "animateXTo", Q_ARG(QVariant, (column - fallDist) * m_gameCanvas->property("blockSize").toInt()));
                        m_board[index(column - fallDist,row)] = obj;
                        m_board[index(column, row)] = nullptr;
                    }
                }
            }
        }
    }

    void refill()
    {
        for (int column = 0; column < m_maxColumn; column++) {
            for (int row = 0; row < m_maxRow; row++) {
                if (!m_board[index(column, row)])
                    createBlock(column, row);
            }
        }
    }

    void victoryCheck()
    {
        // Awards bonuses for no blocks left
        bool deservesBonus = true;
        if (m_board[index(0, m_maxRow - 1)] || m_board[index(0, 0)])
            deservesBonus = false;
        // Checks for game over
        if (deservesBonus){
            if (m_gameCanvas.impl()->curTurn() == 1)
                m_gameCanvas->setProperty("score", m_gameCanvas->property("score").toInt() + 1000);
            else
                m_gameCanvas->setProperty("score2", m_gameCanvas->property("score2").toInt() + 1000);
        }
        m_gameOver = deservesBonus;
        if (m_gameCanvas.impl()->curTurn() == 1){
            if (!(floodMoveCheck(0, m_maxRow - 1, -1)))
                m_gameOver = true;
        }else{
            if (!(floodMoveCheck(0, 0, -1, true)))
                m_gameOver = true;
        }
        if (m_gameMode == "puzzle"){
            puzzleVictoryCheck(deservesBonus);//Takes it from here
            return;
        }
        if (m_gameOver) {
            int winnerScore = std::max(m_gameCanvas->property("score").toInt(), m_gameCanvas->property("score2").toInt());
            if (m_gameMode == "multiplayer"){
                m_gameCanvas->setProperty("score", winnerScore);
                saveHighScore(m_gameCanvas->property("score2").toInt());
            }
            saveHighScore(m_gameCanvas->property("score").toInt());
            m_gameDuration = QDateTime::currentDateTime();
            emit gameDurationChanged();
            m_gameCanvas->setProperty("gameOver", true);
        }
    }

    // Only floods up and right, to see if it can find adjacent same-typed blocks
    bool floodMoveCheck(int column, int row, int type, bool goDownInstead = false)
    {
        if (column >= m_maxColumn || column < 0 || row >= m_maxRow || row < 0)
            return false;
        if (!m_board[index(column, row)])
            return false;
        int myType = m_board[index(column, row)]->property("type").toInt();
        if (type == myType)
            return true;
        if (goDownInstead)
            return floodMoveCheck(column + 1, row, myType, goDownInstead) ||
                   floodMoveCheck(column, row + 1, myType, goDownInstead);
        else
            return floodMoveCheck(column + 1, row, myType) ||
                   floodMoveCheck(column, row - 1, myType);
    }

    bool createBlock(int column, int row)
    {
        return createBlock(column, row, column % m_types);//Math.floor(Math.random() * types););
    }

    bool createBlock(int column, int row, int type)
    {
        // Note that we don't wait for the component to become ready. This will
        // only work if the block QML is a local file. Otherwise the component will
        // not be ready immediately. There is a statusChanged signal on the
        // component you could use if you want to wait to load remote files.
        if (m_component->isReady()) {
            if (type < 0 || type > 4) {
                qWarning("Invalid type requested");//TODO: Is this triggered by custom levels much?
                return false;
            }
            QQuickItem *dynamicObject = qobject_cast<QQuickItem*>(m_component->beginCreate(qmlContext(this)));
            if (!dynamicObject) {
                qWarning("error creating block");
                qWarning() << m_component->errorString();
                return false;
            }
            int blockSize = m_gameCanvas->property("blockSize").toInt();
            dynamicObject->setParentItem(m_gameCanvas);
            dynamicObject->setProperty("type", type);
            dynamicObject->setProperty("x", column * blockSize);
            dynamicObject->setProperty("y", -1 * blockSize);
            dynamicObject->setProperty("width", blockSize);
            dynamicObject->setProperty("height", blockSize);
            m_component->completeCreate();

            // The block will be destroyed by QML.
            QQmlEngine::setObjectOwnership(dynamicObject, QQmlEngine::JavaScriptOwnership);

            QMetaObject::invokeMethod(dynamicObject, "animateYTo", Q_ARG(QVariant, row * blockSize));
            QMetaObject::invokeMethod(dynamicObject, "fadeIn");
            m_board[index(column,row)] = dynamicObject;
        } else {
            qWarning("error loading block component");
            qWarning() << m_component->errorString();
            return false;
        }
        return true;
    }

    void loadMap(const QString &map)
    {
        m_puzzlePath = map;
        QQmlComponent levelComp(qmlEngine(this), QUrl("qrc:///demos/samegame/content/" + m_puzzlePath));
        if (levelComp.status() != QQmlComponent::Ready){
            qWarning("Error loading level");
            qWarning() << levelComp.errorString();
            return;
        }
        m_puzzleLevel = qobject_cast<QQuickItem*>(levelComp.create(qmlContext(this)));
        if (!m_puzzleLevel || !m_puzzleLevel->property("startingGrid").value<QJSValue>().isArray()) {
            qWarning("Bugger!");
            return;
        }
        m_gameCanvas.impl()->showPuzzleGoal(m_puzzleLevel->property("goalText").toString());
        //showPuzzleGoal should call finishLoadingMap as the next thing it does, before handling more events
    }

    void puzzleVictoryCheck(bool clearedAll) // m_gameOver has also been set if no more moves
    {
        bool won = true;
        quint64 soFar = m_gameDuration.msecsTo(QDateTime::currentDateTime());
        int score = m_gameCanvas->property("score").toInt();
        int scoreTarget = m_puzzleLevel->property("scoreTarget").toInt();
        bool mustClear = m_puzzleLevel->property("mustClear").toBool();
        if (scoreTarget != -1 && score < scoreTarget){
            won = false;
        }
        if (scoreTarget != -1 && score >= scoreTarget && !mustClear){
            m_gameOver = true;
        }
        int timeTarget = m_puzzleLevel->property("timeTarget").toInt();
        if (timeTarget != -1 && soFar/1000.0 > timeTarget){
            m_gameOver = true;
        }
        int moves = m_gameCanvas->property("moves").toInt();
        int moveTarget = m_puzzleLevel->property("moveTarget").toInt();
        if (moveTarget != -1 && moves >= moveTarget){
            m_gameOver = true;
        }
        if (mustClear && m_gameOver && !clearedAll) {
            won = false;
        }

        if (m_gameOver) {
            m_gameCanvas->setProperty("gameOver", true);
            m_gameCanvas.impl()->showPuzzleEnd(won);

            if (won) {
                // Store progress
                saveLevelHistory();
            }
        }
    }

    void getHighScore()
    {
        QSqlDatabase db = QSqlDatabase::database();
        db.transaction();

        db.exec("CREATE TABLE IF NOT EXISTS Scores(game TEXT, score NUMBER, gridSize TEXT, time NUMBER)");

        QSqlQuery rs = db.exec(QString("SELECT * FROM Scores WHERE gridSize = \"%1x%2\" AND game = \"%3\" ORDER BY score desc")
            .arg(m_maxColumn)
            .arg(m_maxRow)
            .arg(m_gameMode));
        if (rs.next())
            m_gameCanvas->setProperty("highScore", rs.value("score"));
        else
            m_gameCanvas->setProperty("highScore", 0);

        db.commit();
    }

    void saveHighScore(int score)
    {
        QSqlDatabase db = QSqlDatabase::database();
        if (score >= m_gameCanvas->property("highScore").toInt())//Update UI field
            m_gameCanvas->setProperty("highScore", score);
        db.transaction();

        db.exec("CREATE TABLE IF NOT EXISTS Scores(game TEXT, score NUMBER, gridSize TEXT, time NUMBER)");

        QSqlQuery query(db);
        query.prepare("INSERT INTO Scores VALUES(?, ?, ?, ?)");
        query.addBindValue(m_gameMode);
        query.addBindValue(score);
        query.addBindValue(QString("%1x%2").arg(m_maxColumn).arg(m_maxRow));
        query.addBindValue(m_gameDuration.secsTo(QDateTime::currentDateTime()));
        query.exec();

        db.commit();
    }

    void getLevelHistory()
    {
        QSqlDatabase db = QSqlDatabase::database();
        db.transaction();

        db.exec("CREATE TABLE IF NOT EXISTS Puzzle(level TEXT, score NUMBER, moves NUMBER, time NUMBER)");

        QSqlQuery rs = db.exec(QString("SELECT * FROM Puzzle WHERE level = \"%1\" ORDER BY score desc")
            .arg(m_puzzlePath));
        if (rs.next()) {
            m_gameCanvas->setProperty("puzzleWon", true);
            m_gameCanvas->setProperty("highScore", rs.value("score"));
        } else {
            m_gameCanvas->setProperty("puzzleWon", false);
            m_gameCanvas->setProperty("highScore", 0);
        }

        db.commit();
    }

    void saveLevelHistory()
    {
        QSqlDatabase db = QSqlDatabase::database();
        m_gameCanvas->setProperty("puzzleWon", true);
        db.transaction();

        db.exec("CREATE TABLE IF NOT EXISTS Puzzle(level TEXT, score NUMBER, moves NUMBER, time NUMBER)");

        QSqlQuery query(db);
        query.prepare("INSERT INTO Puzzle VALUES(?, ?, ?, ?)");
        query.addBindValue(m_puzzlePath);
        query.addBindValue(m_gameCanvas->property("score"));
        query.addBindValue(m_gameCanvas->property("moves"));
        query.addBindValue(m_gameDuration.secsTo(QDateTime::currentDateTime()));
        query.exec();

        db.commit();
    }
};

#endif
