/*
 * Sparkle - zero-configuration fully distributed self-organizing encrypting VPN
 * Copyright (C) 2009 Peter Zotov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LOG_H__
#define __LOG_H__

#include <Sparkle/Sparkle>
#include <QObject>
#include <QString>
#include <QStringList>

class QHostAddress;

namespace Sparkle {

class SparkleNode;

class SPARKLE_DECL Log {
	enum loglevel_t {
		Debug		= 1,
		Info		= 2,
		Warning		= 3,
		Error		= 4,
		Fatal		= 5
	};

	class Stream {
	public:
		Stream(QString _format, loglevel_t _loglevel) : format(_format), loglevel(_loglevel), base(10) {}

		QString format;
		QStringList list;
		loglevel_t loglevel;
		uint base;
	};

	Stream* stream;

public:
	explicit inline Log(const char* format, loglevel_t loglevel = Info) : stream(new Stream(QString(format), loglevel)) {}
	inline ~Log() { Log::emitMessage(stream->loglevel, prepare()); }

	Log& operator<<(short v);
	Log& operator<<(ushort v);
	Log& operator<<(int v);
	Log& operator<<(uint v);
	Log& operator<<(long v);
	Log& operator<<(ulong v);

	Log& operator<<(double v);

	Log& operator<<(char v);
	Log& operator<<(const char* v);
	Log& operator<<(bool v);
	Log& operator<<(const QString &v);

	Log& operator<<(const QHostAddress &v);

	Log& operator<<(const SparkleNode &v);

	inline static Log debug(const char* format)	{ return Log(format, Debug);	}
	inline static Log info(const char* format)	{ return Log(format, Info);	}
	inline static Log warn(const char* format)	{ return Log(format, Warning);	}
	inline static Log error(const char* format)	{ return Log(format, Error);	}
	inline static Log fatal(const char* format)	{ return Log(format, Fatal);	}

	QString prepare();

	static void emitMessage(loglevel_t loglevel, QString message);
};

}

#endif
