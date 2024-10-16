/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2023 Uniot <contact@uniot.io>
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

#pragma once

#include <Arduino.h>
#include <Bytes.h>
#include <Ed25519.h>
#include <Logger.h>

#include "CBORObject.h"
#include "COSE.h"
#include "ICOSESigner.h"

namespace uniot {
class COSEMessage {
 public:
  COSEMessage(COSEMessage const &) = delete;
  void operator=(COSEMessage const &) = delete;

  COSEMessage()
      : mpProtectedHeader(nullptr),
        mpUnprotectedHeader(nullptr),
        mpPayload(nullptr),
        mpSignature(nullptr),
        mReadSuccess(false) {
    _create();
  }

  COSEMessage(Bytes buf)
      : mpProtectedHeader(nullptr),
        mpUnprotectedHeader(nullptr),
        mpPayload(nullptr),
        mpSignature(nullptr) {
    mReadSuccess = _read(buf);
  }

  virtual ~COSEMessage() {
    _clean();
  }

  bool read(const Bytes &buf) {
    _clean();
    mReadSuccess = _read(buf);
    return mReadSuccess;
  }

  inline bool wasReadSuccessful() const {
    return mReadSuccess;
  }

  inline Bytes getProtectedHeader() {
    return mRoot._getBytes(mpProtectedHeader);
  }

  inline CBORObject getUnprotectedHeader() {
    return mRoot._getMap(mpUnprotectedHeader);
  }

  inline Bytes getUnprotectedKid() {
    return getUnprotectedHeader().getBytes(COSEHeaderLabel::KeyIdentifier);
  }

  inline Bytes getPayload() {
    return mRoot._getBytes(mpPayload);
  }

  inline Bytes getSignature() {
    return mRoot._getBytes(mpSignature);
  }

  inline bool isSigned() {
    auto signature = getSignature();
    CBORObject pHeader(getProtectedHeader());
    auto alg = pHeader.getInt(COSEHeaderLabel::Algorithm);

    return alg != 0 && signature.size() > 0;
  }

  void setUnprotectedKid(const Bytes& kid) {
    getUnprotectedHeader().put(COSEHeaderLabel::KeyIdentifier, kid.raw(), kid.size());
  }

  bool setPayload(const Bytes &payload) {
    mRawPayload = payload;
    return cn_cbor_data_update(mpPayload, mRawPayload.raw(), mRawPayload.size());
  }

  void sign(const ICOSESigner &signer, const Bytes &external = Bytes()) {
    auto alg = signer.signerAlgorithm();
    if (alg != COSEAlgorithm::EdDSA) {
      UNIOT_LOG_ERROR("sign failed: alg '%d' is not supported", alg);
      return;
    }

    CBORObject pHeader;
    pHeader.put(COSEHeaderLabel::Algorithm, alg);
    _setProtectedHeader(pHeader);

    auto toSign = _toBeSigned(external);
    auto signature = signer.sign(toSign);
    _setSignature(signature);
  }

  bool verify(const Bytes &publicKey) {
    CBORObject pHeader(getProtectedHeader());
    auto alg = pHeader.getInt(COSEHeaderLabel::Algorithm);
    if (alg != COSEAlgorithm::EdDSA) {
      UNIOT_LOG_ERROR("verify failed: alg '%d' is not supported", alg);
      return false;
    }

    auto toVerify = _toBeSigned();
    auto signature = getSignature();

    switch (alg) {
      case COSEAlgorithm::EdDSA:
        return Ed25519::verify(signature.raw(), publicKey.raw(), toVerify.raw(), toVerify.size());
      default:
        return false;
    }
  }

  Bytes build() const {
    return mRoot.build();
  }

  void clean() {
    _clean();
    _create();
  }

 private:
  void _create() {
    mRoot._clean();
    auto root = cn_cbor_array_create(mRoot._errback());
    cn_cbor_array_append(root, mpProtectedHeader = cn_cbor_data_create(nullptr, 0, mRoot._errback()), mRoot._errback());
    cn_cbor_array_append(root, mpUnprotectedHeader = cn_cbor_map_create(mRoot._errback()), mRoot._errback());
    cn_cbor_array_append(root, mpPayload = cn_cbor_data_create(nullptr, 0, mRoot._errback()), mRoot._errback());
    cn_cbor_array_append(root, mpSignature = cn_cbor_data_create(nullptr, 0, mRoot._errback()), mRoot._errback());
    mRoot.mpMapNode = cn_cbor_tag_create(COSETag::Sign1, root, mRoot._errback());
  }

