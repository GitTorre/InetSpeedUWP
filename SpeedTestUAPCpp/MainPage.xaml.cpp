//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

using namespace SpeedTestUAPCpp;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace InetSpeedUAP;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

MainPage::MainPage()
{
	InitializeComponent();
}

void MainPage::SpeedButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (InternetConnectionState::Connected)
	{
		//auto connectionSpeed = InternetConnectionState::GetInternetConnectionSpeed();

		auto connectionSpeedWithHost = InternetConnectionState::GetInternetConnectionSpeedWithHostName(ref new Windows::Networking::HostName("pinterest.com"));

		auto _speed = create_task(connectionSpeedWithHost);

		_speed.then([this](ConnectionSpeed speed)
		{
			__speed = speed;

			if (speed == ConnectionSpeed::Unknown) return;

			TextBoxResults->Text += "__speed" + " " + __speed.ToString() + "\n";
			//"RawSpeed value: " + InternetConnectionState::RawSpeed + "\n";

			if (speed == ConnectionSpeed::High)
			{
				//This is a highspeed, low latency Internet connection...
			}
			else if (speed == ConnectionSpeed::Average)
			{
				//This is the most common connection speed across Cellular and Wifi. Thus, Average (average speed, average latency)...
			}
			else if (speed == ConnectionSpeed::Low)
			{
				//This is a low speed, high latency connection...
			}
			TextBoxResults->Text += speed.ToString() + "\n";

			switch (speed)
			{
			case ConnectionSpeed::Unknown:
				TextBoxResults->Text += "case: Unknown" + "\n";
				break;
			case ConnectionSpeed::High:
				TextBoxResults->Text += "case: High" + "\n";
				break;
			case ConnectionSpeed::Average:
				TextBoxResults->Text += "case: Average" + "\n";
				break;
			case ConnectionSpeed::Low:
				TextBoxResults->Text += "case: Low" + "\n";
				break;
			}
		});
	}
	else
	{
		TextBoxResults->Text = "Not connected...";
	}
}
