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

namespace uniot {
typedef enum COSETag {
  Sign = 98,
  Sign1 = 18,
  Encrypted = 96,
  Encrypted1 = 16,
  Mac = 97,
  Mac0 = 17
} COSETag;

typedef enum COSEHeaderLabel {
  Algorithm = 1,
  Critical = 2,
  ContentType = 3,
  KeyIdentifier = 4,
  IV = 5,
  PartialIV = 6,
  CounterSignature = 7,
  CounterSignature0 = 9,
  // ...
} COSEHeaderLabel;

typedef enum COSEAlgorithm {
  PS256 = -37,
  PS384 = -38,
  PS512 = -39,
  ES256 = -7,
  ES384 = -35,
  ES512 = -36,
  EdDSA = -8,
  // ...
} COSEAlgorithm;
}  // namespace uniot
