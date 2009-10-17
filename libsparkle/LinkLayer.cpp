/*
 * Sparkle - zero-configuration fully distributed self-organizing encrypting VPN
 * Copyright (C) 2009 Sergey Gridassov, Peter Zotov
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

#include <QCoreApplication>
#include <QStringList>
#include <QHostInfo>
#include <QTimer>
#include <QtGlobal>
#ifdef Q_OS_WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

#include "LinkLayer.h"
#include "SparkleNode.h"
#include "PacketTransport.h"
#include "SHA1Digest.h"
#include "Router.h"
#include "Log.h"
#include "ApplicationLayer.h"

LinkLayer::LinkLayer(Router &_router, PacketTransport &_transport, RSAKeyPair &_hostKeyPair, ApplicationLayer *_app)
		: QObject(NULL), hostKeyPair(_hostKeyPair), router(_router), transport(_transport),
		preparingForShutdown(false), transportInitiated(false), app(_app)
{
	connect(&transport, SIGNAL(receivedPacket(QByteArray&, QHostAddress, quint16)),
			SLOT(handlePacket(QByteArray&, QHostAddress, quint16)));
	
	pingTimer = new QTimer(this);
	pingTimer->setSingleShot(true);
	pingTimer->setInterval(5000);
	connect(pingTimer, SIGNAL(timeout()), SLOT(pingTimeout()));
	
	joinTimer = new QTimer(this);
	joinTimer->setSingleShot(true);
	joinTimer->setInterval(5000);
	connect(joinTimer, SIGNAL(timeout()), SLOT(joinTimeout()));
	
	Log::debug("link layer (protocol version %1) is ready") << ProtocolVersion;

	app->attachLinkLayer(this);
}

bool LinkLayer::joinNetwork(QHostAddress remoteIP, quint16 remotePort, bool forceBehindNAT) {
	Log::debug("link: joining via [%1]:%2") << remoteIP << remotePort;

	if(!initTransport())
		return false;
	
	this->forceBehindNAT = forceBehindNAT;
	
	joinStep = JoinVersionRequest;
	sendProtocolVersionRequest(wrapNode(remoteIP, remotePort));
	
	joinTimer->start();

	return true;
}

void LinkLayer::joinTimeout() {
	Log::error("link: join timeout");

	revertJoin();

	emit joinFailed();
}

bool LinkLayer::createNetwork(QHostAddress localIP, quint8 networkDivisor) {
	SparkleNode *self = new SparkleNode(localIP, transport.getPort());
	Q_CHECK_PTR(self);
	self->setMaster(true);
	self->setAuthKey(hostKeyPair);
	self->configureByKey();
	
	router.setSelfNode(self);
	
	if(!initTransport())
		return false;
	
	Log::debug("link: created network, my endpoint is [%1]:%2") << localIP << transport.getPort();
	
	this->networkDivisor = networkDivisor;
	Log::debug("link: network divisor is 1/%1") << networkDivisor;
	
	joinStep = JoinFinished;
	emit joined(self);

	return true;
}

void LinkLayer::exitNetwork() {
	if(joinStep != JoinFinished) {
		Log::debug("link: join isn't finished, skipping finalization");
		emit readyForShutdown();
		return;
	}	
	
	if(router.getSelfNode()->isMaster() && router.getMasters().count() == 1) {
		Log::debug("link: i'm the last master");
		reincarnateSomeone();
	} else {
		Log::debug("link: sending exit notification");
		sendExitNotification(router.selectMaster());
	}

	if(awaitingNegotiation.count() > 0)
		preparingForShutdown = true;
	else
		emit readyForShutdown();
}

bool LinkLayer::initTransport() {
	if(transportInitiated)
		return true;

	if(!transport.beginReceiving()) {
		Log::error("link: cannot initiate transport (port is already bound?)");
		return false;
	} else {
		Log::debug("link: transport initiated");

		transportInitiated = true;

		return true;
	}
}

bool LinkLayer::isMaster() {
	if(router.getSelfNode() == NULL)
		return false;
	else
		return router.getSelfNode()->isMaster();
}

SparkleNode* LinkLayer::wrapNode(QHostAddress host, quint16 port) {
	foreach(SparkleNode* node, nodeSpool) {	
		if(node->getRealIP() == host && node->getRealPort() == port)
			return node;
	}
	
	SparkleNode* node = new SparkleNode(host, port);
	Q_CHECK_PTR(node);
	nodeSpool.append(node);
	
	connect(node, SIGNAL(negotiationTimedOut(SparkleNode*)), SLOT(negotiationTimeout(SparkleNode*)));
	
	//Log::debug("link: added [%1]:%2 to node spool") << host << port;
	
	return node;
}

void LinkLayer::sendPacket(packet_type_t type, QByteArray data, SparkleNode* node) {
	packet_header_t hdr;
	hdr.length = sizeof(packet_header_t) + data.size();
	hdr.type = type;

	data.prepend(QByteArray((const char *) &hdr, sizeof(packet_header_t)));

	if(node == router.getSelfNode()) {
		Log::error("link: attempting to send packet to myself, dropping");
		return;
	}

	transport.sendPacket(data, node->getRealIP(), node->getRealPort());
}

void LinkLayer::sendEncryptedPacket(packet_type_t type, QByteArray data, SparkleNode *node) {
	packet_header_t hdr;
	hdr.length = sizeof(packet_header_t) + data.size();
	hdr.type = type;

	data.prepend(QByteArray((const char *) &hdr, sizeof(packet_header_t)));
	
	if(!node->areKeysNegotiated()) {
		node->pushQueue(data);
		if(awaitingNegotiation.contains(node)) {
			Log::warn("link: [%1]:%2 still awaiting negotiation") << *node;
		} else {
			Log::debug("link: initiating negotiation with [%1]:%2") << *node;
			
			node->negotiationStart();
			awaitingNegotiation.append(node);
			sendPublicKeyExchange(node, &hostKeyPair, true);
		}
	} else {
		encryptAndSend(data, node);
	}
}

void LinkLayer::encryptAndSend(QByteArray data, SparkleNode *node) {
	Q_ASSERT(node->areKeysNegotiated());
	
	sendPacket(EncryptedPacket, node->getMySessionKey()->encrypt(data), node);
}

void LinkLayer::negotiationTimeout(SparkleNode* node) {
	Log::warn("link: negotiation timeout for [%1]:%2, dropping queue") << *node;
	
	node->flushQueue();
	awaitingNegotiation.removeOne(node);
	
	if(awaitingNegotiation.count() == 0 && preparingForShutdown)
		emit readyForShutdown();
}


void LinkLayer::handlePacket(QByteArray &data, QHostAddress host, quint16 port) {
	const packet_header_t *hdr = (packet_header_t *) data.constData();

	if((size_t) data.size() < sizeof(packet_header_t) || hdr->length != data.size()) {
		Log::warn("link: malformed packet from [%1]:%2") << host << port;
		return;
	}

	QByteArray payload = data.right(data.size() - sizeof(packet_header_t));
	SparkleNode* node = wrapNode(host, port);

	switch((packet_type_t) hdr->type) {		
		case ProtocolVersionRequest:
			handleProtocolVersionRequest(payload, node);
			return;

		case ProtocolVersionReply:
			handleProtocolVersionReply(payload, node);
			return;
		
		case PublicKeyExchange:
			handlePublicKeyExchange(payload, node);
			return;
		
		case SessionKeyExchange:
			handleSessionKeyExchange(payload, node);
			return;
		
		case Ping:
			handlePing(payload, node);
			return;
		
		case EncryptedPacket:
			break; // will be handled later
		
		default: {
			Log::warn("link: packet of unknown type %1 from [%2]:%3") <<
					hdr->type << host << port;
			return;
		}
	}

	// at this point we have encrypted packet in payload.	
	if(!node->areKeysNegotiated()) {
		Log::warn("link: no keys for encrypted packet from [%1]:%2") <<
				host << port;
		return;
	}
	
	// don't call handlePacket again to prevent receiving of unencrypted packet
	//   of types that imply encryption
	
	QByteArray decData = node->getHisSessionKey()->decrypt(payload);
	const packet_header_t *decHdr = (packet_header_t *) decData.constData();

	if((size_t) decData.size() < sizeof(packet_header_t) || 
			decHdr->length < sizeof(packet_header_t) ||
			decHdr->length > decData.size()) {
		Log::warn("link: malformed encrypted payload from [%1]:%2") << host << port;
		return;
	}

	// Blowfish requires 64-bit chunks, here we truncate alignment zeroes at end
	if(decData.size() > decHdr->length && decData.size() < decHdr->length + 8)
		decData.resize(decHdr->length);
	
	QByteArray decPayload = decData.right(decData.size() - sizeof(packet_header_t));
	switch((packet_type_t) decHdr->type) {
		case IntroducePacket:
			handleIntroducePacket(decPayload, node);
			return;
			
		case MasterNodeRequest:
			handleMasterNodeRequest(decPayload, node);
			return;
		
		case MasterNodeReply:
			handleMasterNodeReply(decPayload, node);
			return;
		
		case PingRequest:
			handlePingRequest(decPayload, node);
			return;
		
		case PingInitiate:
			handlePingInitiate(decPayload, node);
			return;
		
		case RegisterRequest:
			handleRegisterRequest(decPayload, node);
			return;
		
		case RegisterReply:
			handleRegisterReply(decPayload, node);
			return;
		
		case Route:
			handleRoute(decPayload, node);
			return;
		
		case RouteRequest:
			handleRouteRequest(decPayload, node);
			return;
		
		case RouteMissing:
			handleRouteMissing(decPayload, node);
			return;
		
		case RouteInvalidate:
			handleRouteInvalidate(decPayload, node);
			return;
		
		case RoleUpdate:
			handleRoleUpdate(decPayload, node);
			return;
		
		case ExitNotification:
			handleExitNotification(decPayload, node);
			return;
		
		case DataPacket:
			app->handleDataPacket(decPayload, node);
			return;
		
		default: {
			Log::warn("link: encrypted packet of unknown type %1 from [%2]:%3") <<
					decHdr->type << host << port;
		}
	}
}

bool LinkLayer::checkPacketSize(QByteArray& payload, quint16 requiredSize,
					 SparkleNode* node, const char* packetName,
						 packet_size_class_t sizeClass) {
	if((payload.size() != requiredSize && sizeClass == PacketSizeEqual) ||
		(payload.size() <= requiredSize && sizeClass == PacketSizeGreater)) {
		Log::warn("link: malformed %3 packet from [%1]:%2") << *node << packetName;
		return false;
	}
	
	return true;
}

bool LinkLayer::checkPacketExpection(SparkleNode* node, const char* packetName, join_step_t neededStep) {
	if(joinStep != neededStep) {
		Log::warn("link: unexpected %3 packet from [%1]:%2") << *node << packetName;
		return false;
	}
	
	return true;
}

/* ======= PACKET HANDLERS ======= */