  bool _read(const Bytes &buf) {
    mRoot.read(buf);

    if (mRoot.mErr.err != CN_CBOR_NO_ERROR) {
      UNIOT_LOG_ERROR("read failed: %s", cn_cbor_error_str[mRoot.mErr.err]);
      return false;
    }
    if (mRoot.mpMapNode->type != CN_CBOR_TAG) {
      UNIOT_LOG_ERROR("read failed: there is no tag");
      return false;
    }
    if (mRoot.mpMapNode->v.sint != COSETag::Sign1) {
      UNIOT_LOG_ERROR("read failed: tag is not 18 (COSE_Sign1)");
      return false;
    }
    auto rootArray = mRoot.mpMapNode->first_child;
    if (!rootArray) {
      UNIOT_LOG_ERROR("read failed: root not found");
      return false;
    }
    if (rootArray->type != CN_CBOR_ARRAY) {
      UNIOT_LOG_ERROR("read failed: root is not an array");
      return false;
    }
    auto protectedHeader = cn_cbor_index(rootArray, 0);
    if (!protectedHeader) {
      UNIOT_LOG_ERROR("read failed: protectedHeader not found");
      return false;
    }
    if (protectedHeader->type != CN_CBOR_BYTES) {
      UNIOT_LOG_ERROR("read failed: protectedHeader is not bytes");
      return false;
    }
    auto unprotectedHeader = cn_cbor_index(rootArray, 1);
    if (!unprotectedHeader) {
      UNIOT_LOG_ERROR("read failed: unprotectedHeader not found");
      return false;
    }
    if (unprotectedHeader->type != CN_CBOR_MAP) {
      UNIOT_LOG_ERROR("read failed: unprotectedHeader is not map");
      return false;
    }
    auto payload = cn_cbor_index(rootArray, 2);
    if (!payload) {
      UNIOT_LOG_ERROR("read failed: payload not found");
      return false;
    }
    if (payload->type != CN_CBOR_BYTES) {
      UNIOT_LOG_ERROR("read failed: payload is not bytes");
      return false;
    }
    auto signature = cn_cbor_index(rootArray, 3);
    if (!signature) {
      UNIOT_LOG_ERROR("read failed: signature not found");
      return false;
    }
    if (signature->type != CN_CBOR_BYTES) {
      UNIOT_LOG_ERROR("read failed: signature is not bytes");
      return false;
    }

    mpProtectedHeader = protectedHeader;
    mpUnprotectedHeader = unprotectedHeader;
    mpPayload = payload;
    mpSignature = signature;

    return true;
  }

  void _clean() {
    mRoot._clean();
    mpProtectedHeader = nullptr;
    mpUnprotectedHeader = nullptr;
    mpPayload = nullptr;
    mpSignature = nullptr;

    mRawProtectedHeader.clean();
    mRawPayload.clean();
    mRawSignature.clean();
  }

  bool _setProtectedHeader(const CBORObject &pHeader) {
    mRawProtectedHeader = pHeader.build();
    return cn_cbor_data_update(mpProtectedHeader, mRawProtectedHeader.raw(), mRawProtectedHeader.size());
  }

  bool _setSignature(const Bytes &signature) {
    mRawSignature = signature;
    return cn_cbor_data_update(mpSignature, mRawSignature.raw(), mRawSignature.size());
  }

  Bytes _toBeSigned(const Bytes &external = Bytes()) {
    auto protectedHeader = getProtectedHeader();
    auto payload = getPayload();

    auto sigStruct = cn_cbor_array_create(mRoot._errback());
    cn_cbor_array_append(sigStruct, cn_cbor_string_create("Signature1", mRoot._errback()), mRoot._errback());                                 // context
    cn_cbor_array_append(sigStruct, cn_cbor_data_create(protectedHeader.raw(), protectedHeader.size(), mRoot._errback()), mRoot._errback());  // body_protected
    cn_cbor_array_append(sigStruct, cn_cbor_data_create(external.raw(), external.size(), mRoot._errback()), mRoot._errback());                // external_aad
    cn_cbor_array_append(sigStruct, cn_cbor_data_create(payload.raw(), payload.size(), mRoot._errback()), mRoot._errback());                  // payload
    auto rawSigStruct = mRoot._build(sigStruct);
    cn_cbor_free(sigStruct);

    return rawSigStruct;
  }

  CBORObject mRoot;
  cn_cbor *mpProtectedHeader;
  cn_cbor *mpUnprotectedHeader;
  cn_cbor *mpPayload;
  cn_cbor *mpSignature;

  Bytes mRawProtectedHeader;
  Bytes mRawPayload;
  Bytes mRawSignature;
  bool mReadSuccess;
};
}  // namespace uniot
