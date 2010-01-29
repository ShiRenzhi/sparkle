/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtMultimedia module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtMultimedia/qaudioengine.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAbstractAudioDeviceInfo
    \brief The QAbstractAudioDeviceInfo class provides access for QAudioDeviceInfo to access the audio
    device provided by the plugin.
    \internal

    \ingroup multimedia

    This class implements the audio functionality for
    QAudioDeviceInfo, i.e., QAudioDeviceInfo keeps a
    QAbstractAudioDeviceInfo and routes function calls to it. For a
    description of the functionality that QAbstractAudioDeviceInfo
    implements, you can read the class and functions documentation of
    QAudioDeviceInfo.

    \sa QAudioDeviceInfo
*/

/*!
    \fn virtual QAudioFormat QAbstractAudioDeviceInfo::preferredFormat() const
    Returns the nearest settings.
*/

/*!
    \fn virtual bool QAbstractAudioDeviceInfo::isFormatSupported(const QAudioFormat& format) const
    Returns true if \a format is available from audio device.
*/

/*!
    \fn virtual QAudioFormat QAbstractAudioDeviceInfo::nearestFormat(const QAudioFormat& format) const
    Returns the nearest settings \a format.
*/

/*!
    \fn virtual QString QAbstractAudioDeviceInfo::deviceName() const
    Returns the audio device name.
*/

/*!
    \fn virtual QStringList QAbstractAudioDeviceInfo::codecList()
    Returns the list of currently available codecs.
*/

/*!
    \fn virtual QList<int> QAbstractAudioDeviceInfo::frequencyList()
    Returns the list of currently available frequencies.
*/

/*!
    \fn virtual QList<int> QAbstractAudioDeviceInfo::channelsList()
    Returns the list of currently available channels.
*/

/*!
    \fn virtual QList<int> QAbstractAudioDeviceInfo::sampleSizeList()
    Returns the list of currently available sample sizes.
*/

/*!
    \fn virtual QList<QAudioFormat::Endian> QAbstractAudioDeviceInfo::byteOrderList()
    Returns the list of currently available byte orders.
*/

/*!
    \fn virtual QList<QAudioFormat::SampleType> QAbstractAudioDeviceInfo::sampleTypeList()
    Returns the list of currently available sample types.
*/

/*!
    \class QAbstractAudioOutput
    \brief The QAbstractAudioOutput class provides access for QAudioOutput to access the audio
    device provided by the plugin.
    \internal

    \ingroup multimedia

    QAbstractAudioOutput implements audio functionality for
    QAudioOutput, i.e., QAudioOutput routes function calls to
    QAbstractAudioOutput. For a description of the functionality that
    is implemented, see the QAudioOutput class and function
    descriptions.

    \sa QAudioOutput
*/

/*!
    \fn virtual QIODevice* QAbstractAudioOutput::start(QIODevice* device)
    Uses the \a device as the QIODevice to transfer data. If \a device is null then the class
    creates an internal QIODevice. Returns a pointer to the QIODevice being used to handle
    the data transfer. This QIODevice can be used to write() audio data directly. Passing a
    QIODevice allows the data to be transfered without any extra code.
*/

/*!
    \fn virtual void QAbstractAudioOutput::stop()
    Stops the audio output.
*/

/*!
    \fn virtual void QAbstractAudioOutput::reset()
    Drops all audio data in the buffers, resets buffers to zero.
*/

/*!
    \fn virtual void QAbstractAudioOutput::suspend()
    Stops processing audio data, preserving buffered audio data.
*/

/*!
    \fn virtual void QAbstractAudioOutput::resume()
    Resumes processing audio data after a suspend()
*/

/*!
    \fn virtual int QAbstractAudioOutput::bytesFree() const
    Returns the free space available in bytes in the audio buffer.
*/

/*!
    \fn virtual int QAbstractAudioOutput::periodSize() const
    Returns the period size in bytes.
*/

/*!
    \fn virtual void QAbstractAudioOutput::setBufferSize(int value)
    Sets the audio buffer size to \a value in bytes.
*/

/*!
    \fn virtual int QAbstractAudioOutput::bufferSize() const
    Returns the audio buffer size in bytes.
*/

/*!
    \fn virtual void QAbstractAudioOutput::setNotifyInterval(int ms)
    Sets the interval for notify() signal to be emitted. This is based on the \a ms
    of audio data processed not on actual real-time. The resolution of the timer
    is platform specific.
*/

