// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SSL_SSL_SERVER_CONFIG_H_
#define NET_SSL_SSL_SERVER_CONFIG_H_

#include <stdint.h>

#include <vector>

#include "base/containers/flat_map.h"
#include "net/base/net_export.h"
#include "net/socket/next_proto.h"
#include "net/ssl/ssl_config.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace net {

class ClientCertVerifier;

// A collection of server-side SSL-related configuration settings.
struct NET_EXPORT SSLServerConfig {
  enum ClientCertType {
    NO_CLIENT_CERT,
    OPTIONAL_CLIENT_CERT,
    REQUIRE_CLIENT_CERT,
  };

  // Defaults
  SSLServerConfig();
  SSLServerConfig(const SSLServerConfig& other);
  ~SSLServerConfig();

  // The minimum and maximum protocol versions that are enabled.
  // (Use the SSL_PROTOCOL_VERSION_xxx enumerators defined in ssl_config.h)
  // SSL 2.0 and SSL 3.0 are not supported. If version_max < version_min, it
  // means no protocol versions are enabled.
  uint16_t version_min;
  uint16_t version_max;

  // Whether early data is enabled on this connection. The caller is obligated
  // to reject early data that is non-safe to be replayed.
  bool early_data_enabled;

  // Presorted list of cipher suites which should be explicitly prevented from
  // being used in addition to those disabled by the net built-in policy.
  //
  // By default, all cipher suites supported by the underlying SSL
  // implementation will be enabled except for:
  // - Null encryption cipher suites.
  // - Weak cipher suites: < 80 bits of security strength.
  // - FORTEZZA cipher suites (obsolete).
  // - IDEA cipher suites (RFC 5469 explains why).
  // - Anonymous cipher suites.
  // - ECDSA cipher suites on platforms that do not support ECDSA signed
  //   certificates, as servers may use the presence of such ciphersuites as a
  //   hint to send an ECDSA certificate.
  // The ciphers listed in |disabled_cipher_suites| will be removed in addition
  // to the above list.
  //
  // Though cipher suites are sent in TLS as "uint8_t CipherSuite[2]", in
  // big-endian form, they should be declared in host byte order, with the
  // first uint8_t occupying the most significant byte.
  // Ex: To disable TLS_RSA_WITH_RC4_128_MD5, specify 0x0004, while to
  // disable TLS_ECDH_ECDSA_WITH_RC4_128_SHA, specify 0xC002.
  std::vector<uint16_t> disabled_cipher_suites;

  // If true, causes only ECDHE cipher suites to be enabled.
  bool require_ecdhe;

  // cipher_suite_for_testing, if set, causes the server to only support the
  // specified cipher suite in TLS 1.2 and below. This should only be used in
  // unit tests.
  absl::optional<uint16_t> cipher_suite_for_testing;

  // signature_algorithm_for_testing, if set, causes the server to only support
  // the specified signature algorithm in TLS 1.2 and below. This should only be
  // used in unit tests.
  absl::optional<uint16_t> signature_algorithm_for_testing;

  // curves_for_testing, if not empty, specifies the list of NID values (e.g.
  // NID_X25519) to configure as supported curves for the TLS connection.
  std::vector<int> curves_for_testing;

  // Sets the requirement for client certificates during handshake.
  ClientCertType client_cert_type;

  // List of DER-encoded X.509 DistinguishedName of certificate authorities
  // to be included in the CertificateRequest handshake message,
  // if client certificates are required.
  std::vector<std::string> cert_authorities;

  // Provides the ClientCertVerifier that is to be used to verify
  // client certificates during the handshake.
  // The |client_cert_verifier| continues to be owned by the caller,
  // and must outlive any sockets spawned from this SSLServerContext.
  // This field is meaningful only if client certificates are requested.
  // If a verifier is not provided then all certificates are accepted.
  ClientCertVerifier* client_cert_verifier;

  // The list of application level protocols supported with ALPN (Application
  // Layer Protocol Negotiation), in decreasing order of preference.  Protocols
  // will be advertised in this order during TLS handshake.
  NextProtoVector alpn_protos;

  // ALPS TLS extension is enabled and corresponding data is sent to client if
  // client also enabled ALPS, for each NextProto in |application_settings|.
  // Data might be empty.
  base::flat_map<NextProto, std::vector<uint8_t>> application_settings;

  // If non-empty, the DER-encoded OCSP response to staple.
  std::vector<uint8_t> ocsp_response;
};

}  // namespace net

#endif  // NET_SSL_SSL_SERVER_CONFIG_H_
