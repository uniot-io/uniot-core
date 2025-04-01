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
/**
 * @brief Implementation of COSE_Sign1 message format as specified in RFC 8152
 * @ingroup utils_cose
 * @{
 *
 * This class provides functionality to create, read, sign, and verify COSE_Sign1 messages.
 * It implements the message structure defined in section 4.2 of RFC 8152.
 * The class supports EdDSA signatures (algorithm -8).
 */
class COSEMessage {
 public:
  /** @brief Copy constructor disabled */
  COSEMessage(COSEMessage const &) = delete;
  /** @brief Assignment operator disabled */
  void operator=(COSEMessage const &) = delete;

  /**
   * @brief Default constructor
   *
   * Creates an empty COSE_Sign1 message with default structure
   */
  COSEMessage()
      : mpProtectedHeader(nullptr),
        mpUnprotectedHeader(nullptr),
        mpPayload(nullptr),
        mpSignature(nullptr),
        mReadSuccess(false) {
    _create();
  }

  /**
   * @brief Constructs a message from existing CBOR data
   *
   * @param buf CBOR-encoded COSE_Sign1 message
   */
  COSEMessage(Bytes buf)
      : mpProtectedHeader(nullptr),
        mpUnprotectedHeader(nullptr),
        mpPayload(nullptr),
        mpSignature(nullptr) {
    mReadSuccess = _read(buf);
  }

  /** @brief Destructor */
  virtual ~COSEMessage() {
    _clean();
  }

  /**
   * @brief Reads a CBOR-encoded COSE_Sign1 message
   *
   * @param buf CBOR data to parse
   * @retval true The message was read successfully
   * @retval false The message was not read successfully
   */
  bool read(const Bytes &buf) {
    _clean();
    mReadSuccess = _read(buf);
    return mReadSuccess;
  }

  /**
   * @brief Checks if the message was read successfully
   *
   * @retval true The read operation was successful
   * @retval false The read operation failed
   */
  inline bool wasReadSuccessful() const {
    return mReadSuccess;
  }

  /**
   * @brief Gets the protected header as a byte string
   *
   * @retval Bytes Contains the CBOR-encoded protected header
   */
  inline Bytes getProtectedHeader() {
    return mRoot._getBytes(mpProtectedHeader);
  }

  /**
   * @brief Gets the unprotected header as a CBORObject
   *
   * @retval CBORObject Represents the unprotected header
   */
  inline CBORObject getUnprotectedHeader() {
    return mRoot._getMap(mpUnprotectedHeader);
  }

  /**
   * @brief Gets the key identifier from the unprotected header
   *
   * @retval Bytes Contains the key identifier (kid)
   * @retval Bytes Empty if the key identifier is not present
   */
  inline Bytes getUnprotectedKid() {
    return getUnprotectedHeader().getBytes(COSEHeaderLabel::KeyIdentifier);
  }

  /**
   * @brief Gets the payload of the message
   *
   * @retval Bytes Contains the payload data
   */
  inline Bytes getPayload() {
    return mRoot._getBytes(mpPayload);
  }

  /**
   * @brief Gets the signature of the message
   *
   * @retval Bytes Contains the signature
   */
  inline Bytes getSignature() {
    return mRoot._getBytes(mpSignature);
  }

  /**
   * @brief Checks if the message has a valid signature structure
   *
   * @retval true The message is signed (has an algorithm and non-empty signature)
   * @retval false The message is not signed
   */
  inline bool isSigned() {
    auto signature = getSignature();
    CBORObject pHeader(getProtectedHeader());
    auto alg = pHeader.getInt(COSEHeaderLabel::Algorithm);

    return alg != 0 && signature.size() > 0;
  }

  /**
   * @brief Sets the key identifier in the unprotected header
   *
   * @param kid Key identifier to set
   */
  void setUnprotectedKid(const Bytes& kid) {
    getUnprotectedHeader().put(COSEHeaderLabel::KeyIdentifier, kid.raw(), kid.size());
  }

