#pragma once
namespace InetSpeedUAP
{
	public enum class ConnectionSpeed
	{
		High,
		Average,
		Low,
		Unknown
	};

	private enum class ConnectionType
	{
		Cellular,
		WiFi,
		LAN,
		None
	};
}