/* ProtocolVersionRequest */

void LinkLayer::sendProtocolVersionRequest(SparkleNode* node) {
	sendPacket(ProtocolVersionRequest, QByteArray(), node);
}

void LinkLayer::handleProtocolVersionRequest(QByteArray &payload, SparkleNode* node) {
	if(!checkPacketSize(payload, 0, node, "ProtocolVersionRequest"))
		return;

	sendProtocolVersionReply(node);
}

/* ProtocolVersionReply */

void LinkLayer::sendProtocolVersionReply(SparkleNode* node) {
	protocol_version_reply_t ver;
	ver.version = ProtocolVersion;

	sendPacket(ProtocolVersionReply, QByteArray((const char*) &ver, sizeof(ver)), node);
}

void LinkLayer::handleProtocolVersionReply(QByteArray &payload, SparkleNode* node) {
	if(!checkPacketSize(payload, sizeof(protocol_version_reply_t), node, "ProtocolVersionReply"))
		return;

	if(!checkPacketExpection(node, "ProtocolVersionReply", JoinVersionRequest))
		return;

	const protocol_version_reply_t *ver = (const protocol_version_reply_t *) payload.data();
	Log::debug("link: remote protocol version: %1") << ver->version;
	
	if(ver->version != ProtocolVersion) {
		Log::error("link: protocol version mismatch: got %1, expected %2") << ver->version << ProtocolVersion;

		revertJoin();

		emit joinFailed();
	}
	
	joinStep = JoinMasterNodeRequest;
	sendMasterNodeRequest(node);
	
	joinTimer->start();
}

