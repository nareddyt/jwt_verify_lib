// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "jwt_verify_lib/jwt.h"

#include "google/protobuf/util/message_differencer.h"
#include "gtest/gtest.h"
#include "jwt_verify_lib/struct_utils.h"

using google::protobuf::util::MessageDifferencer;

#include <functional>
#include <vector>

namespace google {
namespace jwt_verify {
namespace {

// JWT with
// Header:  {"alg":"RS256","typ":"JWT","customheader":"abc"}
// Payload:
// {"iss":"https://example.com","sub":"test@example.com","iat":
// 1501281000,"exp":1501281058,"nbf":1501281000,"jti":"identity","custompayload":1234}
const std::string good_jwt =
    "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImN1c3RvbWhlYWRlciI6ImFiYyJ9Cg."
    "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIsIm"
    "lh"
    "dCI6IDE1MDEyODEwMDAsImV4cCI6MTUwMTI4MTA1OCwibmJmIjoxNTAxMjgxMDAwLCJqdGkiOi"
    "Jp"
    "ZGVudGl0eSIsImN1c3RvbXBheWxvYWQiOjEyMzR9Cg"
    ".U2lnbmF0dXJl";

TEST(JwtParseTest, GoodJwt) {
  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(good_jwt), Status::Ok);

  EXPECT_EQ(jwt.alg_, "RS256");
  EXPECT_EQ(jwt.kid_, "");
  EXPECT_EQ(jwt.iss_, "https://example.com");
  EXPECT_EQ(jwt.sub_, "test@example.com");
  EXPECT_EQ(jwt.audiences_, std::vector<std::string>());
  EXPECT_EQ(jwt.iat_, 1501281000);
  EXPECT_EQ(jwt.nbf_, 1501281000);
  EXPECT_EQ(jwt.exp_, 1501281058);
  EXPECT_EQ(jwt.jti_, std::string("identity"));
  EXPECT_EQ(jwt.signature_, "Signature");

  StructUtils header_getter(jwt.header_pb_);
  std::string str_value;
  EXPECT_EQ(header_getter.GetString("customheader", &str_value),
            StructUtils::OK);
  EXPECT_EQ(str_value, std::string("abc"));

  StructUtils payload_getter(jwt.payload_pb_);
  uint64_t int_value;
  EXPECT_EQ(payload_getter.GetUInt64("custompayload", &int_value),
            StructUtils::OK);
  EXPECT_EQ(int_value, 1234);
}

TEST(JwtParseTest, Copy) {
  Jwt original;
  ASSERT_EQ(original.parseFromString(good_jwt), Status::Ok);

  // Copy constructor
  Jwt constructed(original);
  Jwt copied;
  copied = original;

  std::vector<std::reference_wrapper<Jwt>> jwts{constructed, copied};

  for (auto jwt = jwts.begin(); jwt != jwts.end(); ++jwt) {
    Jwt &ref = (*jwt);
    EXPECT_EQ(ref.alg_, original.alg_);
    EXPECT_EQ(ref.kid_, original.kid_);
    EXPECT_EQ(ref.iss_, original.iss_);
    EXPECT_EQ(ref.sub_, original.sub_);
    EXPECT_EQ(ref.audiences_, original.audiences_);
    EXPECT_EQ(ref.iat_, original.iat_);
    EXPECT_EQ(ref.nbf_, original.nbf_);
    EXPECT_EQ(ref.exp_, original.exp_);
    EXPECT_EQ(ref.jti_, original.jti_);
    EXPECT_EQ(ref.signature_, original.signature_);
    EXPECT_TRUE(
        MessageDifferencer::Equals(ref.header_pb_, original.header_pb_));
    EXPECT_TRUE(
        MessageDifferencer::Equals(ref.payload_pb_, original.payload_pb_));
  }
}

TEST(JwtParseTest, GoodJwtWithMultiAud) {
  // {"iss":"https://example.com","aud":["aud1","aud2"],"exp":1517878659,"sub":"https://example.com"}
  const std::string jwt_text =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6ImFmMDZjMTlmOGU1YjMzMTUyMT"
      "ZkZjAxMGZkMmI5YTkzYmFjMTM1YzgifQ."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwiYXVkIjpbImF1ZDEiLCJhdWQyIl0sImV4"
      "cCI6"
      "MTUxNzg3ODY1OSwic3ViIjoiaHR0cHM6Ly9leGFtcGxlLmNvbSJ9Cg"
      ".U2lnbmF0dXJl";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text), Status::Ok);

