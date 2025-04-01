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

/**
 * @defgroup utils_cose COSE (CBOR Object Signing and Encryption)
 * @ingroup utils
 * @brief CBOR Object Signing and Encryption (COSE) implementation
 *
 * Provides enumerations for working with COSE (RFC 8152), which
 * defines how to create and process signatures, message authentication codes,
 * and encryption using CBOR for IoT applications.
 *
 * COSE uses CBOR encoding and provides similar functionality to JOSE (JSON Object
 * Signing and Encryption) but with a more compact representation.
 *
 * @see https://tools.ietf.org/html/rfc8152
 */

namespace uniot {

/**
 * @enum COSETag
 * @brief CBOR tag values that identify the type of COSE message
 * @ingroup utils_cose
 *
 * These tags are used in CBOR encoding to indicate the COSE message type.
 * Each tag corresponds to a specific security operation defined in RFC 8152.
 */
typedef enum COSETag {
  Sign = 98,      ///< Multiple signature (tag 98) - COSE_Sign
  Sign1 = 18,     ///< Single signature (tag 18) - COSE_Sign1
  Encrypted = 96, ///< Multiple recipient encrypted data (tag 96) - COSE_Encrypt
  Encrypted1 = 16, ///< Single recipient encrypted data (tag 16) - COSE_Encrypt0
  Mac = 97,       ///< Multiple recipient MAC (tag 97) - COSE_Mac
  Mac0 = 17       ///< Single recipient MAC (tag 17) - COSE_Mac0
} COSETag;

/**
 * @enum COSEHeaderLabel
 * @brief Common header parameter labels for COSE messages
 * @ingroup utils_cose
 *
 * These integer values identify header parameters in the protected and
 * unprotected header maps of COSE messages, as defined in RFC 8152 Section 3.
 */
typedef enum COSEHeaderLabel {
  Algorithm = 1,          ///< Cryptographic algorithm used (label 1)
  Critical = 2,           ///< Critical headers that must be understood (label 2)
  ContentType = 3,        ///< Content type of the payload (label 3)
  KeyIdentifier = 4,      ///< Key identifier (kid) (label 4)
  IV = 5,                 ///< Initialization Vector (label 5)
  PartialIV = 6,          ///< Partial Initialization Vector (label 6)
  CounterSignature = 7,   ///< Counter signature (label 7)
  CounterSignature0 = 9,  ///< Counter signature w/o countersigner (label 9)
  // ...                  ///< Additional header parameters may be defined
} COSEHeaderLabel;

/**
 * @enum COSEAlgorithm
 * @brief Cryptographic algorithm identifiers for COSE
 * @ingroup utils_cose
 *
 * These values identify cryptographic algorithms used in COSE messages.
 * Negative values are used for crypto algorithms as per the COSE standard.
 * The algorithms are described in RFC 8152 Sections 8, 9, and 10.
 */
typedef enum COSEAlgorithm {
  PS256 = -37,  ///< RSASSA-PSS w/ SHA-256
  PS384 = -38,  ///< RSASSA-PSS w/ SHA-384
  PS512 = -39,  ///< RSASSA-PSS w/ SHA-512
  ES256 = -7,   ///< ECDSA w/ SHA-256
  ES384 = -35,  ///< ECDSA w/ SHA-384
  ES512 = -36,  ///< ECDSA w/ SHA-512
  EdDSA = -8,   ///< EdDSA signature algorithm
  // ...        ///< Additional algorithms may be defined
} COSEAlgorithm;

}  // namespace uniot