/* PublicKeyExchange */

void LinkLayer::sendPublicKeyExchange(SparkleNode* node, const RSAKeyPair* key, bool needHisKey, quint32 cookie) {
	key_exchange_t ke;
	ke.needOthersKey = needHisKey;
	
	if(needHisKey) {
		ke.cookie = qrand();
		cookies[ke.cookie] = node;
	} else {
		ke.cookie = cookie;
	}
	
	QByteArray request;
	if(key)	request.append(key->getPublicKey());
	else 	request.append(router.getSelfNode()->getAuthKey()->getPublicKey());
	request.prepend(QByteArray((const char*) &ke, sizeof(ke)));
	
	sendPacket(PublicKeyExchange, request, node);
}

void LinkLayer::handlePublicKeyExchange(QByteArray &payload, SparkleNode* node) {
	if(!checkPacketSize(payload, sizeof(key_exchange_t), node, "PublicKeyExchange", PacketSizeGreater))
		return;

	const key_exchange_t *ke = (const key_exchange_t*) payload.constData();

	QByteArray key = payload.mid(sizeof(key_exchange_t));
	if(!ke->needOthersKey && !cookies.contains(ke->cookie)) {
		cookies.remove(ke->cookie);
		Log::warn("link: unexpected pubkey from [%1]:%2") << *node;
		return;
	}
		
	if(!node->setAuthKey(key)) {
		Log::warn("link: received malformed public key from [%1]:%2") << *node;
		awaitingNegotiation.removeOne(node);
		return;
	} else {
		Log::debug("link: received public key for [%1]:%2") << *node;
	}

	if(ke->needOthersKey) {
		sendPublicKeyExchange(node, NULL, false, ke->cookie);
	} else {
		SparkleNode* origNode = cookies[ke->cookie];
		cookies.remove(ke->cookie);
		
		if(*origNode != *node) {
			Log::info("link: node [%1]:%2 is apparently behind the same NAT, rewriting")
				<< *origNode;
			origNode->setRealIP(node->getRealIP());
			origNode->setRealPort(node->getRealPort());
			origNode->setAuthKey(key);
			node = origNode;
		}
		
		if(router.getSelfNode() != NULL && !router.getSelfNode()->isMaster())
			sendIntroducePacket(node);
		
		sendSessionKeyExchange(node, true);
	}
}