  /**
   * @brief Sets the payload of the message
   *
   * @param payload Data to set as the payload
   * @retval true The payload was set successfully
   * @retval false The payload was not set successfully
   */
  bool setPayload(const Bytes &payload) {
    mRawPayload = payload;
    return cn_cbor_data_update(mpPayload, mRawPayload.raw(), mRawPayload.size());
  }

  /**
   * @brief Signs the message using the provided signer
   *
   * @param signer Object implementing the ICOSESigner interface that performs the signature
   * @param external Optional external data to include in the signature calculation (AAD)
   */
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

  /**
   * @brief Verifies the signature of the message using the provided public key
   *
   * @param publicKey Public key to use for verification
   * @retval true The signature is valid
   * @retval false The signature is invalid
   */
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

  /**
   * @brief Builds the CBOR representation of the message
   *
   * @retval Bytes Contains the CBOR-encoded message
   */
  Bytes build() const {
    return mRoot.build();
  }

  /**
   * @brief Resets the message to its initial empty state
   */
  void clean() {
    _clean();
    _create();
  }

 private:
  /**
   * @brief Creates the initial COSE_Sign1 structure
   */
  void _create() {
    mRoot._clean();
    auto root = cn_cbor_array_create(mRoot._errback());
    cn_cbor_array_append(root, mpProtectedHeader = cn_cbor_data_create(nullptr, 0, mRoot._errback()), mRoot._errback());
    cn_cbor_array_append(root, mpUnprotectedHeader = cn_cbor_map_create(mRoot._errback()), mRoot._errback());
    cn_cbor_array_append(root, mpPayload = cn_cbor_data_create(nullptr, 0, mRoot._errback()), mRoot._errback());
    cn_cbor_array_append(root, mpSignature = cn_cbor_data_create(nullptr, 0, mRoot._errback()), mRoot._errback());
    mRoot.mpMapNode = cn_cbor_tag_create(COSETag::Sign1, root, mRoot._errback());
  }

  /**
   * @brief Parses CBOR data into a COSE_Sign1 message
   *
   * @param buf CBOR data to parse
   * @retval true Parsing was successful
   * @retval false Parsing failed
   */
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

  /**
   * @brief Cleans up all internal data structures
   */
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

  /**
   * @brief Sets the protected header from a CBORObject
   *
   * @param pHeader CBORObject containing the header data
   * @retval true The protected header was set successfully
   * @retval false The protected header was not set successfully
   */
  bool _setProtectedHeader(const CBORObject &pHeader) {
    mRawProtectedHeader = pHeader.build();
    return cn_cbor_data_update(mpProtectedHeader, mRawProtectedHeader.raw(), mRawProtectedHeader.size());
  }

  /**
   * @brief Sets the signature value
   *
   * @param signature Signature bytes to set
   * @retval true The signature was set successfully
   * @retval false The signature was not set successfully
   */
  bool _setSignature(const Bytes &signature) {
    mRawSignature = signature;
    return cn_cbor_data_update(mpSignature, mRawSignature.raw(), mRawSignature.size());
  }

  /**
   * @brief Constructs the Sig_structure for signing/verification as defined in RFC 8152
   *
   * @param external Optional external data (AAD) to include in the signature calculation
   * @retval Bytes Contains the CBOR-encoded Sig_structure data
   */
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

  CBORObject mRoot;                 /**< Root CBOR object for the message */
  cn_cbor *mpProtectedHeader;       /**< Pointer to the protected header in the CBOR structure */
  cn_cbor *mpUnprotectedHeader;     /**< Pointer to the unprotected header in the CBOR structure */
  cn_cbor *mpPayload;               /**< Pointer to the payload in the CBOR structure */
  cn_cbor *mpSignature;             /**< Pointer to the signature in the CBOR structure */

  Bytes mRawProtectedHeader;        /**< Raw bytes of the protected header */
  Bytes mRawPayload;                /**< Raw bytes of the payload */
  Bytes mRawSignature;              /**< Raw bytes of the signature */
  bool mReadSuccess;                /**< Flag indicating if the last read operation was successful */
};
/** @} */
}  // namespace uniot
