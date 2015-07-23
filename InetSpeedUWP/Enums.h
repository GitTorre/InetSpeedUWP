#pragma once
namespace InetSpeedUWP
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