/* SessionKeyExchange */

void LinkLayer::sendSessionKeyExchange(SparkleNode* node, bool needHisKey) {
	key_exchange_t ke;
	ke.needOthersKey = needHisKey;
	
	QByteArray request;
	request.append(node->getMySessionKey()->getBytes());
	request.prepend(QByteArray((const char*) &ke, sizeof(ke)));
	
	sendPacket(SessionKeyExchange, request, node);
}

void LinkLayer::handleSessionKeyExchange(QByteArray &payload, SparkleNode* node) {
	if(!checkPacketSize(payload, sizeof(key_exchange_t), node, "SessionKeyExchange", PacketSizeGreater))
		return;

	const key_exchange_t *ke = (const key_exchange_t*) payload.constData();

	QByteArray key = payload.mid(sizeof(key_exchange_t));
	node->setHisSessionKey(key);

	Log::debug("link: stored session key for [%1]:%2") << *node;
	
	if(ke->needOthersKey) {
		sendSessionKeyExchange(node, false);
	} else {
		node->negotiationFinished();
		awaitingNegotiation.removeOne(node);

		while(!node->isQueueEmpty())
			encryptAndSend(node->popQueue(), node);
		
		if(awaitingNegotiation.count() == 0 && preparingForShutdown)
			emit readyForShutdown();
	}
}

/* IntroducePacket */

void LinkLayer::sendIntroducePacket(SparkleNode* node) {
	introduce_packet_t intr;
	intr.sparkleIP = router.getSelfNode()->getSparkleIP().toIPv4Address();
	memcpy(intr.sparkleMAC, router.getSelfNode()->getSparkleMAC().constData(), 6);

	sendEncryptedPacket(IntroducePacket, QByteArray((const char*) &intr, sizeof(introduce_packet_t)), node);
}

void LinkLayer::handleIntroducePacket(QByteArray &payload, SparkleNode* node) {
	if(!checkPacketSize(payload, sizeof(introduce_packet_t), node, "IntroducePacket"))
		return;
	
	if(node->getSparkleMAC().length() > 0 || router.getNodes().contains(node)) {
		Log::warn("link: node [%2]:%3 is already introduced as %1") << node->getSparkleIP() << *node;
		return;
	}
	
	const introduce_packet_t *intr = (const introduce_packet_t*) payload.constData();

	node->setSparkleIP(QHostAddress(intr->sparkleIP));
	node->setSparkleMAC(QByteArray((const char*) intr->sparkleMAC, 6));
	node->setMaster(false);
	
	router.updateNode(node);

	Log::debug("link: node [%1]:%2 introduced itself as %3") << *node << node->getSparkleIP();
}

