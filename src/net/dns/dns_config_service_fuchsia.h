// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_DNS_DNS_CONFIG_SERVICE_FUCHSIA_H_
#define NET_DNS_DNS_CONFIG_SERVICE_FUCHSIA_H_

#include "base/macros.h"
#include "net/dns/dns_config_service.h"

namespace net {
namespace internal {

// DnsConfigService implementation for Fuchsia. Currently is a stub that returns
// an empty config (which means DNS resolver always falls back to
// getaddrinfo()).
class NET_EXPORT_PRIVATE DnsConfigServiceFuchsia : public DnsConfigService {
 public:
  DnsConfigServiceFuchsia();
  ~DnsConfigServiceFuchsia() override;

 protected:
  // DnsConfigService overrides.
  void ReadConfigNow() override;
  void ReadHostsNow() override;
  bool StartWatching() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DnsConfigServiceFuchsia);
};

}  // namespace internal
}  // namespace net

#endif  // NET_DNS_DNS_CONFIG_SERVICE_FUCHSIA_H_