  EXPECT_EQ(jwt.jwt_, jwt_text);
  EXPECT_EQ(jwt.alg_, "RS256");
  EXPECT_EQ(jwt.kid_, "af06c19f8e5b3315216df010fd2b9a93bac135c8");
  EXPECT_EQ(jwt.iss_, "https://example.com");
  EXPECT_EQ(jwt.sub_, "https://example.com");
  EXPECT_EQ(jwt.audiences_, std::vector<std::string>({"aud1", "aud2"}));
  EXPECT_EQ(jwt.iat_, 0);  // When there's no iat claim default to 0
  EXPECT_EQ(jwt.nbf_, 0);  // When there's no nbf claim default to 0
  EXPECT_EQ(
      jwt.jti_,
      std::string(""));  // When there's no jti claim default to an empty string
  EXPECT_EQ(jwt.exp_, 1517878659);
  EXPECT_EQ(jwt.signature_, "Signature");
}

TEST(JwtParseTest, TestEmptyJwt) {
  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(""), Status::JwtBadFormat);
}

TEST(JwtParseTest, TestTooManySections) {
  Jwt jwt;
  std::string jwt_str = "aaa.bbb.ccc.ddd.eee";
  ASSERT_EQ(jwt.parseFromString(jwt_str), Status::JwtBadFormat);
}

TEST(JwtParseTest, TestTooLargeJwt) {
  Jwt jwt;
  // string > 8096 of MaxJwtSize
  std::string jwt_str(10240, 'c');
  ASSERT_EQ(jwt.parseFromString(jwt_str), Status::JwtBadFormat);
}

TEST(JwtParseTest, TestParseHeaderBadBase64) {
  /*
   * jwt with header replaced by
   * "{"alg":"RS256","typ":"JWT", this is a invalid json}"
   */
  const std::string jwt_text =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsIHRoaXMgaXMgYSBpbnZhbGlkIGpzb259+."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0.VGVzdFNpZ25hdHVyZQ";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text),
            Status::JwtHeaderParseErrorBadBase64);
}

TEST(JwtParseTest, TestParseHeaderBadJson) {
  /*
   * jwt with header replaced by
   * "{"alg":"RS256","typ":"JWT", this is a invalid json}"
   */
  const std::string jwt_text =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsIHRoaXMgaXMgYSBpbnZhbGlkIGpzb259."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0.VGVzdFNpZ25hdHVyZQ";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text), Status::JwtHeaderParseErrorBadJson);
}

TEST(JwtParseTest, TestParseHeaderAbsentAlg) {
  /*
   * jwt with header replaced by
   * "{"typ":"JWT"}"
   */
  const std::string jwt_text =
      "eyJ0eXAiOiJKV1QifQ."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0"
      ".VGVzdFNpZ25hdHVyZQ";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text), Status::JwtHeaderBadAlg);
}

TEST(JwtParseTest, TestParseHeaderAlgIsNotString) {
  /*
   * jwt with header replaced by
   * "{"alg":256,"typ":"JWT"}"
   */
  const std::string jwt_text =
      "eyJhbGciOjI1NiwidHlwIjoiSldUIn0."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0.VGVzdFNpZ25hdHVyZQ";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text), Status::JwtHeaderBadAlg);
}

