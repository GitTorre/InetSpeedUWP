//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

using namespace UI;

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
		auto connectionSpeedWithHost = InternetConnectionState::GetInternetConnectionSpeedWithHostName(ref new Windows::Networking::HostName("pinterest.com"));
		auto _speed = create_task(connectionSpeedWithHost);

		_speed.then([this](ConnectionSpeed speed)
		{
			if (speed == ConnectionSpeed::Unknown) return;
			TextBoxResults->Text += speed.ToString() + "\n" + "Raw Speed: " + InternetConnectionState::RawSpeed + "\n";
		});
	}
	else
	{
		TextBoxResults->Text = "Not connected...";
	}
}