#include "stdafx.h"
#include "socket_relay.h"

void socket_relay::map_client(uint32_t client_id, uint32_t token) {
    if (auto nh = m_clients.extract(client_id); !nh.empty()) {
        if (token > 0) {
            auto& dval = nh.mapped();
            dval.token = token;
            m_clients.insert(std::move(nh));
        }
    } else {
        if (token > 0) {
            relay_unit unit;
            unit.token = token;
            m_clients.emplace(client_id, unit);
        }
    }
}

void socket_relay::map_group(uint32_t group_id, uint32_t client_id, bool enter) {
    if (auto cit = m_clients.find(client_id); cit != m_clients.end()) {
        auto token = cit->second.token;
        if (auto it = m_groups.find(group_id); it != m_groups.end()) {
            if (enter) {
                it->second.insert(token);
            } else {
                it->second.erase(token);
            }
        } else {
            if (enter) {
                m_groups.emplace(group_id, std::set{ token });
            }
        }
    }
}

void socket_relay::map_server(uint32_t client_id, uint32_t server_id, uint32_t token) {
    auto service_id = (server_id >> 16) & 0xff;
    if (auto it = m_clients.find(client_id); it != m_clients.end()) {
        it->second.m_services[service_id] = { server_id, token };
    } else {
        relay_unit unit;
        unit.m_services[service_id] = { server_id, token };;
        m_clients.emplace(client_id, unit);
    }
}

bool socket_relay::check_service(uint32_t server_id, uint32_t client_id) {
     if (auto it = m_clients.find(client_id); it != m_clients.end()) {
        auto service_id = (server_id >> 16) & 0xff;
        return it->second.m_services[service_id].server_id == server_id;
     }
     return false;
}

void socket_relay::do_forward_broadcast(pbyte data, size_t data_len) {
    for (auto& [_, unit] : m_clients) {
        m_mgr->send(unit.token, data, data_len);
    }
}

void socket_relay::do_forward_client(relay_header* header, pbyte data, size_t data_len) {
    if (auto it = m_clients.find(header->target_id); it != m_clients.end()) {
        m_mgr->send(it->second.token, data, data_len);
    }
}

void socket_relay::do_forward_service(relay_header* header, uint32_t client_id, pbyte data, size_t data_len) {
    if (auto it = m_clients.find(header->target_id); it != m_clients.end()) {
        auto unit = it->second;
        uint8_t service_id = header->target_id;
        auto stoken = unit.m_services[service_id].token;
        if (unit.crc8 != header->crc8) {
            if (stoken > 0 && m_mgr->send(stoken, data, data_len)) {
                unit.crc8 = header->crc8;
                return;
            }
            header->code = CODE_UNREACH;
        } else {
            header->code = CODE_CRC8ERR;
        }
        header->type = RELAY_SELF;
        header->flag = (uint8_t)FLAG_RES;
        header->len = sizeof(relay_header);
        m_mgr->send(unit.token, &header, sizeof(relay_header));
    }
}

void socket_relay::do_forward_group(relay_header* header, pbyte data, size_t data_len) {
    if (auto it = m_groups.find(header->target_id); it != m_groups.end()) {
        for (auto token : it->second) {
            m_mgr->send(token, data, data_len);
        }
    }
}

uint8_t socket_relay::do_forward_relay(router_header* header, pbyte data, size_t data_len) {
    auto it = m_clients.find(header->target_id);
    if (it == m_clients.end()) {
        return m_relay_service_id;
    }
    auto sunit = it->second.m_services[header->service_id];
    if (sunit.token == 0) {
        return m_relay_service_id;
    }
    header->type = FORWARD_SELF;
    sendv_item items[] = { {header, sizeof(router_header)}, {data, data_len} };
    m_mgr->send(sunit.token, data, data_len);
    return 0;
}