TEST(JwtParseTest, TestParseHeaderInvalidAlg) {
  /*
   * jwt with header replaced by
   * "{"alg":"InvalidAlg","typ":"JWT"}"
   */
  const std::string jwt_text =
      "eyJhbGciOiJJbnZhbGlkQWxnIiwidHlwIjoiSldUIn0."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0.VGVzdFNpZ25hdHVyZQ";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text), Status::JwtHeaderNotImplementedAlg);
}

TEST(JwtParseTest, TestParseHeaderBadFormatKid) {
  // JWT with bad-formatted kid
  // Header:  {"alg":"RS256","typ":"JWT","kid":1}
  // Payload:
  // {"iss":"https://example.com","sub":"test@example.com","exp":1501281058}
  const std::string jwt_text =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6MX0."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0.VGVzdFNpZ25hdHVyZQ";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text), Status::JwtHeaderBadKid);
}

TEST(JwtParseTest, TestParsePayloadBadBase64) {
  /*
   * jwt with payload replaced by
   * "this is not a json"
   */
  const std::string jwt_text =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.dGhpcyBpcyBub3QgYSBqc29u+."
      "VGVzdFNpZ25hdHVyZQ";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text),
            Status::JwtPayloadParseErrorBadBase64);
}

TEST(JwtParseTest, TestParsePayloadBadJson) {
  /*
   * jwt with payload replaced by
   * "this is not a json"
   */
  const std::string jwt_text =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.dGhpcyBpcyBub3QgYSBqc29u."
      "VGVzdFNpZ25hdHVyZQ";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text), Status::JwtPayloadParseErrorBadJson);
}

TEST(JwtParseTest, TestParsePayloadIssNotString) {
  /*
   * jwt with payload { "iss": true, "sub": "test_subject", "exp": 123456789 }
   */
  const std::string jwt_text =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyAiaXNzIjogdHJ1ZSwgICAgInN1YiI6ICJ0ZXN0X3N1YmplY3QiLCAgImV4cCI6ICAxMjM0"
      "NTY3ODkgfQ."
      "VGVzdFNpZ25hdHVyZQ";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text),
            Status::JwtPayloadParseErrorIssNotString);
}

TEST(JwtParseTest, TestParsePayloadSubNotString) {
  /*
   * jwt with payload {"iss": "test_issuer", "sub": 123456, "exp": 123456789 }
   */
  const std::string jwt_text =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyAiaXNzIjoidGVzdF9pc3N1ZXIiLCAic3ViIjogMTIzNDU2LCAgImV4cCI6IDEyMzQ1Njc4"
      "OSB9."
      "VGVzdFNpZ25hdHVyZQ";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text),
            Status::JwtPayloadParseErrorSubNotString);
}

TEST(JwtParseTest, TestParsePayloadIatNotInteger) {
  /*
   * jwt with payload { "iss":"test_issuer", "sub": "test_subject", "iat":
   * "123456789" }
   */
  const std::string jwt_text =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyAiaXNzIjoidGVzdF9pc3N1ZXIiLCAic3ViIjogInRlc3Rfc3ViamVjdCIsICJpYXQiOiAi"
      "MTIzNDU2Nzg5IiB9."
      "VGVzdFNpZ25hdHVyZQ";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text),
            Status::JwtPayloadParseErrorIatNotInteger);
}

TEST(JwtParseTest, TestParsePayloadIatNotPositive) {
  /*
   * jwt with payload { "iss":"test_issuer", "sub": "test_subject", "iat":
   * "-12345" }
   */
  const std::string jwt_text =
      "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjotMTIzNDV9."
      "J0q58VUq4Vx71aVlH0gRCtNfmQrQ1Cw2dFVZ6WqDbBw";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text),
            Status::JwtPayloadParseErrorIatNotPositive);
}

