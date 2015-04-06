#pragma once
#include "pch.h"
#include "Enums.h"

using namespace Platform;
using namespace Platform::Collections;
using namespace Concurrency;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Networking;
using namespace Windows::Networking::Connectivity;
using namespace std;

namespace InetSpeedUAP
{
	public ref class InternetConnectionState sealed
	{
		static ConnectionType InternetConnectionState::GetConnectionType();
		static property bool _custom;
		static ConnectionSpeed InternetConnectionState::InternetConnectSocketAsync();
		static property bool _connected;
		static property HostName^ _serverHost;
		static ConnectionSpeed InternetConnectionState::GetConnectionSpeed(double roundtriptime);

	public:
		static IAsyncOperation<ConnectionSpeed>^ InternetConnectionState::GetInternetConnectionSpeed();
		static IAsyncOperation<ConnectionSpeed>^ InternetConnectionState::GetInternetConnectionSpeedWithHostName(HostName^ hostName);
		static property bool InternetConnectionState::Connected { bool get(); }
		//static property double InternetConnectionState::RawSpeed;
	};
}