/* MasterNodeRequest */

void LinkLayer::sendMasterNodeRequest(SparkleNode* node) {
	sendEncryptedPacket(MasterNodeRequest, QByteArray(), node);
}

void LinkLayer::handleMasterNodeRequest(QByteArray &payload, SparkleNode* node) {
	if(!checkPacketSize(payload, 0, node, "MasterNodeRequest"))
		return;
	
	// scatter load over the whole network
	SparkleNode* master = router.selectMaster();
	
	if(master == NULL)
		Log::fatal("link: cannot choose master, this is probably a bug");
	
	sendMasterNodeReply(node, master);
}

/* MasterNodeReply */

void LinkLayer::sendMasterNodeReply(SparkleNode* node, SparkleNode* masterNode) {
	master_node_reply_t reply;
	reply.addr = masterNode->getRealIP().toIPv4Address();
	reply.port = masterNode->getRealPort();
	
	sendEncryptedPacket(MasterNodeReply, QByteArray((const char*) &reply, sizeof(master_node_reply_t)), node);
}

void LinkLayer::handleMasterNodeReply(QByteArray &payload, SparkleNode* node) {
	if(!checkPacketSize(payload, sizeof(master_node_reply_t), node, "MasterNodeReply"))
		return;

	if(!checkPacketExpection(node, "MasterNodeReply", JoinMasterNodeRequest))
		return;
	
	const master_node_reply_t *reply = (const master_node_reply_t*) payload.constData();
	
	SparkleNode* master = wrapNode(QHostAddress(reply->addr), reply->port);
	joinMaster = master;
	
	Log::debug("link: determined master node: [%1]:%2") << *master;
	
	if(!forceBehindNAT) {
		joinStep = JoinAwaitingPings;
		joinPing.addr = 0;
		joinPingsEmitted = 4;
		joinPingsArrived = 0;
		pingTimer->start();
		sendPingRequest(node, master, 4);
	} else {
		Log::debug("link: skipping NAT detection");
		
		joinStep = JoinRegistration;
		sendRegisterRequest(master, true);
	}
	
	joinTimer->start();
}

/* PingRequest */

void LinkLayer::sendPingRequest(SparkleNode* node, SparkleNode* target, int count) {
	ping_request_t req;
	req.count = count;
	req.addr = target->getRealIP().toIPv4Address();
	req.port = target->getRealPort();
	
	sendEncryptedPacket(PingRequest, QByteArray((const char*) &req, sizeof(ping_request_t)), node);
}

void LinkLayer::handlePingRequest(QByteArray &payload, SparkleNode* node) {
	if(!checkPacketSize(payload, sizeof(ping_request_t), node, "PingRequest"))
		return;
	
	const ping_request_t *req = (const ping_request_t*) payload.constData();

	SparkleNode* target = wrapNode(QHostAddress(req->addr), req->port);
	
	if(*router.getSelfNode() == *target) {
		doPing(node, req->count);
		return;
	}
	
	sendPingInitiate(target, node, req->count);
}

/* PingInitiate */

void LinkLayer::sendPingInitiate(SparkleNode* node, SparkleNode* target, int count) {
	ping_request_t req;
	req.count = count;
	req.addr = target->getRealIP().toIPv4Address();
	req.port = target->getRealPort();
	
	sendEncryptedPacket(PingInitiate, QByteArray((const char*) &req, sizeof(ping_request_t)), node);
}

void LinkLayer::handlePingInitiate(QByteArray &payload, SparkleNode* node) {
	if(!checkPacketSize(payload, sizeof(ping_request_t), node, "PingInitiate"))
		return;
	
	const ping_request_t *req = (const ping_request_t*) payload.constData();
	
	doPing(wrapNode(QHostAddress(req->addr), req->port), req->count);
}

void LinkLayer::doPing(SparkleNode* node, quint8 count) {
	if(count > 16) {
		Log::warn("link: request for many (%1) ping's for [%2]:%3. DoS attempt? Dropping.") << count << *node;
		return;
	}
	
	for(int i = 0; i < count; i++)
		sendPing(node);
}

/* Ping */

