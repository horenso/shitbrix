#include "enet_helper.hpp"
#include "globals.hpp"
#include "error.hpp"

namespace
{

const size_t NET_CHANNELS = 2;

}

ENet& ENet::instance()
{
	static ENet enet;
	return enet;
}

HostPtr ENet::create_server(enet_uint16 port) const
{
	ENetAddress address;

	/* Bind the server to the default localhost.     */
	/* A specific host address can be specified by   */
	/* enet_address_set_host (& address, "x.x.x.x"); */
	address.host = ENET_HOST_ANY;
	address.port = port;

	Log::info("ENet: Create Server.");
	HostPtr server{enet_host_create(&address, MAX_CLIENTS, NET_CHANNELS, 0, 0)};
	enetok(server.get());

	return server;
}

std::pair<HostPtr, ENetPeer*> ENet::create_client(const char* server_name, enet_uint16 port) const
{
	Log::info("ENet: Create Client.");
	HostPtr host{enet_host_create(NULL, 1, NET_CHANNELS, 0, 0)};
	enetok(host.get());

	ENetAddress address;
	enet_address_set_host(&address, server_name);
	address.port = port;

	Log::info("ENet: Connect to \"%s\".", server_name);
	ENetPeer* peer;
	enetok(peer = enet_host_connect(host.get(), &address, 2, 0));

	return std::make_pair(move(host), peer);
}

PacketPtr ENet::create_packet(const std::string &data, ENetPacketFlag flag) const
{
	PacketPtr packet{enet_packet_create(data.c_str(), data.size() + 1, flag)};
	enetok(packet.get());

	return move(packet);
}

ENet::ENet()
{
	enetok(enet_initialize());
}

ENet::~ENet()
{
	enet_deinitialize();
}
