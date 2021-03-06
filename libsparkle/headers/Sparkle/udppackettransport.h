/*
 * Sparkle - zero-configuration fully distributed self-organizing encrypting VPN
 * Copyright (C) 2009 Sergey Gridassov
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

#ifndef __UDP_PACKET_TRANSPORT__H__
#define __UDP_PACKET_TRANSPORT__H__

#include <Sparkle/PacketTransport>
#include <Sparkle/Sparkle>

namespace Sparkle {

class UdpPacketTransportPrivate;

class SPARKLE_DECL UdpPacketTransport : public PacketTransport
{
	Q_OBJECT
	Q_DECLARE_PRIVATE(UdpPacketTransport)
	
protected:
	UdpPacketTransport(UdpPacketTransportPrivate &dd, QObject *parent);
	
public:
	UdpPacketTransport(QHostAddress addr, quint16 port, QObject *parent = 0);
	virtual ~UdpPacketTransport();

	virtual bool beginReceiving();
	virtual quint16 port();

public slots:
	virtual void endReceiving();
	virtual void sendPacket(QByteArray &packet, QHostAddress host, quint16 port);

private slots:
	void haveDatagram();

signals:
	void receivedPacket(QByteArray &packet, QHostAddress host, quint16 port);

protected:
	UdpPacketTransportPrivate * const d_ptr;
};

}

#endif