void LinkLayer::sendPing(SparkleNode* node) {
	ping_t ping;
	ping.addr = node->getRealIP().toIPv4Address();
	ping.port = node->getRealPort();
	
	sendPacket(Ping, QByteArray((const char*) &ping, sizeof(ping_t)), node);
}

void LinkLayer::handlePing(QByteArray &payload, SparkleNode* node) {
	if(!checkPacketSize(payload, sizeof(ping_t), node, "Ping"))
		return;
	
	if(!checkPacketExpection(node, "Ping", JoinAwaitingPings))
		return;
	
	if(node != joinMaster) {
		Log::warn("link: unexpected ping from node [%1]:%2") << *node;
		return;
	}
	
	const ping_t* ping = (const ping_t*) payload.constData();
	
	joinPingsArrived++;
	if(joinPing.addr == 0) {
		joinPing = *ping;
	} else if(joinPing.addr != ping->addr || joinPing.port != ping->port) {
		Log::error("link: got nonidentical pings");

		revertJoin();

		emit joinFailed();
	}
	
	if(joinPingsArrived == joinPingsEmitted)
		joinGotPinged();
}

void LinkLayer::pingTimeout() {
	if(joinPingsArrived == 0) {
		Log::debug("link: no pings arrived, NAT is detected");
		
		joinStep = JoinRegistration;
		
		Log::debug("link: registering on [%1]:%2") << *joinMaster;
		sendRegisterRequest(joinMaster, true);
	} else {
		joinGotPinged();
	}
}

void LinkLayer::joinGotPinged() {
	Log::debug("link: %1% of pings arrived") << (joinPingsArrived * 100 / joinPingsEmitted);
	
	pingTimer->stop();
	
	joinStep = JoinRegistration;
	
	Log::debug("link: no NAT detected, my real address is [%1]:%2")
				<< QHostAddress(joinPing.addr) << joinPing.port;
	
	Log::debug("link: registering on [%1]:%2") << *joinMaster;
	sendRegisterRequest(joinMaster, false);
	
	joinTimer->start();
}

/* RegisterRequest */

void LinkLayer::sendRegisterRequest(SparkleNode* node, bool isBehindNAT) {
	register_request_t req;
	req.isBehindNAT = isBehindNAT;
	
	sendEncryptedPacket(RegisterRequest, QByteArray((const char*) &req, sizeof(register_request_t)), node);
}

void LinkLayer::handleRegisterRequest(QByteArray &payload, SparkleNode* node) {
	if(!checkPacketSize(payload, sizeof(register_request_t), node, "RegisterRequest"))
		return;

	if(!router.getSelfNode()->isMaster()) {
		Log::warn("link: got RegisterRequest while not master");
		return;
	}

	const register_request_t* req = (const register_request_t*) payload.constData();

	node->configureByKey();
	node->setBehindNAT(req->isBehindNAT);

	if(!node->isBehindNAT()) {
		if(router.getMasters().count() == 1) {
			node->setMaster(true);
		} else {
			double ik = 1. / networkDivisor;
			double rk = ((double) router.getMasters().count()) / (router.getNodes().count() + 1);
			if(rk < ik) {
				Log::debug("link: insufficient masters (I %1; R %2), adding one") << ik << rk;
				node->setMaster(true);
			} else {
				node->setMaster(false);
			}
		}
	} else {
		node->setMaster(false);
	}

	QList<SparkleNode*> updates;
	if(node->isMaster())	updates = router.getOtherNodes();
	else			updates = router.getOtherMasters();

	foreach(SparkleNode* update, updates) {
		sendRoute(node, update);
		sendRoute(update, node);
	}
	
	sendRoute(node, router.getSelfNode());

	router.updateNode(node);

	sendRegisterReply(node);
}

/* RegisterReply */

void LinkLayer::sendRegisterReply(SparkleNode* node) {
	register_reply_t reply;
	reply.isMaster = node->isMaster();
	reply.networkDivisor = networkDivisor;
	reply.sparkleIP = node->getSparkleIP().toIPv4Address();
	if(node->isBehindNAT()) {
		reply.realIP = node->getRealIP().toIPv4Address();
		reply.realPort = node->getRealPort();
	} else {
		reply.realIP = reply.realPort = 0;
	}
	
	Q_ASSERT(node->getSparkleMAC().length() == 6);
	memcpy(reply.sparkleMAC, node->getSparkleMAC().constData(), 6);
	
	sendEncryptedPacket(RegisterReply, QByteArray((const char*) &reply, sizeof(register_reply_t)), node);
}

