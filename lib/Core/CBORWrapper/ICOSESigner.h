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

#include <Bytes.h>

#include "COSE.h"

namespace uniot {
/**
 * @brief Interface for CBOR Object Signing and Encryption (COSE) signing operations
 * @ingroup utils_cose
 * @{
 *
 * ICOSESigner provides an interface for implementing cryptographic signing
 * functionality according to the COSE (RFC 8152) standard. Classes implementing
 * this interface can be used to sign data using various cryptographic algorithms.
 */
class ICOSESigner {
 public:
  /**
   * @brief Virtual destructor to ensure proper cleanup of derived classes
   */
  virtual ~ICOSESigner() {}

  /**
   * @brief Gets the key identifier used by this signer
   *
   * @retval Bytes The key identifier as a byte sequence
   */
  virtual Bytes keyId() const = 0;

  /**
   * @brief Signs the provided data using the implementation's cryptographic algorithm
   *
   * @param data The bytes to be signed
   * @retval Bytes The resulting signature as a byte sequence
   */
  virtual Bytes sign(const Bytes &data) const = 0;

  /**
   * @brief Gets the COSE algorithm identifier used by this signer
   *
   * @retval COSEAlgorithm The algorithm identifier as defined in the COSE standard
   */
  virtual COSEAlgorithm signerAlgorithm() const = 0;
};
/** @} */
}  // namespace uniot