TEST(JwtParseTest, TestParsePayloadNbfNotInteger) {
  /*
   * jwt with payload { "iss":"test_issuer", "sub": "test_subject", "nbf":
   * "123456789" }
   */
  const std::string jwt_text =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyAiaXNzIjoidGVzdF9pc3N1ZXIiLCAic3ViIjogInRlc3Rfc3ViamVjdCIsICJuYmYiOiAi"
      "MTIzNDU2Nzg5IiB9."
      "VGVzdFNpZ25hdHVyZQ";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text),
            Status::JwtPayloadParseErrorNbfNotInteger);
}

TEST(JwtParseTest, TestParsePayloadNbfNotPositive) {
  /*
   * jwt with payload { "iss":"test_issuer", "sub": "test_subject", "nbf":
   * "-12345" }
   */
  const std::string jwt_text =
      "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwibmJmIjotMTIzNDV9."
      "rlnrK7unNEaaghPFhNQnDp1GRbCU0rGORO2yDf5YIZk";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text),
            Status::JwtPayloadParseErrorNbfNotPositive);
}

TEST(JwtParseTest, TestParsePayloadExpNotInteger) {
  /*
   * jwt with payload { "iss":"test_issuer", "sub": "test_subject", "exp":
   * "123456789" }
   */
  const std::string jwt_text =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyAiaXNzIjoidGVzdF9pc3N1ZXIiLCAic3ViIjogInRlc3Rfc3ViamVjdCIsICJleHAiOiAi"
      "MTIzNDU2Nzg5IiB9."
      "VGVzdFNpZ25hdHVyZQ";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text),
            Status::JwtPayloadParseErrorExpNotInteger);
}

TEST(JwtParseTest, TestParsePayloadExpNotPositive) {
  /*
   * jwt with payload { "iss":"test_issuer", "sub": "test_subject", "exp":
   * "-12345" }
   */
  const std::string jwt_text =
      "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiZXhwIjotMTIzNDV9."
      "BCgzT_CEurIxa0MxbS9seJ62lgfJT54P7AQpUkp65GE";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text),
            Status::JwtPayloadParseErrorExpNotPositive);
}

TEST(JwtParseTest, TestParsePayloadJtiNotString) {
  /*
   * jwt with payload { "iss":"test_issuer", "sub": "test_subject", "jti":
   * 1234567}
   */
  const std::string jwt_text =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyAiaXNzIjoidGVzdF9pc3N1ZXIiLCAic3ViIjogInRlc3Rfc3ViamVjdCIsICJqdGkiOiAx"
      "MjM0NTY3fQ."
      "VGVzdFNpZ25hdHVyZQ";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text),
            Status::JwtPayloadParseErrorJtiNotString);
}

TEST(JwtParseTest, TestParsePayloadAudInteger) {
  /*
   * jwt with payload { "iss":"test_issuer", "sub": "test_subject", "aud":
   * 1234567}
   */
  const std::string jwt_text =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyAiaXNzIjoidGVzdF9pc3N1ZXIiLCAic3ViIjogInRlc3Rfc3ViamVjdCIsICJhdWQiOiAx"
      "MjM0NTY3fQ."
      "VGVzdFNpZ25hdHVyZQ";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text),
            Status::JwtPayloadParseErrorAudNotString);
}

TEST(JwtParseTest, TestParsePayloadAudIntegerList) {
  /*
   * jwt with payload { "iss":"test_issuer", "sub": "test_subject", "aud": [1,2]
   * }
   */
  const std::string jwt_text =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyAiaXNzIjoidGVzdF9pc3N1ZXIiLCAic3ViIjogInRlc3Rfc3ViamVjdCIsICJhdWQiOiBb"
      "MSwyXX0."
      "VGVzdFNpZ25hdHVyZQ";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text),
            Status::JwtPayloadParseErrorAudNotString);
}

