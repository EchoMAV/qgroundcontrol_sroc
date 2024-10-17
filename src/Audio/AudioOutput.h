/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QList>
#include <QPair>
#include <QTextToSpeech>

#include "QGCToolbox.h"

class QGCApplication;
class QMediaPlayer;

/// Text to Speech Interface
class AudioOutput : public QGCTool
{
    Q_OBJECT
public:
    AudioOutput(QGCApplication* app, QGCToolbox* toolbox);

    static bool     getMillisecondString    (const QString& string, QString& match, int& number);
    static QString  fixTextMessageForAudio  (const QString& string);

public slots:
    /// Convert string to speech output and say it
    void            say                     (const QString& text);
    /// Play media from a file
    void            playFromFile            (const QString& file);

private slots:
    void            _stateChanged           (QTextToSpeech::State state);

protected:
    QTextToSpeech*  _tts;
    QMediaPlayer*   _mp;
    QList<QPair<QString,bool>> _toPlay; //second bool: true = say, false = play file
};

