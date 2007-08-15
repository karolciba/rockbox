/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


#ifndef HTTPGET_H
#define HTTPGET_H

#include <QFile>
#include <QHttp>

class QUrl;

class HttpGet : public QObject
{
    Q_OBJECT

    public:
        HttpGet(QObject *parent = 0);

        bool getFile(const QUrl &url);
        void setProxy(const QUrl &url);
        QHttp::Error error(void);
        QString errorString(void);
        void setFile(QFile*);
        int httpResponse(void);
        QByteArray readAll(void);

    public slots:
        void abort(void);

    signals:
        void done(bool);
        void dataReadProgress(int, int);
        void requestFinished(int, bool);

    private slots:
        void httpDone(bool error);
        void httpProgress(int, int);
        void httpFinished(int, bool);
        void httpResponseHeader(const QHttpResponseHeader&);
        void httpState(int);
        void httpStarted(int);

    private:
        QHttp http;
        QFile *outputFile;
        int response;
        int getRequest;
        QByteArray dataBuffer;
        bool outputToBuffer;
};

#endif
