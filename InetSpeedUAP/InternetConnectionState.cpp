#include "pch.h"
#include "InternetConnectionState.h"
#include "Enums.h"
#include "pplpp.h"

using namespace InetSpeedUWP;
using namespace Windows::Networking;
using namespace Windows::Networking::Connectivity;
using namespace Windows::Networking::Sockets;
using namespace pplpp;

Array<String^>^ _socketTcpWellKnownHostNames = ref new Array <String^>(4) { "google.com", "bing.com", "facebook.com", "yahoo.com" };

//Care of http://stackoverflow.com/a/16533789
ConnectionType InternetConnectionState::GetConnectionType()
{
	auto profile = NetworkInformation::GetInternetConnectionProfile();
	if (profile == nullptr) return ConnectionType::None;

	auto interfaceType = profile->NetworkAdapter->IanaInterfaceType;

	// 71 is WiFi & 6 is Ethernet(LAN)
	if (interfaceType == 71)
	{
		return ConnectionType::WiFi;
	}
	else if (interfaceType == 6)
	{
		return ConnectionType::LAN;
	}
	// 243 & 244 is 3G/Mobile
	else if (interfaceType == 243 || interfaceType == 244)
	{
		return ConnectionType::Cellular;
	}
	else
	{
		return ConnectionType::None;
	}
}

ConnectionSpeed InternetConnectionState::GetConnectionSpeed(double roundtriptime)
{
	if (!(roundtriptime > 0.0))
	{
		return ConnectionSpeed::Unknown;
	}

	if (roundtriptime <= 0.0014)
	{
		return ConnectionSpeed::High;
	}

	if (roundtriptime > 0.0014 && roundtriptime < 0.14)
	{
		return ConnectionSpeed::Average;
	}

	return ConnectionSpeed::Low;
}

ConnectionSpeed InternetConnectionState::InternetConnectSocketAsync()
{
	bool _canceled = false;
	int retries = 4;
	long long task_timeout_ms = 1000;
	auto connectionType = InternetConnectionState::GetConnectionType();

	if (connectionType == ConnectionType::LAN)
	{
		task_timeout_ms = 1000;
	}

	if (connectionType == ConnectionType::Cellular || connectionType == ConnectionType::WiFi)
	{
		retries = 2;
	}

	double currentSpeed = 0.0;

	for (int i = 0; i < retries; ++i)
	{
		if (_serverHost == nullptr || !_custom)
		{
			_serverHost = ref new HostName(_socketTcpWellKnownHostNames[i]);
		}

		StreamSocket^ _clientSocket = ref new StreamSocket();
		_clientSocket->Control->NoDelay = true;
		_clientSocket->Control->QualityOfService = SocketQualityOfService::LowLatency;
		_clientSocket->Control->KeepAlive = false;

		//tasks must complete in a fixed amount of time, cancel otherwise..
		timed_cancellation_token_source tcs;
		std::chrono::milliseconds timeout(task_timeout_ms);

		try
		{
			create_task([&]
			{
				tcs.cancel(timeout);
				return _clientSocket->ConnectAsync(_serverHost, "80", SocketProtectionLevel::PlainSocket);
			}, tcs.get_token()).then([&]
			{
				currentSpeed += _clientSocket->Information->RoundTripTimeStatistics.Min / 1000000.0;
			}).get();
		}
		catch (Platform::COMException^ e)
		{
			currentSpeed = 0.0;
			retries--;
		}
		catch (task_canceled&)
		{
			currentSpeed = 0.0;
			retries--;
		}

		delete _clientSocket;
	}

	//Compute speed...
	if (currentSpeed == 0.0)
	{
		return ConnectionSpeed::Unknown;
	}
	else
	{
		double rawSpeed = currentSpeed / retries;
		RawSpeed = rawSpeed;
		return GetConnectionSpeed(rawSpeed);
	}
}

IAsyncOperation<ConnectionSpeed>^ InternetConnectionState::GetInternetConnectionSpeed()
{
	if (!Connected)
	{
		return create_async([]() -> ConnectionSpeed
		{
			return ConnectionSpeed::Unknown;
		});
	}

	InternetConnectionState::_serverHost = nullptr;
	InternetConnectionState::_custom = false;

	return create_async([&]() -> ConnectionSpeed
	{
		return InternetConnectionState::InternetConnectSocketAsync();
	});
}

IAsyncOperation<ConnectionSpeed>^ InternetConnectionState::GetInternetConnectionSpeedWithHostName(HostName^ hostName)
{
	if (!Connected)
	{
		return create_async([]() -> ConnectionSpeed
		{
			return ConnectionSpeed::Unknown;
		});
	}

	if (hostName != nullptr)
	{
		InternetConnectionState::_serverHost = hostName;
		InternetConnectionState::_custom = true;
	}

	return create_async([&]() -> ConnectionSpeed
	{
		return InternetConnectionState::InternetConnectSocketAsync();
	});
}

bool InternetConnectionState::Connected::get()
{
	auto internetConnectionProfile = Windows::Networking::Connectivity::NetworkInformation::GetInternetConnectionProfile();
	if (internetConnectionProfile == nullptr)
	{
		InternetConnectionState::_connected = false;
	}
	else
	{
		if (internetConnectionProfile->GetNetworkConnectivityLevel() == NetworkConnectivityLevel::InternetAccess)
		{
			InternetConnectionState::_connected = true;
		}
	}
	return InternetConnectionState::_connected;
}