void LinkLayer::handleRegisterReply(QByteArray &payload, SparkleNode* node) {
	if(!checkPacketSize(payload, sizeof(register_reply_t), node, "RegisterReply"))
		return;
	
	if(!checkPacketExpection(node, "RegisterReply", JoinRegistration))
		return;
	
	const register_reply_t* reply = (const register_reply_t*) payload.constData();

	SparkleNode* self;
	if(reply->realIP != 0) { // i am behind NAT
		Log::debug("link: external endpoint was assigned by NAT passthrough");
		self = wrapNode(QHostAddress(reply->realIP), reply->realPort);
		self->setBehindNAT(true);
	} else {
		self = wrapNode(QHostAddress(joinPing.addr), joinPing.port);
		self->setBehindNAT(false);
	}
	self->setSparkleIP(QHostAddress(reply->sparkleIP));
	self->setSparkleMAC(QByteArray((const char*) reply->sparkleMAC, 6));
	self->setAuthKey(hostKeyPair);
	self->setMaster(reply->isMaster);
	router.setSelfNode(self);
	
	networkDivisor = reply->networkDivisor;
	Log::debug("link: network divisor is 1/%1") << networkDivisor;
	
	joinTimer->stop();
	
	joinStep = JoinFinished;
	emit joined(self);
}

/* Route */

void LinkLayer::sendRoute(SparkleNode* node, SparkleNode* target)
{
	route_t route;
	route.realIP = target->getRealIP().toIPv4Address();
	route.realPort = target->getRealPort();
	route.sparkleIP = target->getSparkleIP().toIPv4Address();
	route.isMaster = target->isMaster();
	route.isBehindNAT = target->isBehindNAT();

	Q_ASSERT(node->getSparkleMAC().length() == 6);
	memcpy(route.sparkleMAC, target->getSparkleMAC().constData(), 6);
	
	sendEncryptedPacket(Route, QByteArray((const char*) &route, sizeof(route_t)), node);
}

void LinkLayer::handleRoute(QByteArray &payload, SparkleNode* node) {
	if(!checkPacketSize(payload, sizeof(route_t), node, "Route"))
		return;

	if(!node->isMaster() && router.getSelfNode() != NULL) {
		Log::warn("link: Route packet from unauthoritative source [%1]:%2") << *node;
		return;
	}

	const route_t* route = (const route_t*) payload.constData();
	
	SparkleNode* target = wrapNode(QHostAddress(route->realIP), route->realPort);
	if(target == router.getSelfNode()) {
		Log::warn("link: attempt to add myself by Route packet from [%1]:%2") << *node;
		return;
	}
	
	Log::debug("link: Route received from [%1]:%2") << *node;
	
	target->setSparkleIP(QHostAddress(route->sparkleIP));
	target->setSparkleMAC(QByteArray((const char*) route->sparkleMAC, 6));
	target->setMaster(route->isMaster);
	target->setBehindNAT(route->isBehindNAT);
	
	router.updateNode(target);
}

/* RouteRequest */

void LinkLayer::sendRouteRequest(QHostAddress host) {
	route_request_t req;
	req.sparkleIP = host.toIPv4Address();
	
	sendEncryptedPacket(RouteRequest, QByteArray((const char*) &req, sizeof(route_request_t)), 
				router.selectMaster());
}

void LinkLayer::handleRouteRequest(QByteArray &payload, SparkleNode* node) {
	if(!checkPacketSize(payload, sizeof(route_request_t), node, "RouteRequest"))
		return;
	
	if(!router.getSelfNode()->isMaster()) {
		Log::warn("link: i'm slave and got route request from [%1]:%2") << *node;
		return;
	}

	const route_request_t* req = (const route_request_t*) payload.constData();
	QHostAddress host(req->sparkleIP);
	
	SparkleNode* target = router.searchSparkleNode(host);
	if(target) {
		sendRoute(node, target);
	} else {
		sendRouteMissing(node, host);
	}
}

/* RouteMissing */

void LinkLayer::sendRouteMissing(SparkleNode* node, QHostAddress host) {
	route_missing_t req;
	req.sparkleIP = host.toIPv4Address();
	
	sendEncryptedPacket(RouteMissing, QByteArray((const char*) &req, sizeof(route_request_t)), node);
}