/*!
    \fn virtual int QAbstractAudioOutput::notifyInterval() const
    Returns the notify interval in milliseconds.
*/

/*!
    \fn virtual qint64 QAbstractAudioOutput::processedUSecs() const
    Returns the amount of audio data processed since start() was called in milliseconds.
*/

/*!
    \fn virtual qint64 QAbstractAudioOutput::elapsedUSecs() const
    Returns the milliseconds since start() was called, including time in Idle and suspend states.
*/

/*!
    \fn virtual QAudio::Error QAbstractAudioOutput::error() const
    Returns the error state.
*/

/*!
    \fn virtual QAudio::State QAbstractAudioOutput::state() const
    Returns the state of audio processing.
*/

/*!
    \fn virtual QAudioFormat QAbstractAudioOutput::format() const
    Returns the QAudioFormat being used.
*/

/*!
    \fn QAbstractAudioOutput::stateChanged(QAudio::State state)
    This signal is emitted when the device \a state has changed.
*/

/*!
    \fn QAbstractAudioOutput::notify()
    This signal is emitted when x ms of audio data has been processed
    the interval set by setNotifyInterval(x).
*/


/*!
    \class QAbstractAudioInput
    \brief The QAbstractAudioInput class provides access for QAudioInput to access the audio
    device provided by the plugin.
    \internal

    \ingroup multimedia

    QAudioDeviceInput keeps an instance of QAbstractAudioInput and
    routes calls to functions of the same name to QAbstractAudioInput.
    This means that it is QAbstractAudioInput that implements the
    audio functionality. For a description of the functionality, see
    the QAudioInput class description.

    \sa QAudioInput
*/

/*!
    \fn virtual QIODevice* QAbstractAudioInput::start(QIODevice* device)
    Uses the \a device as the QIODevice to transfer data. If \a device is null
    then the class creates an internal QIODevice. Returns a pointer to the
    QIODevice being used to handle the data transfer. This QIODevice can be used to
    read() audio data directly. Passing a QIODevice allows the data to be transfered
    without any extra code.
*/

/*!
    \fn virtual void QAbstractAudioInput::stop()
    Stops the audio input.
*/

/*!
    \fn virtual void QAbstractAudioInput::reset()
    Drops all audio data in the buffers, resets buffers to zero.
*/

/*!
    \fn virtual void QAbstractAudioInput::suspend()
    Stops processing audio data, preserving buffered audio data.
*/

/*!
    \fn virtual void QAbstractAudioInput::resume()
    Resumes processing audio data after a suspend().
*/

/*!
    \fn virtual int QAbstractAudioInput::bytesReady() const
    Returns the amount of audio data available to read in bytes.
*/

/*!
    \fn virtual int QAbstractAudioInput::periodSize() const
    Returns the period size in bytes.
*/

/*!
    \fn virtual void QAbstractAudioInput::setBufferSize(int value)
    Sets the audio buffer size to \a value in milliseconds.
*/

/*!
    \fn virtual int QAbstractAudioInput::bufferSize() const
    Returns the audio buffer size in milliseconds.
*/

/*!
    \fn virtual void QAbstractAudioInput::setNotifyInterval(int ms)
    Sets the interval for notify() signal to be emitted. This is based
    on the \a ms of audio data processed not on actual real-time.
    The resolution of the timer is platform specific.
*/

/*!
    \fn virtual int QAbstractAudioInput::notifyInterval() const
    Returns the notify interval in milliseconds.
*/

/*!
    \fn virtual qint64 QAbstractAudioInput::processedUSecs() const
    Returns the amount of audio data processed since start() was called in milliseconds.
*/

/*!
    \fn virtual qint64 QAbstractAudioInput::elapsedUSecs() const
    Returns the milliseconds since start() was called, including time in Idle and suspend states.
*/

/*!
    \fn virtual QAudio::Error QAbstractAudioInput::error() const
    Returns the error state.
*/

/*!
    \fn virtual QAudio::State QAbstractAudioInput::state() const
    Returns the state of audio processing.
*/

/*!
    \fn virtual QAudioFormat QAbstractAudioInput::format() const
    Returns the QAudioFormat being used
*/

/*!
    \fn QAbstractAudioInput::stateChanged(QAudio::State state)
    This signal is emitted when the device \a state has changed.
*/

/*!
    \fn QAbstractAudioInput::notify()
    This signal is emitted when x ms of audio data has been processed
    the interval set by setNotifyInterval(x).
*/


QT_END_NAMESPACE
