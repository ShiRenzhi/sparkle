/*
 * Sippy - zero-configuration fully distributed self-organizing encrypting IM
 * Copyright (C) 2009 Sergey Gridassov
 *
 * Ths program is free software: you can redistribute it and/or modify
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

#ifndef __EXTENDED_LOGIN__H__
#define __EXTENDED_LOGIN__H__

#include <QObject>

#include <QHostInfo>

class LinkLayer;

class ExtendedLogin: public QObject {
	Q_OBJECT

public:
	ExtendedLogin(LinkLayer *link, QObject *parent = 0);
	virtual ~ExtendedLogin();

public slots:
	void login(bool create, QString host, bool behindNat);
	void signaled();

private slots:
	void sippyClosed();
	void linkShutDown();
	void linkJoinFailed();
	void linkJoined();

	void hostnameResolved(QHostInfo info);

signals:
	void loggedIn();
	void loginFailed(QString message);

private:
	void doRealLogin(QHostAddress address);

private:
	LinkLayer *link;

	bool isClosed, behindNat, createNetwork;
	QString enteredHost;
};

#endif

