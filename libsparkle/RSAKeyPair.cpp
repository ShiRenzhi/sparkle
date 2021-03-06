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

#include <Sparkle/RSAKeyPair>
#include <Sparkle/Log>

#include <QFile>

#include <stdio.h>
#include <stdlib.h>

#include "crypto/rsa.h"

#include "SparkleRandom.h"

using namespace Sparkle;

QDataStream & operator<< (QDataStream& stream, const mpi &data);
QDataStream & operator>> (QDataStream& stream, mpi &data);

namespace Sparkle {

class RSAKeyPairPrivate {
public:
	enum {
		KeyMagic = 0x424D5049	// 'BMPI' (Binary MPI)
	};

	RSAKeyPairPrivate() {
		rsa_init(&key, RSA_PKCS_V15, 0, SparkleRandom::integer, NULL);	
	}
	
	virtual ~RSAKeyPairPrivate() {		
		rsa_free(&key);
	}

	rsa_context key;
};

}

RSAKeyPair::RSAKeyPair() : d_ptr(new RSAKeyPairPrivate) {

}

RSAKeyPair::RSAKeyPair(RSAKeyPairPrivate &dd) : d_ptr(&dd) {

}

RSAKeyPair::~RSAKeyPair() {
	delete d_ptr;
}

bool RSAKeyPair::generate(int bits) {
	Q_D(RSAKeyPair);

	return rsa_gen_key(&d->key, bits, 65537) == 0;
}

bool RSAKeyPair::writeToFile(QString filename) const {
	Q_D(const RSAKeyPair);

	QByteArray rawKey;

	QDataStream stream(&rawKey, QIODevice::WriteOnly);

	stream << (quint32) RSAKeyPairPrivate::KeyMagic;
	stream << d->key.ver;
	stream << d->key.len;
	stream << d->key.N;
	stream << d->key.E;
	stream << d->key.D;
	stream << d->key.P;
	stream << d->key.Q;
	stream << d->key.DP;
	stream << d->key.DQ;
	stream << d->key.QP;
	stream << d->key.RN;
	stream << d->key.RP;
	stream << d->key.RQ;

	QFile keyFile(filename);
	if(!keyFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return false;
	}
	

	keyFile.write(rawKey.toBase64());

	keyFile.close();

	return true;
}

bool RSAKeyPair::readFromFile(QString filename) {
	Q_D(RSAKeyPair);

	QFile keyFile(filename);

	if(!keyFile.open(QIODevice::ReadOnly)) {
		return false;
	}

	QByteArray rawdata = keyFile.readAll();

	keyFile.close();

	if(rawdata.left(10) == "-----BEGIN") {
		Log::error("Your private key is in wrong format, re-generate it");
		
		return false;
	}

	QByteArray data = QByteArray::fromBase64(rawdata);

	QDataStream stream(&data, QIODevice::ReadOnly);

	quint32 magic;

	stream >> magic;
	
	if(magic != RSAKeyPairPrivate::KeyMagic) {
		Log::error("RSAKeyPair::readFromFile: bad RSA key magic: %1") << magic;
		
		return false;
	}

	stream >> d->key.ver;
	stream >> d->key.len;
	stream >> d->key.N;
	stream >> d->key.E;
	stream >> d->key.D;
	stream >> d->key.P;
	stream >> d->key.Q;
	stream >> d->key.DP;
	stream >> d->key.DQ;
	stream >> d->key.QP;
	stream >> d->key.RN;
	stream >> d->key.RP;
	stream >> d->key.RQ;

	return true;
}

QByteArray RSAKeyPair::publicKey() const {
	Q_D(const RSAKeyPair);
	
	QByteArray rawKey;

	QDataStream stream(&rawKey, QIODevice::WriteOnly);
	
	stream << (quint32) RSAKeyPairPrivate::KeyMagic;
	stream << d->key.ver;
	stream << d->key.len;
	stream << d->key.N;
	stream << d->key.E;
	stream << d->key.RN;

	return rawKey;
}

bool RSAKeyPair::setPublicKey(QByteArray data) {
	Q_D(RSAKeyPair);
	
	QDataStream stream(&data, QIODevice::ReadOnly);

	quint32 magic;

	stream >> magic;
	
	if(magic != RSAKeyPairPrivate::KeyMagic) {
		Log::error("RSAKeyPair::setPublicKey: bad RSA key magic: %1") << magic;
		
		return false;
	}
	
	stream >> d->key.ver;
	stream >> d->key.len;
	stream >> d->key.N;
	stream >> d->key.E;
	stream >> d->key.RN;

	return true;
}

QByteArray RSAKeyPair::encrypt(QByteArray plaintext) {
	Q_D(RSAKeyPair);

	QByteArray output;

	int rsize = d->key.len;

	int flen = rsize - 11;
	unsigned char *chunk = new unsigned char[rsize];

	for(; plaintext.size() > 0; ) {
		int dlen = qMin<int>(flen, plaintext.length());

		rsa_pkcs1_encrypt(&d->key, RSA_PUBLIC, dlen, (unsigned char *) plaintext.data(), chunk);

		output += QByteArray((char *) chunk, rsize);

		plaintext = plaintext.right(plaintext.size() - dlen);
	}

	delete chunk;

	return output;
}

QByteArray RSAKeyPair::decrypt(QByteArray cryptotext) {
	Q_D(RSAKeyPair);

	QByteArray output;

	int rsize = d->key.len;

	unsigned char *chunk = new unsigned char[rsize];

	for(; cryptotext.size() > 0; ) {
		int dlen = qMin<int>(rsize, cryptotext.size());

		int dec;

		rsa_pkcs1_decrypt(&d->key, RSA_PRIVATE, &dec, (unsigned char *) cryptotext.data(), chunk, dlen);

		output += QByteArray((char *) chunk, dec);

		cryptotext = cryptotext.right(cryptotext.size() - rsize);
	}
	
	delete chunk;

	return output;
}

QDataStream & operator<< (QDataStream& stream, const mpi &data) {
	QByteArray raw;
	
	mpi *mp = (mpi *) &data;
		
	if(mp && mp->p != NULL) {
		raw.resize(mpi_size(mp));
		
		mpi_write_binary(mp, (unsigned char *) raw.data(), raw.size());
	}
	
	stream << raw;

	return stream;
}

QDataStream & operator >> (QDataStream& stream, mpi &data) {
	QByteArray raw;

	stream >> raw;
	
	if(raw.size() == 0) {
		mpi_free(&data, NULL);
	} else
		mpi_read_binary(&data, (unsigned char *) raw.data(), raw.size());

	return stream;
}