void LinkLayer::handleRouteMissing(QByteArray &payload, SparkleNode* node) {
	if(!checkPacketSize(payload, sizeof(route_missing_t), node, "RouteMissing"))
		return;
	
	const route_missing_t* req = (const route_missing_t*) payload.constData();
	QHostAddress host(req->sparkleIP);
	
	Log::info("link: no route to %1") << host;
}

/* RouteInvalidate */

void LinkLayer::sendRouteInvalidate(SparkleNode* node, SparkleNode* target) {
	route_invalidate_t inv;
	inv.realIP = target->getRealIP().toIPv4Address();
	inv.realPort = target->getRealPort();
	
	sendEncryptedPacket(RouteInvalidate, QByteArray((const char*) &inv, sizeof(route_invalidate_t)), node);
}

void LinkLayer::handleRouteInvalidate(QByteArray& payload, SparkleNode* node) {
	if(!checkPacketSize(payload, sizeof(route_invalidate_t), node, "RouteInvalidate"))
		return;
	
	const route_invalidate_t* inv = (const route_invalidate_t*) payload.constData();
	SparkleNode* target = wrapNode(QHostAddress(inv->realIP), inv->realPort);
	
	Log::debug("link: invalidating route %5 @ [%1]:%2 because of command from [%3]:%4")
			<< *target << *node << node->getSparkleIP();
	
	router.removeNode(target);
	nodeSpool.removeOne(target);
	delete target;
}

/* RoleUpdate */

void LinkLayer::sendRoleUpdate(SparkleNode* node, bool isMasterNow) {
	role_update_t update;
	update.isMasterNow = isMasterNow;
	
	sendEncryptedPacket(RoleUpdate, QByteArray((const char*) &update, sizeof(role_update_t)), node);
}

void LinkLayer::handleRoleUpdate(QByteArray& payload, SparkleNode* node) {
	if(!checkPacketSize(payload, sizeof(role_update_t), node, "RoleUpdate"))
		return;
	
	if(!node->isMaster()) {
		Log::warn("link: RoleUpdate packet was received from slave [%1]:%2, dropping") << *node;
		return;
	}

	const role_update_t* update = (const role_update_t*) payload.constData();

	Log::info("link: switching to %3 role caused by [%1]:%2") << *node
		<< (update->isMasterNow ? "Master" : "Slave");
	
	router.getSelfNode()->setMaster(update->isMasterNow);
}

/* ExitNotification */

void LinkLayer::sendExitNotification(SparkleNode* node) {
	sendEncryptedPacket(ExitNotification, QByteArray(), node);
}

void LinkLayer::handleExitNotification(QByteArray& payload, SparkleNode* node) {
	if(!checkPacketSize(payload, 0, node, "ExitNotification"))
		return;
	
	if(!router.getSelfNode()->isMaster()) {
		Log::warn("link: ExitNotification was received from [%1]:%2, but I am slave") << *node;
		return;
	}

	router.removeNode(node);

	foreach(SparkleNode* target, router.getOtherNodes())	
		sendRouteInvalidate(target, node);

	nodeSpool.removeOne(node);
	delete node;

	double ik = 1. / networkDivisor;
	double rk = ((double) router.getMasters().count()) / (router.getNodes().count());
	if(rk < ik || router.getMasters().count() == 1) {
		Log::debug("link: insufficient masters (I %1; R %2)") << ik << rk;
		
		reincarnateSomeone();
	}
}

void LinkLayer::reincarnateSomeone() {
	if(router.getNodes().count() == 1) {
		Log::warn("link: there're no nodes to reincarnate");
		return;
	}

	SparkleNode* target = router.selectWhiteSlave();
	Log::debug("link: %1 @ [%2]:%3 is selected as target") << target->getSparkleIP() << *target;
	
	target->setMaster(true);
	
	router.updateNode(target);
	
	foreach(SparkleNode* node, router.getOtherNodes()) {
		if(!node->isMaster() && node != target) {
			sendRoute(node, target);
			sendRoute(target, node);
		}
	}
	
	sendRoleUpdate(target, true);
}

/* ======= END ======= */

void LinkLayer::sendDataToNode(QByteArray packet, SparkleNode *node) {
	sendEncryptedPacket(DataPacket, packet, node);
}

void LinkLayer::revertJoin() {
	foreach(SparkleNode *node, nodeSpool)
		delete node;

	router.clear();
	nodeSpool.clear();
	awaitingNegotiation.clear();
	cookies.clear();
}