TEST(JwtParseTest, InvalidSignature) {
  // {"iss":"https://example.com","sub":"test@example.com","exp":1501281058,
  // aud: [aud1, aud2] }
  // signature part is invalid.
  const std::string jwt_text =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6ImFmMDZjMTlmOGU1YjMzMTUyMT"
      "ZkZjAxMGZkMmI5YTkzYmFjMTM1YzgifQ.eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tI"
      "iwiaWF0IjoxNTE3ODc1MDU5LCJhdWQiOlsiYXVkMSIsImF1ZDIiXSwiZXhwIjoxNTE3ODc"
      "4NjU5LCJzdWIiOiJodHRwczovL2V4YW1wbGUuY29tIn0.invalid-signature";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text),
            Status::JwtSignatureParseErrorBadBase64);
}

TEST(JwtParseTest, GoodNestedJwt) {
  /*
   * jwt with payload
   * {
   *  "sub": "test@example.com",
   *  "aud": "example_service",
   *  "exp": 2001001001,
   *  "nested": {
   *    "key-1": "value1",
   *    "nested-2": {
   *      "key-2": "value2",
   *      "key-3": true,
   *      "key-4": 9999
   *    }
   *  }
   * }
   */
  const std::string jwt_text =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImF1ZCI6ImV4YW1wbGVfc2"
      "V"
      "ydmljZSIsImV4cCI6MjAwMTAwMTAwMSwibmVzdGVkIjp7ImtleS0xIjoidmFsdWUxIiwibmV"
      "zdGVkLTIiOnsia2V5LTIiO"
      "iJ"
      "2YWx1ZTIiLCJrZXktMyI6dHJ1ZSwia2V5LTQiOjk5OTl9fX0."
      "IWZiZ0dCqFG13fGKSu8t7nBHTFTXvtBXOp68gIcO-"
      "1K3k0dhuWwX6umIDm_1W9Y8NdztS-"
      "4jH4ULqRdR9QQFkxE7727USTHexN2sAqqxmAa1zdu2F-v3__VD8yONngWEWmw_"
      "n-RbP0H1NEBcQf4uYuLIXWi-buGBzcyxwpEPLFnCRarunCEMSp3loPCm-SOBNf2ISeQ0h_"
      "dpQ9dnWWxVvVA8T_AxROSto_"
      "8eF_"
      "o1zEnAbr8emLHDeeSFJNqhktT0ZTvv0__"
      "stILRAobYRO5ztRBUs4WJ6cgX7rGSMFo5cgP1RMrQKpfHKP9WFHpHhogQ4UXi7ndCxTM6r0G"
      "BinZRiA";

  Jwt jwt;
  ASSERT_EQ(jwt.parseFromString(jwt_text), Status::Ok);

  StructUtils payload_getter_1(jwt.payload_pb_);
  ::google::protobuf::Struct struct_value;
  EXPECT_EQ(payload_getter_1.GetStruct("nested", &struct_value),
            StructUtils::OK);
  StructUtils payload_getter_2(struct_value);
  std::string string_value;
  EXPECT_EQ(payload_getter_2.GetString("key-1", &string_value),
            StructUtils::OK);
  // fetching: nested.key-1 = value1
  EXPECT_EQ(string_value, "value1");

  ::google::protobuf::Struct nested_struct_value;
  EXPECT_EQ(payload_getter_2.GetStruct("nested-2", &nested_struct_value),
            StructUtils::OK);
  StructUtils payload_getter_3(nested_struct_value);
  EXPECT_EQ(payload_getter_3.GetString("key-2", &string_value),
            StructUtils::OK);
  // fetching: nested.nested-2.key-2 = value2
  EXPECT_EQ(string_value, "value2");
  bool bool_value;
  EXPECT_EQ(payload_getter_3.GetBoolean("key-3", &bool_value), StructUtils::OK);
  // fetching: nested.nested-2.key-3 = true
  EXPECT_EQ(bool_value, true);
  uint64_t int_value;
  EXPECT_EQ(payload_getter_3.GetUInt64("key-4", &int_value), StructUtils::OK);
  // fetching: nested.nested-2.key-4 = 9999
  EXPECT_EQ(int_value, 9999);
}

}  // namespace
}  // namespace jwt_verify
}  // namespace google
