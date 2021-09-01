// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/connect_job_factory.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/memory/scoped_refptr.h"
#include "net/base/host_port_pair.h"
#include "net/base/network_isolation_key.h"
#include "net/base/privacy_mode.h"
#include "net/base/proxy_server.h"
#include "net/base/request_priority.h"
#include "net/dns/public/secure_dns_policy.h"
#include "net/http/http_proxy_connect_job.h"
#include "net/socket/connect_job.h"
#include "net/socket/socket_tag.h"
#include "net/socket/socks_connect_job.h"
#include "net/socket/ssl_connect_job.h"
#include "net/socket/transport_connect_job.h"
#include "net/socket/websocket_transport_connect_job.h"
#include "net/ssl/ssl_config.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/abseil-cpp/absl/types/variant.h"
#include "url/gurl.h"
#include "url/scheme_host_port.h"

namespace net {

namespace {

template <typename T>
std::unique_ptr<T> CreateFactoryIfNull(std::unique_ptr<T> in) {
  if (in)
    return in;
  return std::make_unique<T>();
}

bool UsingSsl(const ConnectJobFactory::Endpoint& endpoint) {
  if (absl::holds_alternative<url::SchemeHostPort>(endpoint)) {
    return GURL::SchemeIsCryptographic(
        base::ToLowerASCII(absl::get<url::SchemeHostPort>(endpoint).scheme()));
  }

  DCHECK(
      absl::holds_alternative<ConnectJobFactory::SchemelessEndpoint>(endpoint));
  return absl::get<ConnectJobFactory::SchemelessEndpoint>(endpoint).using_ssl;
}

HostPortPair ToHostPortPair(const ConnectJobFactory::Endpoint& endpoint) {
  if (absl::holds_alternative<url::SchemeHostPort>(endpoint)) {
    return HostPortPair::FromSchemeHostPort(
        absl::get<url::SchemeHostPort>(endpoint));
  }

  DCHECK(
      absl::holds_alternative<ConnectJobFactory::SchemelessEndpoint>(endpoint));
  return absl::get<ConnectJobFactory::SchemelessEndpoint>(endpoint)
      .host_port_pair;
}

TransportSocketParams::Endpoint ToTransportEndpoint(
    const ConnectJobFactory::Endpoint& endpoint) {
  if (absl::holds_alternative<url::SchemeHostPort>(endpoint))
    return absl::get<url::SchemeHostPort>(endpoint);

  DCHECK(
      absl::holds_alternative<ConnectJobFactory::SchemelessEndpoint>(endpoint));
  return absl::get<ConnectJobFactory::SchemelessEndpoint>(endpoint)
      .host_port_pair;
}

}  // namespace

ConnectJobFactory::ConnectJobFactory(
    std::unique_ptr<HttpProxyConnectJob::Factory>
        http_proxy_connect_job_factory,
    std::unique_ptr<SOCKSConnectJob::Factory> socks_connect_job_factory,
    std::unique_ptr<SSLConnectJob::Factory> ssl_connect_job_factory,
    std::unique_ptr<TransportConnectJob::Factory> transport_connect_job_factory,
    std::unique_ptr<WebSocketTransportConnectJob::Factory>
        websocket_transport_connect_job_factory)
    : http_proxy_connect_job_factory_(
          CreateFactoryIfNull(std::move(http_proxy_connect_job_factory))),
      socks_connect_job_factory_(
          CreateFactoryIfNull(std::move(socks_connect_job_factory))),
      ssl_connect_job_factory_(
          CreateFactoryIfNull(std::move(ssl_connect_job_factory))),
      transport_connect_job_factory_(
          CreateFactoryIfNull(std::move(transport_connect_job_factory))),
      websocket_transport_connect_job_factory_(CreateFactoryIfNull(
          std::move(websocket_transport_connect_job_factory))) {}

ConnectJobFactory::~ConnectJobFactory() = default;

std::unique_ptr<ConnectJob> ConnectJobFactory::CreateConnectJob(
    url::SchemeHostPort endpoint,
    const ProxyServer& proxy_server,
    const absl::optional<NetworkTrafficAnnotationTag>& proxy_annotation_tag,
    const SSLConfig* ssl_config_for_origin,
    const SSLConfig* ssl_config_for_proxy,
    bool force_tunnel,
    PrivacyMode privacy_mode,
    const OnHostResolutionCallback& resolution_callback,
    RequestPriority request_priority,
    SocketTag socket_tag,
    const NetworkIsolationKey& network_isolation_key,
    SecureDnsPolicy secure_dns_policy,
    const CommonConnectJobParams* common_connect_job_params,
    ConnectJob::Delegate* delegate) const {
  return CreateConnectJob(
      Endpoint(std::move(endpoint)), proxy_server, proxy_annotation_tag,
      ssl_config_for_origin, ssl_config_for_proxy, force_tunnel, privacy_mode,
      resolution_callback, request_priority, socket_tag, network_isolation_key,
      secure_dns_policy, common_connect_job_params, delegate);
}

std::unique_ptr<ConnectJob> ConnectJobFactory::CreateConnectJob(
    bool using_ssl,
    HostPortPair endpoint,
    const ProxyServer& proxy_server,
    const absl::optional<NetworkTrafficAnnotationTag>& proxy_annotation_tag,
    const SSLConfig* ssl_config_for_origin,
    const SSLConfig* ssl_config_for_proxy,
    bool force_tunnel,
    PrivacyMode privacy_mode,
    const OnHostResolutionCallback& resolution_callback,
    RequestPriority request_priority,
    SocketTag socket_tag,
    const NetworkIsolationKey& network_isolation_key,
    SecureDnsPolicy secure_dns_policy,
    const CommonConnectJobParams* common_connect_job_params,
    ConnectJob::Delegate* delegate) const {
  SchemelessEndpoint schemeless_endpoint{using_ssl, std::move(endpoint)};
  return CreateConnectJob(
      std::move(schemeless_endpoint), proxy_server, proxy_annotation_tag,
      ssl_config_for_origin, ssl_config_for_proxy, force_tunnel, privacy_mode,
      resolution_callback, request_priority, socket_tag, network_isolation_key,
      secure_dns_policy, common_connect_job_params, delegate);
}

std::unique_ptr<ConnectJob> ConnectJobFactory::CreateConnectJob(
    Endpoint endpoint,
    const ProxyServer& proxy_server,
    const absl::optional<NetworkTrafficAnnotationTag>& proxy_annotation_tag,
    const SSLConfig* ssl_config_for_origin,
    const SSLConfig* ssl_config_for_proxy,
    bool force_tunnel,
    PrivacyMode privacy_mode,
    const OnHostResolutionCallback& resolution_callback,
    RequestPriority request_priority,
    SocketTag socket_tag,
    const NetworkIsolationKey& network_isolation_key,
    SecureDnsPolicy secure_dns_policy,
    const CommonConnectJobParams* common_connect_job_params,
    ConnectJob::Delegate* delegate) const {
  scoped_refptr<HttpProxySocketParams> http_proxy_params;
  scoped_refptr<SOCKSSocketParams> socks_params;

  if (!proxy_server.is_direct()) {
    // No need to use a NetworkIsolationKey for looking up the proxy's IP
    // address. Cached proxy IP addresses doesn't really expose useful
    // information to destination sites, and not caching them has a performance
    // cost.
    auto proxy_tcp_params = base::MakeRefCounted<TransportSocketParams>(
        proxy_server.host_port_pair(), NetworkIsolationKey(), secure_dns_policy,
        resolution_callback);

    if (proxy_server.is_http_like()) {
      scoped_refptr<SSLSocketParams> ssl_params;
      if (proxy_server.is_secure_http_like()) {
        DCHECK(ssl_config_for_proxy);
        // Set ssl_params, and unset proxy_tcp_params
        ssl_params = base::MakeRefCounted<SSLSocketParams>(
            std::move(proxy_tcp_params), nullptr, nullptr,
            proxy_server.host_port_pair(), *ssl_config_for_proxy,
            PRIVACY_MODE_DISABLED, network_isolation_key);
        proxy_tcp_params = nullptr;
      }

      // TODO(crbug.com/1206799): Pass `endpoint` directly (preserving scheme
      // when available)?
      http_proxy_params = base::MakeRefCounted<HttpProxySocketParams>(
          std::move(proxy_tcp_params), std::move(ssl_params),
          proxy_server.is_quic(), ToHostPortPair(endpoint),
          force_tunnel || UsingSsl(endpoint), *proxy_annotation_tag,
          network_isolation_key);
    } else {
      DCHECK(proxy_server.is_socks());
      // TODO(crbug.com/1206799): Pass `endpoint` directly (preserving scheme
      // when available)?
      socks_params = base::MakeRefCounted<SOCKSSocketParams>(
          std::move(proxy_tcp_params),
          proxy_server.scheme() == ProxyServer::SCHEME_SOCKS5,
          ToHostPortPair(endpoint), network_isolation_key,
          *proxy_annotation_tag);
    }
  }

  // Deal with SSL - which layers on top of any given proxy.
  if (UsingSsl(endpoint)) {
    DCHECK(ssl_config_for_origin);
    scoped_refptr<TransportSocketParams> ssl_tcp_params;
    if (proxy_server.is_direct()) {
      ssl_tcp_params = base::MakeRefCounted<TransportSocketParams>(
          ToTransportEndpoint(endpoint), network_isolation_key,
          secure_dns_policy, resolution_callback);
    }
    // TODO(crbug.com/1206799): Pass `endpoint` directly (preserving scheme
    // when available)?
    auto ssl_params = base::MakeRefCounted<SSLSocketParams>(
        std::move(ssl_tcp_params), std::move(socks_params),
        std::move(http_proxy_params), ToHostPortPair(endpoint),
        *ssl_config_for_origin, privacy_mode, network_isolation_key);
    return ssl_connect_job_factory_->Create(
        request_priority, socket_tag, common_connect_job_params,
        std::move(ssl_params), delegate, /*net_log=*/nullptr);
  }

  if (proxy_server.is_http_like()) {
    return http_proxy_connect_job_factory_->Create(
        request_priority, socket_tag, common_connect_job_params,
        std::move(http_proxy_params), delegate, /*net_log=*/nullptr);
  }

  if (proxy_server.is_socks()) {
    return socks_connect_job_factory_->Create(
        request_priority, socket_tag, common_connect_job_params,
        std::move(socks_params), delegate, /*net_log=*/nullptr);
  }

  DCHECK(proxy_server.is_direct());
  auto tcp_params = base::MakeRefCounted<TransportSocketParams>(
      ToTransportEndpoint(endpoint), network_isolation_key, secure_dns_policy,
      resolution_callback);
  if (!common_connect_job_params->websocket_endpoint_lock_manager) {
    return transport_connect_job_factory_->Create(
        request_priority, socket_tag, common_connect_job_params, tcp_params,
        delegate, /*net_log=*/nullptr);
  }

  return websocket_transport_connect_job_factory_->Create(
      request_priority, socket_tag, common_connect_job_params, tcp_params,
      delegate, /*net_log=*/nullptr);
}

}  // namespace net
